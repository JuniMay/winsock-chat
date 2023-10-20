#include "WS2tcpip.h"
#include "WinSock2.h"
#include "stdio.h"
#include "time.h"

#include "client/client.h"
#include "protocol/protocol.h"

#include <chrono>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

void ClientState::log(const std::wstring& msg) {
  if (!this->log_enabled) {
    return;
  }

  auto const time =
    std::chrono::current_zone()->to_local(std::chrono::system_clock::now());

  std::wcout << std::format(L"\033[90m[ {} CLIENT ]\033[0m {}", time, msg)
             << std::endl;
}

int ClientState::init(char* ip, size_t port) {
  // create the socket
  this->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (this->s == INVALID_SOCKET) {
    this->log(std::format(L"could not create socket: {}", WSAGetLastError()));
    return 1;
  }

  this->log(L"socket created.");

  // set server address
  this->server.sin_family = AF_INET;
  // convert ip to binary
  inet_pton(AF_INET, ip, &this->server.sin_addr.s_addr);
  // convert port to network byte order
  this->server.sin_port = htons((u_short)port);

  this->log(L"connecting to server...");

  // connect to server
  if (connect(this->s, (struct sockaddr*)&this->server, sizeof(this->server)) < 0) {
    this->log(
      std::format(L"connect failed with error code: {}", WSAGetLastError())
    );
    return 1;
  }

  this->log(L"connected.");

  return 0;
}

void ClientState::show_info() {
  // get server ip
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &this->server.sin_addr, ip, INET_ADDRSTRLEN);

  // convert ip to wstring
  std::wstring ip_wstr;
  for (auto c : std::string(ip)) {
    ip_wstr.push_back(c);
  }

  // print info
  this->log(std::format(
    L"connected to server at {}:{}", ip_wstr, ntohs(this->server.sin_port)
  ));
}

