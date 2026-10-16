#ifndef SCTP_H_
#define SCTP_H_

#include "config.h"
#include "dtls_srtp.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DecpMsgType {

  DATA_CHANNEL_OPEN = 0x03,
  DATA_CHANNEL_ACK = 0x02,

} DecpMsgType;

typedef enum DataChannelPpid {

  DATA_CHANNEL_PPID_CONTROL = 50,
  DATA_CHANNEL_PPID_DOMSTRING = 51,
  DATA_CHANNEL_PPID_BINARY_PARTIAL = 52,
  DATA_CHANNEL_PPID_BINARY = 53,
  DATA_CHANNEL_PPID_DOMSTRING_PARTIAL = 54

} DataChannelPpid;

#if !CONFIG_USE_USRSCTP

typedef struct SctpChunkParam {
  uint16_t type;
  uint16_t length;
  uint8_t value[0];

} SctpChunkParam;

typedef enum SctpParamType {

  SCTP_PARAM_STATE_COOKIE = 7,

} SctpParamType;

typedef enum SctpHeaderType {

  SCTP_DATA = 0,
  SCTP_INIT = 1,
  SCTP_INIT_ACK = 2,
  SCTP_SACK = 3,
  SCTP_HEARTBEAT = 4,
  SCTP_HEARTBEAT_ACK = 5,
  SCTP_ABORT = 6,
  SCTP_SHUTDOWN = 7,
  SCTP_SHUTDOWN_ACK = 8,
  SCTP_ERROR = 9,
  SCTP_COOKIE_ECHO = 10,
  SCTP_COOKIE_ACK = 11,
  SCTP_ECNE = 12,
  SCTP_CWR = 13,
  SCTP_SHUTDOWN_COMPLETE = 14,
  SCTP_AUTH = 15,
  SCTP_ASCONF_ACK = 128,
  SCTP_ASCONF = 130,
  SCTP_FORWARD_TSN = 192

} SctpHeaderType;

typedef struct SctpChunkCommon {
  uint8_t type;
  uint8_t flags;
  uint16_t length;

} SctpChunkCommon;

typedef struct SctpForwardTsnChunk {
  SctpChunkCommon common;
  uint32_t new_cumulative_tsn;
  uint16_t stream_number;
  uint16_t stream_sequence_number;

} SctpForwardTsnChunk;

typedef struct SctpHeader {
  uint16_t source_port;
  uint16_t destination_port;
  uint32_t verification_tag;
  uint32_t checksum;

} SctpHeader;

typedef struct SctpPacket {
  SctpHeader header;
  uint8_t chunks[0];

} SctpPacket;

typedef struct SctpSackChunk {
  SctpChunkCommon common;
  uint32_t cumulative_tsn_ack;
  uint32_t a_rwnd;
  uint16_t number_of_gap_ack_blocks;
  uint16_t number_of_dup_tsns;
  uint8_t blocks[0];

} SctpSackChunk;

typedef struct SctpDataChunk {
  uint8_t type;
  uint8_t iube;
  uint16_t length;
  uint32_t tsn;
  uint16_t sid;
  uint16_t sqn;
  uint32_t ppid;
  uint8_t data[0];

} SctpDataChunk;

typedef struct SctpInitChunk {
  SctpChunkCommon common;
  uint32_t initiate_tag;
  uint32_t a_rwnd;
  uint16_t number_of_outbound_streams;
  uint16_t number_of_inbound_streams;
  uint32_t initial_tsn;
  SctpChunkParam param[0];

} SctpInitChunk;

typedef struct SctpCookieEchoChunk {
  SctpChunkCommon common;
  uint8_t cookie[0];
} SctpCookieEchoChunk;

#endif

typedef enum SctpDataPpid {

  PPID_CONTROL = 50,
  PPID_STRING = 51,
  PPID_BINARY = 53,
  PPID_STRING_EMPTY = 56,
  PPID_BINARY_EMPTY = 57

} SctpDataPpid;

#define SCTP_MAX_STREAMS 5
#define SCTP_SINGLE_MESSAGE_MAX 1152
#define SCTP_TX_SLOTS 8
#define SCTP_SEND_BUSY (-2)
#define SCTP_SEND_INVALID (-3)

typedef struct {
  char label[32];  // Stream label
  uint16_t sid;    // Stream ID
} SctpStreamEntry;

typedef struct Sctp {
  struct socket* sock;

  int local_port;
  int remote_port;
  int connected;
  uint8_t association_failed;
  uint32_t verification_tag;
  uint32_t tsn;
  /* 带外协商的单包、无序、零重传模式；旧接口未启用时保持原路径。 */
  uint8_t negotiated;
  void* message_userdata;
  void (*onmessage_ex)(char* msg, size_t len, void* userdata, uint16_t sid, SctpDataPpid type);
#if !CONFIG_USE_USRSCTP
  uint32_t local_tag, rx_tsn, peer_ack, peer_rwnd;
  uint64_t rx_map;
  uint32_t cwnd, ssthresh, ca_bytes, rto, srtt, rttvar, timer_at, forward_tsn;
  uint16_t peer_streams, handshake_len;
  uint8_t phase, retries, forward_pending;
  struct {
    uint32_t tsn, sent_at;
    uint16_t bytes;
    uint8_t state; /* 0=空闲，1=在途，2=gap ACK，3=放弃 */
  } tx[SCTP_TX_SLOTS];
  uint8_t handshake[512];
#endif
  DtlsSrtp* dtls_srtp;
  int stream_count;
  SctpStreamEntry stream_table[SCTP_MAX_STREAMS];

  /* datachannel */
  void (*onmessage)(char* msg, size_t len, void* userdata, uint16_t sid);
  void (*onopen)(void* userdata);
  void (*onclose)(void* userdata);

  void* userdata;
  uint8_t buf[CONFIG_MTU];
} Sctp;

int sctp_create_association(Sctp* sctp, DtlsSrtp* dtls_srtp);

void sctp_destroy_association(Sctp* sctp);

void sctp_usrsctp_init();

void sctp_usrsctp_deinit();

int sctp_is_connected(Sctp* sctp);
/* 全部接口及回调由同一 PeerConnection 循环/锁串行调用。now_ms 为单调毫秒。 */
void sctp_tick(Sctp* sctp, uint32_t now_ms);
int sctp_register_channel(Sctp* sctp, const char* label, uint16_t sid);

void sctp_incoming_data(Sctp* sctp, char* buf, size_t len);

int sctp_outgoing_data(Sctp* sctp, char* buf, size_t len, SctpDataPpid ppid, uint16_t sid);

void sctp_onmessage(Sctp* sctp, void (*onmessage)(char* msg, size_t len, void* userdata, uint16_t sid));

void sctp_onopen(Sctp* sctp, void (*onopen)(void* userdata));

void sctp_onclose(Sctp* sctp, void (*onclose)(void* userdata));

#ifdef __cplusplus
}
#endif

#endif  // SCTP_H_
