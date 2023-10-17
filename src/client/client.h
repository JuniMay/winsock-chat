#ifndef CLIENT_CLIENT_H_
#define CLIENT_CLIENT_H_

#include "WinSock2.h"

#include <string>
#include <mutex>

#include "protocol/protocol.h"

struct ClientState {
  SOCKET s;
  struct sockaddr_in server;

  ident_t ident;

  std::mutex mutex;

  void log(const std::string& msg);
  int init(char* ip, size_t port);

  void loop();

  void show_info();
  void cleanup();
};

void client_recv_handler(ClientState* state);



#endif // CLIENT_CLIENT_H_
