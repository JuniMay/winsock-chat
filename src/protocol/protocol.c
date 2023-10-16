#include "protocol/protocol.h"
#include "string.h"

length_t protocol_wrap_msg_connect(ident_t ident, uint8_t buffer[]) {
  msg_conn_t msg = {
    .header = {.type = MSG_CONNECT, .length = 12}, .ident = ident};

  memcpy(buffer, &msg, sizeof(msg_conn_t));

  return 12;
}

length_t protocol_wrap_msg_disconnect(ident_t ident, uint8_t buffer[]) {
  msg_conn_t msg = {
    .header = {.type = MSG_DISCONNECT, .length = 12}, .ident = ident};

  memcpy(buffer, &msg, sizeof(msg_conn_t));

  return 12;
}

length_t protocol_wrap_msg_send(
  ident_t src,
  ident_t dst,
  format_t format,
  length_t data_len,
  uint8_t data[],
  uint8_t buffer[]
) {
  msg_send_t msg = {
    .header = {.type = MSG_SEND, .length = 20 + data_len},
    .src = src,
    .dst = dst,
    .format = format};

  memcpy(buffer, &msg, sizeof(msg_send_t));
  memcpy(buffer + sizeof(msg_send_t), data, data_len);

  return 20 + data_len;
}

length_t protocol_wrap_msg_join(ident_t src, ident_t dst, uint8_t buffer[]) {
  msg_room_t msg = {
    .header = {.type = MSG_JOIN, .length = 16}, .src = src, .dst = dst};

  memcpy(buffer, &msg, sizeof(msg_room_t));

  return 16;
}

length_t protocol_wrap_msg_leave(ident_t src, ident_t dst, uint8_t buffer[]) {
  msg_room_t msg = {
    .header = {.type = MSG_LEAVE, .length = 16}, .src = src, .dst = dst};

  memcpy(buffer, &msg, sizeof(msg_room_t));

  return 16;
}

length_t protocol_wrap_msg_reply(reply_code_t code, uint8_t buffer[]) {
  msg_reply_t msg = {.header = {.type = MSG_REPLY, .length = 12}, .code = code};

  memcpy(buffer, &msg, sizeof(msg_reply_t));

  return 12;
}