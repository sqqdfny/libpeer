#ifndef RTCP_H_
#define RTCP_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __BYTE_ORDER
#define __BIG_ENDIAN 4321
#define __LITTLE_ENDIAN 1234
#elif __APPLE__
#include <machine/endian.h>
#else
#include <endian.h>
#endif

typedef enum RtcpType {

  RTCP_FIR = 192,
  RTCP_SR = 200,
  RTCP_RR = 201,
  RTCP_SDES = 202,
  RTCP_BYE = 203,
  RTCP_APP = 204,
  RTCP_RTPFB = 205,
  RTCP_PSFB = 206,
  RTCP_XR = 207,

} RtcpType;

typedef struct RtcpHeader {
#if __BYTE_ORDER == __BIG_ENDIAN
  uint16_t version : 2;
  uint16_t padding : 1;
  uint16_t rc : 5;
  uint16_t type : 8;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
  uint16_t rc : 5;
  uint16_t padding : 1;
  uint16_t version : 2;
  uint16_t type : 8;
#endif
  uint16_t length : 16;

} RtcpHeader;

typedef struct RtcpReportBlock {
  uint32_t ssrc;
  uint32_t flcnpl;
  uint32_t ehsnr;
  uint32_t jitter;
  uint32_t lsr;
  uint32_t dlsr;

} RtcpReportBlock;

typedef struct RtcpRr {
  RtcpHeader header;
  uint32_t ssrc;
  RtcpReportBlock report_block[1];

} RtcpRr;

typedef struct RtcpFir {
  uint32_t ssrc;
  uint32_t seqnr;

} RtcpFir;

typedef struct RtcpFb {
  RtcpHeader header;
  uint32_t ssrc;
  uint32_t media;
  char fci[1];

} RtcpFb;

int rtcp_probe(uint8_t* packet, size_t size);

int rtcp_get_pli(uint8_t* packet, int len, uint32_t ssrc);

int rtcp_get_fir(uint8_t* packet, int len, int* seqnr);

RtcpRr rtcp_parse_rr(uint8_t* packet);

typedef struct RtcpRemb {
  uint64_t bitrate_bps;
  int includes_audio;
} RtcpRemb;

/* 返回 1 表示匹配本地视频，0 表示无匹配，-1 表示报文非法。
 * 整个复合包校验通过后才写入结果；允许合法的零带宽估计。 */
int rtcp_parse_remb(const uint8_t* packet, size_t size, uint32_t video_ssrc,
                    uint32_t audio_ssrc, RtcpRemb* remb);

#endif  // RTCP_H_
