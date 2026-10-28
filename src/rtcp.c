#include <stdio.h>
#include <string.h>

#include "address.h"
#include "rtcp.h"
#include "rtp.h"

int rtcp_probe(uint8_t* packet, size_t size) {
  if (size < 8)
    return -1;

  RtpHeader* header = (RtpHeader*)packet;
  return ((header->type >= 64) && (header->type < 96));
}

int rtcp_get_pli(uint8_t* packet, int len, uint32_t ssrc) {
  if (packet == NULL || len != 12)
    return -1;

  memset(packet, 0, len);
  RtcpHeader* rtcp_header = (RtcpHeader*)packet;
  rtcp_header->version = 2;
  rtcp_header->type = RTCP_PSFB;
  rtcp_header->rc = 1;
  rtcp_header->length = htons((len / 4) - 1);
  memcpy(packet + 8, &ssrc, 4);

  return 12;
}

int rtcp_get_fir(uint8_t* packet, int len, int* seqnr) {
  if (packet == NULL || len != 20 || seqnr == NULL)
    return -1;

  memset(packet, 0, len);
  RtcpHeader* rtcp = (RtcpHeader*)packet;
  *seqnr = *seqnr + 1;
  if (*seqnr < 0 || *seqnr >= 256)
    *seqnr = 0;

  rtcp->version = 2;
  rtcp->type = RTCP_PSFB;
  rtcp->rc = 4;
  rtcp->length = htons((len / 4) - 1);
  RtcpFb* rtcp_fb = (RtcpFb*)rtcp;
  RtcpFir* fir = (RtcpFir*)rtcp_fb->fci;
  fir->seqnr = htonl(*seqnr << 24);

  return 20;
}

RtcpRr rtcp_parse_rr(uint8_t* packet) {
  RtcpRr rtcp_rr;
  memcpy(&rtcp_rr.header, packet, sizeof(rtcp_rr.header));
  memcpy(&rtcp_rr.report_block[0], packet + 8, 6 * sizeof(uint32_t));

  return rtcp_rr;
}

static uint32_t rtcp_read_u32(const uint8_t* p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | p[3];
}

int rtcp_parse_remb(const uint8_t* packet, size_t size, uint32_t video_ssrc,
                    uint32_t audio_ssrc, RtcpRemb* remb) {
  size_t pos = 0;
  int found = 0;
  RtcpRemb result = {0};
  if (!packet || !remb || size == 0)
    return -1;

  while (pos < size) {
    const uint8_t* p = packet + pos;
    if (size - pos < 4 || (p[0] >> 6) != 2)
      return -1;
    size_t block_size = ((((size_t)p[2] << 8) | p[3]) + 1) * 4;
    if (block_size > size - pos)
      return -1;
    size_t content_size = block_size;
    if (p[0] & 0x20) {
      uint8_t padding = p[block_size - 1];
      /* RFC 3550：只有复合包末块可以带填充，填充不可吞掉头部。 */
      if (pos + block_size != size || padding == 0 || padding > block_size - 4)
        return -1;
      content_size -= padding;
    }

    if (p[1] == RTCP_PSFB && (p[0] & 0x1f) == 15) {
      if (content_size < 12)
        return -1;
      if (content_size >= 16 && memcmp(p + 12, "REMB", 4) == 0) {
        if (content_size < 20 || p[16] == 0 ||
            content_size != 20 + (size_t)p[16] * 4 || rtcp_read_u32(p + 8) != 0)
          return -1;
        unsigned exponent = p[17] >> 2;
        uint32_t mantissa = ((uint32_t)(p[17] & 3) << 16) |
                            ((uint32_t)p[18] << 8) | p[19];
        /* 检查后再移位，避免大指数导致溢出或未定义行为。 */
        if ((uint64_t)mantissa > (UINT64_MAX >> exponent))
          return -1;
        int has_video = 0;
        int has_audio = 0;
        for (unsigned i = 0; i < p[16]; i++) {
          uint32_t ssrc = rtcp_read_u32(p + 20 + i * 4);
          if (video_ssrc != 0 && ssrc == video_ssrc)
            has_video = 1;
          if (audio_ssrc != 0 && ssrc == audio_ssrc)
            has_audio = 1;
        }
        if (has_video) {
          result.bitrate_bps = (uint64_t)mantissa << exponent;
          result.includes_audio = has_audio;
          found = 1;
        }
      }
    }
    pos += block_size;
  }
  if (found)
    *remb = result;
  return found;
}
