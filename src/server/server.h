#ifndef SERVER_SERVER_H_
#define SERVER_SERVER_H_

#include "WinSock2.h"

typedef struct {
  /// Port to listen to.
  size_t port;
  /// Maximum number of clients.
  size_t max_clients;
  /// Master socket.
  SOCKET master;
  /// Server address.
  struct sockaddr_in server;
  /// Mutex
  HANDLE mutex;
} server_state_t;

typedef struct {
  server_state_t* state;
  SOCKET client_socket;
} server_recv_handler_arg_t;

void server_log(const char* msg, ...);

int server_init(server_state_t* state, size_t port, size_t max_clients);
void server_loop(server_state_t* state);
void server_show_info(server_state_t* state);

void server_cleanup(server_state_t* state);

/// A thread to handle a client.
///
/// Require a mutex to protect the state.
void server_recv_handler(void* arg);

#endif