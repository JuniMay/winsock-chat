#include "WS2tcpip.h"
#include "WinSock2.h"
#include "stdio.h"
#include "time.h"

#include "client/client.h"
#include "protocol/protocol.h"

#include <format>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
  if (argc < 4) {
    printf("Usage: %s <ip> <server port> <ident>\n", argv[0]);
    return 1;
  }
  size_t port = atoi(argv[2]);

  WSADATA wsa;

  ClientState state;

  // set locale chinese
  std::locale::global(std::locale("zh_CN.UTF-8"));
  std::wcin.imbue(std::locale());
  std::wcout.imbue(std::locale());

  state.ident = atoi(argv[3]);

  state.log(L"initializing winsock...");
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    state.log(std::format(L"failed. error code: {}", WSAGetLastError()));
    return 1;
  }

  state.log(L"initialized.");

  if (state.init(argv[1], port) != 0) {
    state.log(L"failed to initialize client.\n");
    state.cleanup();
    return 1;
  }
  state.show_info();

  state.loop();

  state.log(L"disconnected from server.");

  state.cleanup();

  return 0;
}