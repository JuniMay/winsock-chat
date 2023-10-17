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

/// State of the server
struct ServerState {
  /// The port of the server
  size_t port;
  /// The maximum number of clients
  size_t max_clients;
  /// The master socket
  SOCKET master;
  /// The server address
  struct sockaddr_in server;

  bool running = true;

  /// The clients
  std::unordered_map<ident_t, SOCKET> clients;
  /// Mutex for sockets
  std::unordered_map<SOCKET, std::shared_ptr<std::mutex>> socket_mutexes;
  /// The rooms
  std::unordered_map<ident_t, std::unordered_set<ident_t>> rooms;

  /// Mutex
  std::mutex mutex;

  /// Print a message to stdout with a prefix
  void log(const std::wstring& msg);
  /// Initialize the server
  int init(size_t port, size_t max_clients);
  /// Main loop of the server
  void loop();
  /// Show information about the server
  void show_info();
  /// Cleanup the server
  void cleanup();
};

/// The handler for receiving messages from the client.
void server_recv_handler(ServerState* state, SOCKET socket);

/// The handler for quitting the server.
void server_quit_handler(ServerState* state);

#endif