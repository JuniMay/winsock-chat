#include "WS2tcpip.h"
#include "WinSock2.h"
#include "stdio.h"
#include "time.h"

#include "protocol/protocol.h"
#include "server/server.h"

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
  size_t max_clients = 10;
  size_t port = 8888;

  if (argc > 1) {
    max_clients = atoi(argv[1]);
  }

  if (argc > 2) {
    port = atoi(argv[2]);
  }

  WSADATA wsa;

  server_log("initializing winsock...\n");
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    server_log("failed. error code: %d\n", WSAGetLastError());
    return 1;
  }

  server_log("initialized.\n");

  server_state_t state = {
    .port = port,
    .max_clients = max_clients,
  };
  
  if (server_init(&state, port, max_clients) != 0) {
    server_log("failed to initialize server.\n");
    server_cleanup(&state);
    return 1;
  }

  server_loop(&state);

  return 0;
}