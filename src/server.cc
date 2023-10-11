/**
 * This file contains the primary logic for the server. It is responsible for
 * handling socket communication - parsing HTTP requests and sending HTTP responses
 * to the client. 
 */

#include <functional>
#include <iostream>
#include <sstream>
#include <vector>
#include <tuple>
#include <thread>
#include <cstring>
#include <iterator>

#include <unistd.h>

#include "server.hh"
#include "http_messages.hh"
#include "errors.hh"
#include "misc.hh"
#include "routes.hh"

Server::Server(SocketAcceptor const& acceptor) : _acceptor(acceptor) { }

void parse_request(const Socket_t &sock, HttpRequest *const request)
{
  // Parse request helper func
  char *getString = new char[4];
  for (int i = 0; i < 3; i++)
  {
    getString[i] = sock->getc();
  }
  getString[3] = '\0';
  if (strcmp("GET", getString) != 0)
  {
    // Not get request
    return;
  }
 
  delete[] getString; // free mem
  sock->getc();       //skip the space
  std::string req = sock->readline();
  std::istringstream iss(req);
  std::vector<std::string> results((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());
 
  request->request_uri = results.at(0);
  request->http_version = results.at(1);
  request->method = "GET";
 
  while (true)
  {
    // parse headers
    std::string headerString = sock->readline();
    if (headerString.empty() || headerString.compare("\r\n") == 0)
    {
      break;
    }
    int tokenIdx = headerString.find(':');
    std::string key = headerString.substr(0, tokenIdx);
    std::string val = headerString.substr(tokenIdx + 2);
    key = trim(key);
    val = trim(val);
    // std::cout << key << std::endl;
    // std::cout << val << std::endl;
    request->headers.insert(std::pair<std::string, std::string>(key, val));
  }
  // handle auth
  auto auth = request->headers.find("Authorization");
  std::string token = "bWE0Mzg6d29zaGltYWpp"; // token generated base64
  if (auth == request->headers.end())
  {
    // didn't find auth
    request->authentication = false;
  }
  else
  {
    std::stringstream ss_auth;
    ss_auth << "Basic " << token;
    if (ss_auth.str().compare(auth->second) != 0)
    {
      // the auth info doesn't matched up
      request->authentication = false;
    }
  }
 
}

static void poolLoop(const Server* s) {
  while(1){
    Socket_t sock = s->_acceptor.accept_connection();
    s->handle(sock);
  }
}

void Server::run_linear() const {
  while (1) {
    Socket_t sock = _acceptor.accept_connection();
    handle(sock);
  }
}

void Server::run_fork() const {
  // TODO: Task 1.4
  while (1)
  {
    Socket_t sock = _acceptor.accept_connection();
    pid_t child = fork();
    if (child == 0)
    { // child
      handle(sock);
      exit(0);
    }
  }
}

void Server::run_thread() const {
  // TODO: Task 1.4
   while (1)
  {
    Socket_t sock = _acceptor.accept_connection();
    // pthread_attr_t attr;
    // pthread_attr_init(&attr);
    // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    // pthread_t t;
    // pthread_create(&t, &attr, Server::handle, sock);
    std::thread t(&Server::handle, this, std::move(sock));
    t.detach();
  }
}

void Server::run_thread_pool(const int num_threads) const {
  // TODO: Task 1.4
  std::vector<std::thread> threads;
  for(int i = 0; i < num_threads; i++){
    threads.push_back(std::thread(poolLoop, this));
  }
  for(int i = 0; i < num_threads; i++){
    threads.at(i).join();
  }
}

std::vector<Route_t> route_map = {
    std::make_pair("/cgi-bin", handle_cgi_bin),
    std::make_pair("/", handle_htdocs),
    std::make_pair("", handle_default),
    std::make_pair("/hello", handle_hello)
};

void Server::handle(const Socket_t& sock) const {
  HttpRequest request;
  // TODO: implement parsing HTTP requests
  // recommendation:
  // void parse_request(const Socket_t& sock, HttpRequest* const request);
  parse_request(sock, &request);
  request.print();
 
  HttpResponse resp;
 
 if (request.authentication == false)
  {
    resp = handle_unauth(request);
  }
  else
  {
    // TODO: Make a response for the HTTP request
    for (Route_t r : route_map)
    {
      if (r.first.compare(request.request_uri) == 0)
      {
        // call the according function here
        resp = r.second(request);
        std::cout << resp.to_string() << std::endl;
        sock->write(resp.to_string());
        return;
      }
    }
    if (request.request_uri.find("/cgi-bin") != std::string::npos)
    {
      // cgi-bin
      resp = handle_cgi_bin(request);
    }
    else if (request.request_uri.find("/") != std::string::npos)
    {
      // serve file request
      resp = handle_htdocs(request);
    }
 
  }
 
  std::cout << resp.to_string() << std::endl;
  sock->write(resp.to_string());
}

