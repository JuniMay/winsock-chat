#include "WS2tcpip.h"
#include "WinSock2.h"
#include "stdio.h"
#include "time.h"

#include "client/client.h"
#include "protocol/protocol.h"

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
  if (argc < 3) {
    printf("Usage: %s <ip> <server port>\n", argv[0]);
    return 1;
  }
  size_t port = atoi(argv[2]);

  WSADATA wsa;

  client_log("initializing winsock...\n");
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    client_log("failed. error code: %d\n", WSAGetLastError());
    return 1;
  }

  client_log("initialized.\n");

  client_state_t client_state = {0};

  if (client_init(&client_state, argv[1], port) != 0) {
    client_log("failed to initialize client.\n");
    client_cleanup(&client_state);
    return 1;
  }
  client_show_info(&client_state);

  uint8_t message[PROTOCOL_BUFFER_SIZE] = {0};

  // simple "hello server" string without wrapping
  length_t len =
    protocol_wrap_msg_send(0, 0, 0, 13, (uint8_t*)"hello server", message);

  for (int i = 0; i < 10; i++) {
    // send the message
    if (send(client_state.socket, (char*)message, (int)len, 0) < 0) {
      client_log("send to server failed.\n");
      return 1;
    }
    client_log("message sent to server. length: %d\n", len);
  }

  // disconnect from the server
  len = protocol_wrap_msg_disconnect(0, message);

  if (send(client_state.socket, (char*)message, (int)len, 0) < 0) {
    client_log("send to server failed.\n");
    return 1;
  }

  client_log("disconnected from server.\n");

  client_cleanup(&client_state);

  return 0;
}