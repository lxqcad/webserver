#include "http_messages.hh"


HttpResponse handle_htdocs(const HttpRequest& request) {
  HttpResponse response;
  response.http_version = request.http_version;
  // TODO: 
  // Get the request URI, verify the file exists and serve it
  return response;
}
