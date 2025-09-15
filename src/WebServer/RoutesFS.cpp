#include "RoutesFS.h"
#include <LittleFS.h>
#include "HttpUtils.h"

using namespace HttpUtils;

static inline bool guardAuth(AsyncWebServerRequest* req, bool requireAuth){
  if (!requireAuth) return true;
#if defined(WEBSERVER_AUTH_USER) && defined(WEBSERVER_AUTH_PASS)
  if (req->authenticate(WEBSERVER_AUTH_USER, WEBSERVER_AUTH_PASS)) return true;
  req->requestAuthentication();
  return false;
#else
  (void)req;
  return true;
#endif
}

namespace Routes {

void installFS(AsyncWebServer& srv, bool requireAuth){
  // GET /fs/info
  srv.on("/fs/info", HTTP_GET, [requireAuth](AsyncWebServerRequest* req){
    if (!guardAuth(req, requireAuth)) return;
    size_t total = LittleFS.totalBytes();
    size_t used  = LittleFS.usedBytes();
    String json = "{\"total\":" + String(total) + ",\"used\":" + String(used) + "}";
    Serial.printf("[FS] info total=%u used=%u\n", (unsigned)total, (unsigned)used);
    sendJson(req, json);
  });

  // GET /fs/list?path=/dir
  srv.on("/fs/list", HTTP_GET, [requireAuth](AsyncWebServerRequest* req){
    if (!guardAuth(req, requireAuth)) return;
    String path = "/";
    if (req->hasParam("path")) path = req->getParam("path")->value();
    path = sanitizePath(path);

    if (!LittleFS.exists(path)) { req->send(404, "application/json", "{\"error\":\"not_found\"}"); return; }

    File dir = LittleFS.open(path);
    if (!dir || !dir.isDirectory()) { req->send(400, "application/json", "{\"error\":\"not_a_directory\"}"); return; }

    String json = "{\"path\":\"" + jsonEscape(path) + "\",\"entries\":[";
    bool first = true;
    File f = dir.openNextFile();
    while (f) {
      if (!first) json += ","; first = false;
      String name = String(f.name());
      if (name.startsWith(path) && path != "/") name = name.substring(path.length());
      if (name.startsWith("/")) name.remove(0,1);
      json += "{\"name\":\"" + jsonEscape(name) + "\",";
      json += "\"size\":" + String((unsigned)f.size()) + ",";
      json += "\"dir\":"  + String(f.isDirectory() ? "true" : "false") + "}";
      f = dir.openNextFile();
    }
    json += "]}";
    Serial.printf("[FS] list %s\n", path.c_str());
    sendJson(req, json);
  });

  // GET /fs/download?path=/file
  srv.on("/fs/download", HTTP_GET, [requireAuth](AsyncWebServerRequest* req){
    if (!guardAuth(req, requireAuth)) return;
    if (!req->hasParam("path")) { req->send(400, "text/plain", "path required"); return; }
    String path = sanitizePath(req->getParam("path")->value());
    if (!LittleFS.exists(path)) { req->send(404, "text/plain", "not found"); return; }

    String fname = path; int slash = fname.lastIndexOf('/');
    if (slash >= 0 && slash < (int)fname.length()-1) fname = fname.substring(slash+1);

    File* f = new File(LittleFS.open(path, "r"));
    if (!(*f)) { delete f; req->send(404, "text/plain", "not found"); return; }

    AsyncWebServerResponse* res = req->beginChunkedResponse(
      "application/octet-stream",
      [f](uint8_t* buffer, size_t maxLen, size_t)->size_t {
        if (!(*f)) return 0;
        size_t n = f->read(buffer, maxLen);
        if (n == 0) { f->close(); delete f; }
        return n;
      }
    );
    res->addHeader("Content-Disposition", "attachment; filename=\"" + fname + "\"");
    res->addHeader("Cache-Control", "no-store");
    Serial.printf("[FS] download %s\n", path.c_str());
    req->send(res);
  });

  // POST /fs/upload   (multipart/form-data)
  // Concurrency-safe: we gebruiken request->_tempObject om per request een File* te bewaren
  srv.on("/fs/upload", HTTP_POST,
    // completed
    [requireAuth](AsyncWebServerRequest* req){
      if (!guardAuth(req, requireAuth)) return;
      if (req->_tempObject) {
        File* f = reinterpret_cast<File*>(req->_tempObject);
        if (*f) f->close();
        delete f;
        req->_tempObject = nullptr;
      }
      req->send(200, "application/json", "{\"ok\":true}");
    },
    // upload handler
    [requireAuth](AsyncWebServerRequest* req, String filename, size_t index, uint8_t *data, size_t len, bool final){
      if (!guardAuth(req, requireAuth)) return;
      String dir  = req->hasParam("path", true) ? req->getParam("path", true)->value() : "/";
      String over = req->hasParam("overwrite", true) ? req->getParam("overwrite", true)->value() : "0";
      dir = sanitizePath(dir);
      String path = dir;
      if (!path.endsWith("/")) path += "/";
      path += filename;
      path = sanitizePath(path);

      if (index == 0) {
        if (over != "1" && LittleFS.exists(path)) {
          Serial.printf("[FS] upload denied (exists): %s\n", path.c_str());
          req->send(409, "application/json", "{\"error\":\"exists\"}");
          return;
        }
        File* f = new File(LittleFS.open(path, "w"));
        if (!(*f)) {
          delete f;
          Serial.printf("[FS] upload open failed: %s\n", path.c_str());
          req->send(500, "application/json", "{\"error\":\"open_failed\"}");
          return;
        }
        req->_tempObject = f; // per-request file*
        Serial.printf("[FS] upload start %s\n", path.c_str());
      }
      if (len && req->_tempObject) {
        File* f = reinterpret_cast<File*>(req->_tempObject);
        f->write(data, len);
      }
      if (final && req->_tempObject) {
        File* f = reinterpret_cast<File*>(req->_tempObject);
        f->close();
        delete f;
        req->_tempObject = nullptr;
        Serial.printf("[FS] upload done %s (%u bytes)\n", path.c_str(), (unsigned)(index + len));
      }
    }
  );

  // POST /fs/rename  (form fields: from, to)  - overschrijven UIT (409 if to exists)
  srv.on("/fs/rename", HTTP_POST, [requireAuth](AsyncWebServerRequest* req){
    if (!guardAuth(req, requireAuth)) return;
    if (!req->hasParam("from", true) || !req->hasParam("to", true)) {
      req->send(400, "application/json", "{\"error\":\"from_to_required\"}");
      return;
    }
    String from = sanitizePath(req->getParam("from", true)->value());
    String to   = sanitizePath(req->getParam("to",   true)->value());
    if (from == "/") { req->send(400, "application/json", "{\"error\":\"forbidden\"}"); return; }

    if (!LittleFS.exists(from)) { req->send(404, "application/json", "{\"error\":\"source_not_found\"}"); return; }
    if (LittleFS.exists(to))    { req->send(409, "application/json", "{\"error\":\"target_exists\"}");   return; }

    bool ok = LittleFS.rename(from, to);
    Serial.printf("[FS] rename %s -> %s  ok=%d\n", from.c_str(), to.c_str(), (int)ok);
    if (!ok) { req->send(500, "application/json", "{\"error\":\"rename_failed\"}"); return; }
    req->send(200, "application/json", "{\"ok\":true}");
  });
}

} // namespace Routes
