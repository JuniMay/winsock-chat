#include "WS2tcpip.h"
#include "WinSock2.h"
#include "stdio.h"
#include "time.h"

#include "protocol/protocol.h"
#include "server/server.h"

#include <iostream>
#include <string>
#include <format>

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

  ServerState state;

  state.log("initializing winsock...");
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    state.log(std::format("failed. error code: {}", WSAGetLastError()));
    return 1;
  }

  state.log("initialized.");

  if (state.init(port, max_clients) != 0) {
    state.log("failed to initialize server.");
    state.cleanup();
    return 1;
  }
  
  state.loop();

  return 0;
}