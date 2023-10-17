
#include "stdio.h"
#include "time.h"

#include "WS2tcpip.h"
#include "process.h"

#include "protocol/protocol.h"
#include "server/server.h"

#include <chrono>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

void ServerState::log(const std::string& msg) {
  // time (C++ style)
  auto const time =
    std::chrono::current_zone()->to_local(std::chrono::system_clock::now());

  std::cout << std::format("[ {} SERVER ] {}", time, msg) << std::endl;
}

int ServerState::init(size_t port, size_t max_clients) {
  this->port = port;
  this->max_clients = max_clients;

  this->master = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (this->master == INVALID_SOCKET) {
    this->log(std::format("could not create socket: {}", WSAGetLastError()));
    return 1;
  }

  this->log("master socket created.");

  this->server.sin_family = AF_INET;
  this->server.sin_addr.s_addr = INADDR_ANY;
  this->server.sin_port = htons((u_short)port);

  if (bind(this->master, (struct sockaddr*)&this->server, sizeof(this->server)) == SOCKET_ERROR) {
    this->log(std::format("bind failed with error code: {}", WSAGetLastError())
    );
    return 1;
  }

  this->log("bind done.");

  this->show_info();

  return 0;
}

void ServerState::loop() {
  listen(this->master, SOMAXCONN);

  this->log(std::format("listening on port {}...", this->port));

  SOCKET client_socket;
  struct sockaddr_in client_addr;

  int addrlen = sizeof(struct sockaddr_in);

  while (
    (client_socket =
       accept(this->master, (struct sockaddr*)&client_addr, &addrlen))
  ) {
    // modify the clients
    this->mutex.lock();

    if (this->clients.size() >= this->max_clients) {
      this->log("client rejected.\n");
      closesocket(client_socket);
      this->mutex.unlock();
      continue;
    }

    this->log("connection accepted.");

    // unlock the mutex
    this->mutex.unlock();

    // create a thread to handle the client
    std::thread(server_recv_handler, this, client_socket).detach();
  }
}

void ServerState::show_info() {
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

  this->log(std::format("server ip:          {}", ip));
  this->log(std::format("server port:        {}", ntohs(this->server.sin_port))
  );
  this->log(std::format("server max clients: {}", this->max_clients));
}

void ServerState::cleanup() {
  closesocket(this->master);
  WSACleanup();
}

void server_recv_handler(ServerState* state, SOCKET socket) {
  uint8_t buffer[PROTOCOL_BUFFER_SIZE] = {0};
  uint8_t reply_buffer[PROTOCOL_BUFFER_SIZE] = {0};

  state->mutex.lock();
  state->socket_mutexes.emplace((size_t)socket, std::make_shared<std::mutex>());
  state->mutex.unlock();

  while (true) {
    int recv_size = recv(socket, (char*)buffer, PROTOCOL_BUFFER_SIZE, 0);
    state->log(std::format("received {} bytes.", recv_size));


    if (recv_size == 0) {
      state->mutex.lock();
      state->log("socket disconnected.");
      state->socket_mutexes.erase(socket);
      state->mutex.unlock();
      break;
    }

    if (recv_size < 0) {
      state->mutex.lock();
      state->log(
        std::format("recv failed with error code: {}", WSAGetLastError())
      );
      state->socket_mutexes.erase(socket);
      state->mutex.unlock();
      break;
    }

    // parse the message
    uint8_t* iter = buffer;
    while (iter < buffer + recv_size) {
      message_header_t* header = (message_header_t*)iter;
      switch (header->type) {
        case MSG_NONE: {
          state->log("received MSG_NONE.");
          break;
        }
        case MSG_CONNECT: {
          msg_conn_t* conn = (msg_conn_t*)iter;
          state->log(std::format("received MSG_CONNECT from: {}", conn->ident));

          state->mutex.lock();
          state->clients.emplace(conn->ident, socket);
          state->mutex.unlock();

          // reply ok
          state->socket_mutexes[socket]->lock();
          protocol_wrap_msg_reply(RPL_OK, reply_buffer);
          send(socket, (char*)reply_buffer, sizeof(msg_reply_t), 0);
          state->socket_mutexes[socket]->unlock();

          break;
        }
        case MSG_DISCONNECT: {
          msg_conn_t* disconn = (msg_conn_t*)iter;
          state->log(
            std::format("received MSG_DISCONNECT from: {}", disconn->ident)
          );
          break;
        }
        case MSG_SEND: {
          msg_send_t* msg = (msg_send_t*)iter;
          state->log(std::format(
            "received MSG_SEND from {} to {} with `{}`", msg->src, msg->dst,
            (char*)(iter + sizeof(msg_send_t))
          ));

          if (state->clients.find(msg->dst) == state->clients.end()) {
            state->log(std::format("client {} not found.", msg->dst));
            state->socket_mutexes[socket]->lock();
            int len = protocol_wrap_msg_reply(RPL_DST_NOT_FOUND, reply_buffer);
            send(socket, (char*)reply_buffer, len, 0);
            break;
          }

          SOCKET dst_socket = state->clients[msg->dst];

          state->socket_mutexes[dst_socket]->lock();
          send(dst_socket, (char*)iter, header->length, 0);
          state->socket_mutexes[dst_socket]->unlock();

          // reply ok
          state->socket_mutexes[socket]->lock();
          protocol_wrap_msg_reply(RPL_OK, reply_buffer);
          send(socket, (char*)reply_buffer, sizeof(msg_reply_t), 0);
          state->socket_mutexes[socket]->unlock();

          break;
        }
        case MSG_JOIN: {
          msg_room_t* join = (msg_room_t*)iter;
          state->log(
            std::format("received MSG_JOIN from {} to {}", join->src, join->dst)
          );
          break;
        }
        case MSG_LEAVE: {
          msg_room_t* leave = (msg_room_t*)iter;
          state->log(std::format(
            "received MSG_LEAVE from {} to {}", leave->src, leave->dst
          ));

          break;
        }
        default: {
          state->log(std::format(
            "received unknown message type: {}", (uint32_t)header->type
          ));
          break;
        }
      }

      // reply ok
      state->socket_mutexes[socket]->lock();
      protocol_wrap_msg_reply(RPL_OK, reply_buffer);
      send(socket, (char*)reply_buffer, sizeof(msg_reply_t), 0);
      state->socket_mutexes[socket]->unlock();

      iter += header->length;
    }
  }
}