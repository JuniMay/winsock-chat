
#include "stdio.h"
#include "time.h"

#include "WS2tcpip.h"
#include "process.h"

#include "protocol/protocol.h"
#include "server/server.h"

#pragma comment(lib, "ws2_32.lib")

void server_log(const char* msg, ...) {
  time_t now = time(NULL);
  struct tm timeinfo;
  localtime_s(&timeinfo, &now);
  printf(
    "[ %02d:%02d:%02d SERVER ] ", timeinfo.tm_hour, timeinfo.tm_min,
    timeinfo.tm_sec
  );

  va_list args;
  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);
}

int server_init(server_state_t* state, size_t port, size_t max_clients) {
  state->master = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (state->master == INVALID_SOCKET) {
    server_log("could not create socket: %d\n", WSAGetLastError());
    return 1;
  }

  server_log("master socket created.\n");

  state->server.sin_family = AF_INET;
  state->server.sin_addr.s_addr = INADDR_ANY;
  state->server.sin_port = htons((u_short)port);

  if (bind(state->master, (struct sockaddr*)&state->server, sizeof(state->server)) == SOCKET_ERROR) {
    server_log("bind failed with error code: %d\n", WSAGetLastError());
    return 1;
  }

  server_log("bind done.\n");

  server_show_info(state);

  return 0;
}

void server_loop(server_state_t* state) {
  listen(state->master, SOMAXCONN);

  server_log("listening on port %d...\n", state->port);

  SOCKET client_socket;
  struct sockaddr_in client_addr;

  int addrlen = sizeof(struct sockaddr_in);

  while (
    (client_socket =
       accept(state->master, (struct sockaddr*)&client_addr, &addrlen))
  ) {
    server_log("connection accepted.\n");

    server_recv_handler_arg_t* arg =
      (server_recv_handler_arg_t*)malloc(sizeof(server_recv_handler_arg_t));
    arg->state = state;
    arg->client_socket = client_socket;

    _beginthread(server_recv_handler, 0, arg);
  }
}

void server_show_info(server_state_t* state) {
  char hostname[256];
  gethostname(hostname, 256);
  struct addrinfo hints = {0};
  struct addrinfo* addrs;

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  getaddrinfo(hostname, NULL, &hints, &addrs);

  char ip[16];
  inet_ntop(AF_INET, &((struct sockaddr_in*)addrs->ai_addr)->sin_addr, ip, 16);

  server_log("ip & port:   %s:%d\n", ip, state->port);
  server_log("max clients: %d\n", state->max_clients);
}

void server_cleanup(server_state_t* state) {
  closesocket(state->master);
  WSACleanup();
}

void server_recv_handler(void* arg) {
  server_recv_handler_arg_t* handler_arg = (server_recv_handler_arg_t*)arg;

  server_log("client handler started.\n");

  SOCKET client_socket = handler_arg->client_socket;

  uint8_t buffer[PROTOCOL_BUFFER_SIZE];

  while (1) {
    int recv_size = recv(client_socket, buffer, PROTOCOL_BUFFER_SIZE, 0);

    server_log("received %d bytes.\n", recv_size);

    if (recv_size == 0) {
      server_log("client disconnected.\n");
      break;
    } else if (recv_size == SOCKET_ERROR) {
      server_log("recv failed with error code: %d\n", WSAGetLastError());
      break;
    }

    // handle tcp reorder
    uint8_t* iter = buffer;

    while (iter < buffer + recv_size) {
      message_header_t* msg_header = (message_header_t*)iter;

      switch (msg_header->type) {
        case MSG_NONE: {
          server_log("received MSG_NONE.\n");
          break;
        }
        case MSG_CONNECT: {
          server_log(
            "received MSG_CONNECT from %d.\n", ((msg_conn_t*)iter)->ident
          );
          break;
        }
        case MSG_DISCONNECT: {
          server_log(
            "received MSG_DISCONNECT from %d.\n", ((msg_conn_t*)iter)->ident
          );
          break;
        }
        case MSG_SEND: {
          server_log(
            "received MSG_SEND from %d to %d. `%s`\n", ((msg_send_t*)iter)->src,
            ((msg_send_t*)iter)->dst, iter + sizeof(msg_send_t)
          );
          break;
        }
        case MSG_JOIN: {
          server_log(
            "received MSG_JOIN from %d to %d.\n", ((msg_room_t*)iter)->src,
            ((msg_room_t*)iter)->dst
          );
          break;
        }
        case MSG_LEAVE: {
          server_log(
            "received MSG_LEAVE from %d to %d.\n", ((msg_room_t*)iter)->src,
            ((msg_room_t*)iter)->dst
          );
          break;
        }
        case MSG_REPLY: {
          server_log("received MSG_REPLY.\n");
          break;
        }
        default: {
          server_log("received unknown message type: %d.\n", msg_header->type);
          break;
        }
      }
      iter += msg_header->length;
    }
  }

  server_log("client handler stopped.\n");
  closesocket(client_socket);
  free(handler_arg);
  _endthread();
}