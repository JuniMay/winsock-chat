#include "WS2tcpip.h"
#include "WinSock2.h"
#include "stdio.h"
#include "time.h"

#include "client/client.h"
#include "protocol/protocol.h"

#include <chrono>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

void ClientState::log(const std::string& msg) {
  auto const time =
    std::chrono::current_zone()->to_local(std::chrono::system_clock::now());

  std::cout << std::format("[ {} CLIENT ] {}", time, msg) << std::endl;
}

int ClientState::init(char* ip, size_t port) {
  this->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (this->s == INVALID_SOCKET) {
    this->log(std::format("could not create socket: {}", WSAGetLastError()));
    return 1;
  }

  this->log("socket created.");

  this->server.sin_family = AF_INET;
  inet_pton(AF_INET, ip, &this->server.sin_addr.s_addr);
  this->server.sin_port = htons((u_short)port);

  this->log("connecting to server...");

  if (connect(this->s, (struct sockaddr*)&this->server, sizeof(this->server)) < 0) {
    this->log(
      std::format("connect failed with error code: {}", WSAGetLastError())
    );
    return 1;
  }

  this->log("connected.");

  return 0;
}

void ClientState::show_info() {
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &this->server.sin_addr, ip, INET_ADDRSTRLEN);
  this->log(std::format(
    "connected to server at {}:{}", ip, ntohs(this->server.sin_port)
  ));
}

void ClientState::loop() {
  this->log("starting recv thread...");
  std::thread(client_recv_handler, this).detach();

  uint8_t message[PROTOCOL_BUFFER_SIZE] = {0};

  while (true) {
    std::string prompt;
    std::getline(std::cin, prompt);

    if (prompt == "exit") {
      break;
    }

    // split the prompt into tokens
    std::vector<std::string> tokens;
    std::string token;
    bool in_quote = false;

    for (char c : prompt) {
      if (c == '"') {
        in_quote = !in_quote;
        continue;
      }
      if (c == ' ' && !in_quote) {
        if (token.size() > 0) {
          tokens.push_back(token);
          token.clear();
        }
        continue;
      }
      token.push_back(c);
    }

    if (token.size() > 0) {
      tokens.push_back(token);
    }

    if (tokens.size() == 0) {
      continue;
    }

    if (tokens[0] == "send") {
      if (tokens.size() < 3) {
        this->log("usage: send <dst> <message>");
        continue;
      }

      ident_t dst = atoi(tokens[1].c_str());

      // simple "hello server" string without wrapping
      length_t len = protocol_wrap_msg_send(
        this->ident, dst, 0, (int)tokens[2].size(), (uint8_t*)tokens[2].c_str(),
        message
      );

      if (send(this->s, (char*)message, (int)len, 0) < 0) {
        this->log("send to server failed.");
        return;
      }

      this->log("sent message to server.");
    } else if (tokens[0] == "join") {
      if (tokens.size() < 2) {
        this->log("usage: join <room>");
        continue;
      }

      ident_t room = atoi(tokens[1].c_str());

      length_t len = protocol_wrap_msg_join(this->ident, room, message);

      if (send(this->s, (char*)message, (int)len, 0) < 0) {
        this->log("send to server failed.");

        return;
      }

      this->log("sent join message to server.");
    } else if (tokens[0] == "leave") {
      if (tokens.size() < 2) {
        this->log("usage: leave <room>");
        continue;
      }

      ident_t room = atoi(tokens[1].c_str());

      length_t len = protocol_wrap_msg_leave(this->ident, room, message);

      if (send(this->s, (char*)message, (int)len, 0) < 0) {
        this->log("send to server failed.");

        return;
      }

      this->log("sent leave message to server.");
    } else if (tokens[0] == "connect") {
      length_t len = protocol_wrap_msg_connect(this->ident, message);

      if (send(this->s, (char*)message, (int)len, 0) < 0) {
        this->log("send to server failed.");

        return;
      }

      this->log("sent connect message to server.");
    } else if (tokens[0] == "disconnect") {
      length_t len = protocol_wrap_msg_disconnect(this->ident, message);

      if (send(this->s, (char*)message, (int)len, 0) < 0) {
        this->log("send to server failed.");

        return;
      }

      this->log("sent disconnect message to server.");
    } else {
      this->log(std::format("unknown command: {}", tokens[0]));
    }
  }
}

void ClientState::cleanup() {
  closesocket(this->s);
  WSACleanup();
}

void client_recv_handler(ClientState* state) {
  uint8_t buffer[PROTOCOL_BUFFER_SIZE] = {0};

  while (true) {
    int recv_size = recv(state->s, (char*)buffer, PROTOCOL_BUFFER_SIZE, 0);
    if (recv_size == SOCKET_ERROR) {
      state->log(
        std::format("recv failed with error code: {}", WSAGetLastError())
      );
      break;
    }

    if (recv_size == 0) {
      state->log("server disconnected.");
      break;
    }

    state->log(std::format("received {} bytes from server.", recv_size));

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
          msg_send_t* send = (msg_send_t*)iter;
          state->log(std::format(
            "received MSG_SEND from {} to {} with `{}`", send->src, send->dst,
            (char*)(iter + sizeof(msg_send_t))
          ));
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
        case MSG_REPLY: {
          msg_reply_t* reply = (msg_reply_t*)iter;
          state->log(
            std::format("received MSG_REPLY with code: {}", (int)reply->code)
          );
          break;
        }
        default: {
          state->log(std::format(
            "received unknown message type: {}", (uint32_t)header->type
          ));
          break;
        }
      }
      iter += header->length;
    }
  }
}