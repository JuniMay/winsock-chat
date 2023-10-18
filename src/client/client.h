#ifndef CLIENT_CLIENT_H_
#define CLIENT_CLIENT_H_

#include "WinSock2.h"

#include <string>
#include <mutex>

#include "protocol/protocol.h"

/// The state of the client
struct ClientState {
  /// The socket to the server
  SOCKET s;
  /// The server address
  struct sockaddr_in server;
  /// The ident of the client
  ident_t ident;
  /// Mutex
  std::mutex mutex;
  /// Whether the client is running
  bool running = true;
  /// Whether the client log function is enabled
  bool log_enabled = true;

  /// Mark the message is sent but not replied by the server
  bool replied = false;
  /// The condition variable for replied to synchronize the client threads
  std::condition_variable replied_cv;
  
  /// Print a message to stdout with a prefix
  void log(const std::wstring& msg);
  /// Initialize the client
  int init(char* ip, size_t port);
  /// Main loop of the client
  void loop();
  /// Show information about the connected server.
  void show_info();
  /// Cleanup the client
  void cleanup();
};

/// The handler for receiving messages from the server.
void client_recv_handler(ClientState* state);



#endif // CLIENT_CLIENT_H_
