#include "HttpUtils.h"
#include <LittleFS.h>

namespace HttpUtils {

String sanitizePath(const String& in) {
  if (in.length() == 0) return "/";
  String p = in;
  if (!p.startsWith("/")) p = "/" + p;
  p.replace("//", "/");
  while (p.indexOf("..") >= 0) p.replace("..", "");
  if (p.length() == 0) p = "/";
  return p;
}

void setNoCache(AsyncWebServerResponse* r){
  if(!r) return;
  r->addHeader("Cache-Control","no-store, no-cache, must-revalidate, max-age=0");
  r->addHeader("Pragma","no-cache");
}
void setCacheLong(AsyncWebServerResponse* r){
  if(!r) return;
  r->addHeader("Cache-Control","public, max-age=31536000, immutable");
}

String guessMime(const String& p){
  if(p.endsWith(".html")) return "text/html";
  if(p.endsWith(".css"))  return "text/css";
  if(p.endsWith(".js"))   return "application/javascript";
  if(p.endsWith(".json")) return "application/json";
  if(p.endsWith(".svg"))  return "image/svg+xml";
  if(p.endsWith(".png"))  return "image/png";
  if(p.endsWith(".jpg") || p.endsWith(".jpeg")) return "image/jpeg";
  if(p.endsWith(".ico"))  return "image/x-icon";
  return "text/plain";
}

String jsonEscape(const String& s){
  String out; out.reserve(s.length()+8);
  for (size_t i=0;i<s.length();++i){
    char c=s[i];
    if(c=='"'||c=='\\') { out += '\\'; out += c; }
    else if(c=='\n') out += "\\n";
    else if(c=='\r') out += "\\r";
    else out += c;
  }
  return out;
}

void sendJson(AsyncWebServerRequest* req, const String& json) {
  auto* res = req->beginResponse(200, "application/json", json);
  res->addHeader("Cache-Control", "no-store");
  req->send(res);
}

void sendFilePlain(AsyncWebServerRequest* req, String path) {
  if (!path.startsWith("/")) path = "/" + path;
  if (!LittleFS.exists(path)) { req->send(404, "text/plain", "Not found"); return; }

  File* f = new File(LittleFS.open(path, "r"));
  if (!(*f)) { delete f; req->send(404, "text/plain", "Not found"); return; }

  const String mime = guessMime(path);
  AsyncWebServerResponse* res = req->beginChunkedResponse(
    mime,
    [f](uint8_t* buffer, size_t maxLen, size_t /*index*/) -> size_t {
      if (!(*f)) return 0;
      size_t n = f->read(buffer, maxLen);
      if (n == 0) { f->close(); delete f; }
      return n;
    }
  );
  if (mime == "text/html") setNoCache(res); else setCacheLong(res);
  req->send(res);
}

} // namespace HttpUtils
