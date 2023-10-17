#ifndef SERVER_SERVER_H_
#define SERVER_SERVER_H_

#include "WinSock2.h"

#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "protocol/protocol.h"

struct ServerState {
  size_t port;
  size_t max_clients;
  SOCKET master;
  struct sockaddr_in server;

  std::unordered_map<ident_t, SOCKET> clients;
  /// Mutex for sockets
  std::unordered_map<SOCKET, std::shared_ptr<std::mutex>> socket_mutexes;
  std::unordered_map<ident_t, std::unordered_set<ident_t>> rooms;

  /// Mutex
  std::mutex mutex;

  void log(const std::wstring& msg);
  int init(size_t port, size_t max_clients);
  void loop();

  void show_info();

  void cleanup();
};

void server_recv_handler(ServerState* state, SOCKET socket);

#endif