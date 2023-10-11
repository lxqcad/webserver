#include "http_messages.hh"

// need to implement logic for handling /cgi-bin requests here

HttpResponse handle_cgi_bin(const HttpRequest& request) {
  HttpResponse response;
  response.http_version = request.http_version;
  // TODO: 
  return response;
}
