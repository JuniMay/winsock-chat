#include "WS2tcpip.h"
#include "WinSock2.h"
#include "stdio.h"
#include "time.h"

#include "client/client.h"
#include "protocol/protocol.h"

#pragma comment(lib, "ws2_32.lib")

void client_log(const char* msg, ...) {
  time_t now = time(NULL);
  struct tm timeinfo;
  localtime_s(&timeinfo, &now);
  printf(
    "[ %02d:%02d:%02d CLIENT ] ", timeinfo.tm_hour, timeinfo.tm_min,
    timeinfo.tm_sec
  );

  va_list args;
  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);
}

int client_init(client_state_t* state, char* ip, size_t port) {
  state->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (state->socket == INVALID_SOCKET) {
    client_log("could not create socket: %d\n", WSAGetLastError());
    return 1;
  }

  client_log("socket created.\n");

  state->server.sin_family = AF_INET;
  inet_pton(AF_INET, ip, &state->server.sin_addr.s_addr);
  state->server.sin_port = htons((u_short)port);

  client_log("connecting to server...\n");

  if (connect(state->socket, (struct sockaddr*)&state->server, sizeof(state->server)) < 0) {
    client_log("connection failed with error code: %d\n", WSAGetLastError());
    return 1;
  }

  client_log("connected.\n");

  return 0;
}

void client_show_info(client_state_t* state) {
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &state->server.sin_addr, ip, INET_ADDRSTRLEN);
  client_log("server ip:   %s\n", ip);
  client_log("server port: %d\n", ntohs(state->server.sin_port));
}

void client_loop(client_state_t* state) {

}

void client_cleanup(client_state_t* state) {
  closesocket(state->socket);
  WSACleanup();
}