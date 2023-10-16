#ifndef CLIENT_CLIENT_H_
#define CLIENT_CLIENT_H_

typedef struct {
  struct sockaddr_in server;
  SOCKET socket;
} client_state_t;

void client_log(const char* msg, ...);

int client_init(client_state_t* state, const char* ip, size_t port);
void client_show_info(client_state_t* state);

void client_cleanup(client_state_t* state);

void client_loop(client_state_t* state);

void client_send_handler(void* arg);
void client_recv_handler(void* arg);

#endif // CLIENT_CLIENT_H_
