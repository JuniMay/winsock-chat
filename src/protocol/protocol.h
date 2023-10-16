#ifndef PROTOCOL_PROTOCOL_H_
#define PROTOCOL_PROTOCOL_H_

#include <stdint.h>

#define PROTOCOL_BUFFER_SIZE 65535

/// Message type, 4 bytes
typedef enum {
  /// This message should be ignored.
  ///
  /// The length of the message is 8.
  MSG_NONE = 0,
  /// Connect to the server.
  ///
  /// This message is sent by the client to the server.
  /// After receiving this message, the server will reply with a
  /// MSG_REPLY_OK/ERROR.
  /// This should also including the user id.
  ///
  /// The format is
  /// +-+-+-+-+-+-+-+-+-+-+-+-+
  /// |  TYPE |  LEN  |  SRC  |
  /// +-+-+-+-+-+-+-+-+-+-+-+-+
  MSG_CONNECT = 1,
  /// Disconnect from the server.
  ///
  /// This message is sent by the client to the server.
  MSG_DISCONNECT = 2,
  /// Send a message to the server.
  ///
  /// This message is sent by the client to the server.
  /// The format is
  /// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  /// |  TYPE |  LEN  |  SRC  |  DST  | FORMAT| DATA ...  |
  /// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  MSG_SEND = 3,
  /// Join a room.
  ///
  /// This message is sent by the client to the server.
  /// The format is
  /// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  /// |  TYPE |  LEN  |  SRC  |  DST  |
  /// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  /// Where dst is the room id.
  MSG_JOIN = 5,
  /// Leave a room.
  ///
  /// This message is sent by the client to the server.
  /// Format is the same as MSG_JOIN.
  MSG_LEAVE = 6,
  /// Server reply. length is 12 with a reply code.
  MSG_REPLY = 7,
} message_type_t;

/// Reply code from the server
typedef enum {
  /// No reply
  RPL_NONE,
  /// OK
  RPL_OK,
  /// The ID of a `CONNECT` message has been used.
  RPL_DUPLICATED_ID,
  /// The `DST` of a `SEND` message is not found.
  RPL_DST_NOT_FOUND,
  /// The `DST` of a `JOIN` message is not found.
  RPL_ROOM_NOT_FOUND,
} reply_code_t;

/// Message header, 8 bytes
typedef struct {
  /// Message type
  message_type_t type;
  /// Message length, including the header
  uint32_t length;
} message_header_t;

typedef uint32_t ident_t;

/// Connect/Disconnect to the server.
typedef struct {
  /// Header
  message_header_t header;
  /// User id
  ident_t ident;
} msg_conn_t;

/// Send a message to the server.
typedef struct {
  /// Header
  message_header_t header;
  /// Sender
  ident_t src;
  /// Receiver
  ident_t dst;
  /// Message format
  uint32_t format;
} msg_send_t;

/// Join/Leave a room.
typedef struct {
  /// Header
  message_header_t header;
  /// Proposer
  ident_t src;
  /// Room id
  ident_t dst;
} msg_room_t;

/// Server reply.
typedef struct {
  /// Header
  message_header_t header;
  /// Reply code
  reply_code_t code;
} msg_reply_t;

typedef int length_t;
typedef uint32_t format_t;

length_t protocol_wrap_msg_connect(ident_t ident, uint8_t buffer[]);
length_t protocol_wrap_msg_disconnect(ident_t ident, uint8_t buffer[]);

length_t protocol_wrap_msg_send(
  ident_t src,
  ident_t dst,
  format_t format,
  length_t data_len,
  uint8_t data[],
  uint8_t buffer[]
);

length_t protocol_wrap_msg_join(ident_t src, ident_t dst, uint8_t buffer[]);
length_t protocol_wrap_msg_leave(ident_t src, ident_t dst, uint8_t buffer[]);
length_t protocol_wrap_msg_reply(reply_code_t code, uint8_t buffer[]);

#endif