void ClientState::loop() {
  this->log(L"starting recv thread...");

  // start recv thread
  std::thread recv_handler_thread(client_recv_handler, this);

  // the buffer for sending messages
  uint8_t message[PROTOCOL_BUFFER_SIZE] = {0};

  while (true) {
    // print prefix, ident green
    std::wcout << std::format(L"\033[32m{:^17}\033[0m> ", this->ident);

    std::wstring prompt;

    // get prompt
    std::getline(std::wcin, prompt);

    if (prompt == L"exit") {
      break;
    }

    // split the prompt into tokens
    std::vector<std::wstring> tokens;
    std::wstring token;
    bool in_quote = false;

    for (auto c : prompt) {
      if (c == L'"') {
        in_quote = !in_quote;
        continue;
      }
      if (c == L' ' && !in_quote) {
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

    bool set_reply_flag = true;

    if (tokens[0] == L"send") {
      if (tokens.size() < 3) {
        this->log(L"usage: send <dst> <message>");
        continue;
      }

      ident_t dst = std::stoi(tokens[1]);

      // get length of bytes of the content
      length_t content_len = (int)(tokens[2].length() * sizeof(wchar_t));

      uint8_t* data = (uint8_t*)(tokens[2].c_str());

      // get length of bytes of the message
      length_t len =
        protocol_wrap_msg_send(this->ident, dst, 0, content_len, data, message);

      if (send(this->s, (char*)message, (int)len, 0) < 0) {
        this->log(L"send to server failed.");
        return;
      }

      this->log(L"sent message to server.");
    } else if (tokens[0] == L"join") {
      if (tokens.size() < 2) {
        this->log(L"usage: join <room>");
        continue;
      }

      ident_t room = std::stoi(tokens[1]);

      length_t len = protocol_wrap_msg_join(this->ident, room, message);

      if (send(this->s, (char*)message, (int)len, 0) < 0) {
        this->log(L"send to server failed.");

        return;
      }

      this->log(L"sent join message to server.");
    } else if (tokens[0] == L"leave") {
      if (tokens.size() < 2) {
        this->log(L"usage: leave <room>");
        continue;
      }

      ident_t room = std::stoi(tokens[1]);

      length_t len = protocol_wrap_msg_leave(this->ident, room, message);

      if (send(this->s, (char*)message, (int)len, 0) < 0) {
        this->log(L"send to server failed.");

        return;
      }

      this->log(L"sent leave message to server.");
    } else if (tokens[0] == L"connect") {
      length_t len = protocol_wrap_msg_connect(this->ident, message);

      if (send(this->s, (char*)message, (int)len, 0) < 0) {
        this->log(L"send to server failed.");

        return;
      }

      this->log(L"sent connect message to server.");
    } else if (tokens[0] == L"disconnect") {
      length_t len = protocol_wrap_msg_disconnect(this->ident, message);

      if (send(this->s, (char*)message, (int)len, 0) < 0) {
        this->log(L"send to server failed.");
        return;
      }

      this->log(L"sent disconnect message to server.");
    } else {
      this->log(std::format(L"unknown command: {}", tokens[0]));
      std::wcout << std::format(
                      L"\033[90m{:^17}\033[0m> \033[31munknown command: {}",
                      L"client", tokens[0]
                    )
                 << std::endl;

      set_reply_flag = false;
    }

    if (set_reply_flag) {
      // use condition variable to wait for reply
      std::unique_lock<std::mutex> lock(this->mutex);
      this->replied = false;
      this->replied_cv.wait(lock, [this] { return this->replied; });
    }
  }

  this->mutex.lock();
  this->running = false;
  // wait for thread to join
  recv_handler_thread.join();
  this->cleanup();
  this->mutex.unlock();
}

void ClientState::cleanup() {
  this->log(L"cleaning up...");
  closesocket(this->s);
  WSACleanup();
  this->log(L"cleaned up.");
}

void client_recv_handler(ClientState* state) {
  uint8_t buffer[PROTOCOL_BUFFER_SIZE] = {0};

  // set timeout, so that the thread can exit normally
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  if (setsockopt(state->s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
    state->log(
      std::format(L"setsockopt failed with error code: {}", WSAGetLastError())
    );
    return;
  }

  while (true) {
    if (!state->running) {
      break;
    }

    int recv_size = recv(state->s, (char*)buffer, PROTOCOL_BUFFER_SIZE, 0);
    if (recv_size == SOCKET_ERROR) {
      if (WSAGetLastError() == WSAETIMEDOUT) {
        continue;
      }

      state->log(
        std::format(L"recv failed with error code: {}", WSAGetLastError())
      );
      break;
    }

    if (recv_size == 0) {
      state->log(L"server disconnected.");
      break;
    }

    state->log(std::format(L"received {} bytes from server.", recv_size));

    // parse the message
    uint8_t* iter = buffer;
    while (iter < buffer + recv_size) {
      message_header_t* header = (message_header_t*)iter;
      switch (header->type) {
        case MSG_NONE: {
          state->log(L"received MSG_NONE.");
          break;
        }
        case MSG_SEND: {
          msg_send_t* send = (msg_send_t*)iter;

          int content_len = send->header.length - sizeof(msg_send_t);
          std::wstring wstr;

          wchar_t* content = (wchar_t*)(iter + sizeof(msg_send_t));
          for (int i = 0; i < content_len / sizeof(wchar_t); i++) {
            wstr.push_back(content[i]);
          }

          state->log(std::format(
            L"received MSG_SEND from {} to {} with `{}`", send->src, send->dst,
            wstr
          ));

          // center, 15 alinged
          std::wstring detail = L"";

          if (send->dst != state->ident) {
            if (send->src == state->ident) {
              break;
            }
            detail = std::format(L"@ room {}", send->dst);
          }

          detail = std::format(L"{:^5}{}", send->src, detail);

          std::wcout << std::endl
                     << std::format(L"\033[36m{:^17}\033[0m> {}", detail, wstr);

          break;
        }
        case MSG_REPLY: {
          msg_reply_t* reply = (msg_reply_t*)iter;
          state->log(
            std::format(L"received MSG_REPLY with code: {}", (int)reply->code)
          );

          switch (reply->code) {
            case RPL_NONE: {
              break;
            }
            case RPL_OK: {
              state->log(L"server accomplished the request successfully.");
              break;
            }
            case RPL_SEND_FAILED: {
              state->log(L"server failed to send the message.");
              std::wcout
                << std::format(
                     L"\033[90m{:^17}\033[0m> \033[31mfailed to send the "
                     L"message.\033[0m",
                     L"server"
                   )
                << std::endl;
              break;
            }
            case RPL_DUPLICATED_ID: {
              state->log(L"cannot connect to server with duplicated id.");
              std::wcout
                << std::format(
                     L"\033[90m{:^17}\033[0m> \033[31mplease choose another "
                     L"id.\033[0m",
                     L"server"
                   )
                << std::endl;
              break;
            }
            case RPL_DST_NOT_FOUND: {
              state->log(L"the destination of the message is not found.");

              std::wcout << std::format(
                              L"\033[90m{:^17}\033[0m> \033[31mdestination "
                              L"of the message is "
                              L"not found.\033[0m",
                              L"server"
                            )
                         << std::endl;
              break;
            }
            case RPL_ROOM_NOT_FOUND: {
              state->log(L"the room to join or leave is not found.");
              std::wcout << std::format(
                              L"\033[90m{:^17}\033[0m> \033[31mroom is not "
                              L"found.\033[0m",
                              L"server"
                            )
                         << std::endl;
              break;
            }
            case RPL_NOT_IN_ROOM: {
              state->log(L"the client is not in the room.");
              std::wcout << std::format(
                              L"\033[90m{:^17}\033[0m> \033[31mhave not "
                              L"joined the room "
                              L"yet.\033[0m",
                              L"server"
                            )
                         << std::endl;
              break;
            }
            case RPL_ROOM_CONFLICT: {
              state->log(
                L"the room id for join has conflict with an existing client."
              );
              std::wcout << std::format(
                              L"\033[90m{:^17}\033[0m> \033[31mroom id "
                              L"conflict with client.\033[0m",
                              L"server"
                            )
                         << std::endl;
              break;
            }
          }

          // set replied flag and notify
          std::unique_lock<std::mutex> lock(state->mutex);
          state->replied = true;
          state->replied_cv.notify_all();

          break;
        }
        default: {
          state->log(std::format(
            L"received unknown message type: {}", (uint32_t)header->type
          ));
          break;
        }
      }
      iter += header->length;
    }
  }
}