#include "WS2tcpip.h"
#include "WinSock2.h"
#include "stdio.h"
#include "time.h"

#include "client/client.h"
#include "protocol/protocol.h"

#include <format>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
  if (argc < 4) {
    printf("Usage: %s <ip> <server port> <ident>\n", argv[0]);
    return 1;
  }
  size_t port = atoi(argv[2]);

  WSADATA wsa;

  ClientState state;

  state.ident = atoi(argv[3]);

  state.log("initializing winsock...");
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    state.log(std::format("failed. error code: {}", WSAGetLastError()));
    return 1;
  }

  state.log("initialized.");

  if (state.init(argv[1], port) != 0) {
    state.log("failed to initialize client.\n");
    state.cleanup();
    return 1;
  }
  state.show_info();

  state.loop();

  state.log("disconnected from server.");

  state.cleanup();

  return 0;
}