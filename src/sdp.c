#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "rtp.h"
#include "sdp.h"

int sdp_append(char* sdp, const char* format, ...) {
  va_list argptr;

  va_start(argptr, format);

  if (sdp[0] == '\0') {
    vsnprintf(sdp, CONFIG_SDP_BUFFER_SIZE, format, argptr);
  } else {
    vsnprintf(sdp + strlen(sdp), CONFIG_SDP_BUFFER_SIZE - strlen(sdp), format, argptr);
  }

  if (sdp[strlen(sdp) - 1] != '\n') {
    strcat(sdp, "\r\n");
  }

  va_end(argptr);
  return 0;
}

void sdp_reset(char* sdp) {
  memset(sdp, 0, CONFIG_SDP_BUFFER_SIZE);
}

static const char* sdp_direction_name(SdpDirection direction) {
  switch (direction) {
    case SDP_DIR_SENDONLY:
      return "sendonly";
    case SDP_DIR_RECVONLY:
      return "recvonly";
    case SDP_DIR_INACTIVE:
      return "inactive";
    default:
      return "sendrecv";
  }
}

/* RFC 3264 §6：answer 方向 = offer 取反（未声明视同 sendrecv） */
static SdpDirection sdp_answer_direction(SdpDirection offer_direction) {
  switch (offer_direction) {
    case SDP_DIR_SENDONLY:
      return SDP_DIR_RECVONLY;
    case SDP_DIR_RECVONLY:
      return SDP_DIR_SENDONLY;
    case SDP_DIR_INACTIVE:
      return SDP_DIR_INACTIVE;
    default:
      return SDP_DIR_SENDRECV;
  }
}

/* ANSWER 回显 offer 同 kind 的 a=mid，缺失回退语义名；OFFER 恒语义名 */
static const char* sdp_mid_of(SdpGenerateParam* gen, SdpMediaKind kind, const char* fallback) {
  if (gen->is_answer && gen->remote[kind].mid[0] != '\0')
    return gen->remote[kind].mid;
  return fallback;
}

void sdp_append_h264(char* sdp, SdpGenerateParam* gen) {
  int is_answer = gen->is_answer;
  int is_anchor = (gen->mline_count++ == 0);
  int port = 9;
  const char* mid = "video";
  SdpDirection direction = SDP_DIR_SENDRECV;

  if (is_answer) {
    port = is_anchor ? gen->anchor_port : 0;
    mid = sdp_mid_of(gen, SDP_MEDIA_VIDEO, "video");
    direction = sdp_answer_direction(gen->remote[SDP_MEDIA_VIDEO].direction);
  }

  sdp_append(sdp, "m=video %d UDP/TLS/RTP/SAVPF %d", port, gen->h264_pt);
  if (is_answer && !is_anchor)
    sdp_append(sdp, "a=bundle-only");
  sdp_append(sdp, "c=IN IP4 0.0.0.0");
  sdp_append(sdp, "a=rtcp-fb:%d nack", gen->h264_pt);
  sdp_append(sdp, "a=rtcp-fb:%d nack pli", gen->h264_pt);
  if (gen->enable_remb)
    sdp_append(sdp, "a=rtcp-fb:%d goog-remb", gen->h264_pt);
  if (gen->abs_send_time_id)
    sdp_append(sdp, "a=extmap:%u/sendonly %s", gen->abs_send_time_id, SDP_ABS_SEND_TIME_URI);
  sdp_append(sdp, "a=fmtp:%d profile-level-id=42e01f;level-asymmetry-allowed=1;packetization-mode=1", gen->h264_pt);
  sdp_append(sdp, "a=rtpmap:%d H264/90000", gen->h264_pt);
  sdp_append(sdp, "a=ssrc:1 cname:webrtc-h264");
  sdp_append(sdp, "a=%s", sdp_direction_name(direction));
  sdp_append(sdp, "a=mid:%s", mid);
  sdp_append(sdp, "a=rtcp-mux");
}

void sdp_append_pcma(char* sdp, SdpGenerateParam* gen) {
  int is_answer = gen->is_answer;
  int is_anchor = (gen->mline_count++ == 0);
  int port = 9;
  const char* mid = "audio";
  SdpDirection direction = SDP_DIR_SENDRECV;

  if (is_answer) {
    port = is_anchor ? gen->anchor_port : 0;
    mid = sdp_mid_of(gen, SDP_MEDIA_AUDIO, "audio");
    direction = sdp_answer_direction(gen->remote[SDP_MEDIA_AUDIO].direction);
  }

  sdp_append(sdp, "m=audio %d UDP/TLS/RTP/SAVPF 8", port);
  if (is_answer && !is_anchor)
    sdp_append(sdp, "a=bundle-only");
  sdp_append(sdp, "c=IN IP4 0.0.0.0");
  sdp_append(sdp, "a=rtpmap:8 PCMA/8000");
  sdp_append(sdp, "a=ssrc:4 cname:webrtc-pcma");
  sdp_append(sdp, "a=%s", sdp_direction_name(direction));
  sdp_append(sdp, "a=mid:%s", mid);
  sdp_append(sdp, "a=rtcp-mux");
}

void sdp_append_pcmu(char* sdp, SdpGenerateParam* gen) {
  int is_answer = gen->is_answer;
  int is_anchor = (gen->mline_count++ == 0);
  int port = 9;
  const char* mid = "audio";
  SdpDirection direction = SDP_DIR_SENDRECV;

  if (is_answer) {
    port = is_anchor ? gen->anchor_port : 0;
    mid = sdp_mid_of(gen, SDP_MEDIA_AUDIO, "audio");
    direction = sdp_answer_direction(gen->remote[SDP_MEDIA_AUDIO].direction);
  }

  sdp_append(sdp, "m=audio %d UDP/TLS/RTP/SAVPF 0", port);
  if (is_answer && !is_anchor)
    sdp_append(sdp, "a=bundle-only");
  sdp_append(sdp, "c=IN IP4 0.0.0.0");
  sdp_append(sdp, "a=rtpmap:0 PCMU/8000");
  sdp_append(sdp, "a=ssrc:5 cname:webrtc-pcmu");
  sdp_append(sdp, "a=%s", sdp_direction_name(direction));
  sdp_append(sdp, "a=mid:%s", mid);
  sdp_append(sdp, "a=rtcp-mux");
}

void sdp_append_opus(char* sdp, SdpGenerateParam* gen) {
  int is_answer = gen->is_answer;
  int is_anchor = (gen->mline_count++ == 0);
  int port = 9;
  const char* mid = "audio";
  SdpDirection direction = SDP_DIR_SENDRECV;

  if (is_answer) {
    port = is_anchor ? gen->anchor_port : 0;
    mid = sdp_mid_of(gen, SDP_MEDIA_AUDIO, "audio");
    direction = sdp_answer_direction(gen->remote[SDP_MEDIA_AUDIO].direction);
  }

  sdp_append(sdp, "m=audio %d UDP/TLS/RTP/SAVPF 111", port);
  if (is_answer && !is_anchor)
    sdp_append(sdp, "a=bundle-only");
  sdp_append(sdp, "c=IN IP4 0.0.0.0");
  sdp_append(sdp, "a=rtpmap:111 opus/48000/2");
  sdp_append(sdp, "a=ssrc:6 cname:webrtc-opus");
  sdp_append(sdp, "a=%s", sdp_direction_name(direction));
  sdp_append(sdp, "a=mid:%s", mid);
  sdp_append(sdp, "a=rtcp-mux");
}

void sdp_append_datachannel(char* sdp, SdpGenerateParam* gen) {
  int is_answer = gen->is_answer;
  int is_anchor = (gen->mline_count++ == 0);
  int port = SDP_DATACHANNEL_PORT;
  const char* mid = "datachannel";

  if (is_answer) {
    port = is_anchor ? gen->anchor_port : 0;
    mid = sdp_mid_of(gen, SDP_MEDIA_APPLICATION, "datachannel");
  }

  sdp_append(sdp, "m=application %d UDP/DTLS/SCTP webrtc-datachannel", port);
  if (is_answer && !is_anchor)
    sdp_append(sdp, "a=bundle-only");
  sdp_append(sdp, "c=IN IP4 0.0.0.0");
  sdp_append(sdp, "a=mid:%s", mid);
  sdp_append(sdp, "a=sctp-port:5000");
  sdp_append(sdp, "a=max-message-size:%u", gen->max_message_size ? gen->max_message_size : 262144);
}

void sdp_create(char* sdp, int b_video, int b_audio, int b_datachannel, SdpGenerateParam* gen) {
  char bundle[256];
  sdp_append(sdp, "v=0");
  sdp_append(sdp, "o=- 1495799811084970 1495799811084970 IN IP4 0.0.0.0");
  sdp_append(sdp, "s=-");
  sdp_append(sdp, "t=0 0");
  sdp_append(sdp, "a=msid-semantic: iot");
#if ICE_LITE
  sdp_append(sdp, "a=ice-lite");
#endif
  memset(bundle, 0, sizeof(bundle));

  strcat(bundle, "a=group:BUNDLE");

  if (b_video) {
    strcat(bundle, " ");
    strcat(bundle, sdp_mid_of(gen, SDP_MEDIA_VIDEO, "video"));
  }

  if (b_audio) {
    strcat(bundle, " ");
    strcat(bundle, sdp_mid_of(gen, SDP_MEDIA_AUDIO, "audio"));
  }

  if (b_datachannel) {
    strcat(bundle, " ");
    strcat(bundle, sdp_mid_of(gen, SDP_MEDIA_APPLICATION, "datachannel"));
  }

  sdp_append(sdp, bundle);
}

typedef struct SdpRembSection {
  int is_video;
  int port_zero;
  int bundle_only;
  int has_pt;
  int has_h264;
  int receives;
  SdpRembCapabilities caps;
} SdpRembSection;

static int sdp_remb_section_matches(const SdpRembSection* section) {
  return section->is_video && section->has_pt && section->has_h264 && section->receives &&
         (!section->port_zero || section->bundle_only);
}

static uint8_t sdp_remb_extmap_id(const char* line) {
  char* end;
  unsigned long id = strtoul(line + 9, &end, 10);
  if (end == line + 9 || id == 0 || id > 14)
    return 0;
  if (*end == '/') {
    ++end;
    /* 方向相对于对端；只有对端接收该扩展时本机才可以发送。 */
    if (strncmp(end, "recvonly ", 9) == 0)
      end += 9;
    else if (strncmp(end, "sendrecv ", 9) == 0)
      end += 9;
    else
      return 0;
  } else if (*end == ' ') {
    ++end;
  } else {
    return 0;
  }
  while (*end == ' ')
    ++end;
  size_t uri_size = strlen(SDP_ABS_SEND_TIME_URI);
  if (strncmp(end, SDP_ABS_SEND_TIME_URI, uri_size) != 0 ||
      (end[uri_size] != '\0' && end[uri_size] != ' '))
    return 0;
  return (uint8_t)id;
}

int sdp_get_remb_capabilities(const char* sdp, int h264_pt, SdpRembCapabilities* caps) {
  SdpRembSection section = {0};
  uint8_t session_ast_id = 0;
  int session_receives = 1;
  int in_media = 0;
  if (!sdp || !caps || h264_pt < 96 || h264_pt > 127)
    return -1;
  memset(caps, 0, sizeof(*caps));

  const char* start = sdp;
  while (*start) {
    const char* end = strchr(start, '\n');
    size_t size = end ? (size_t)(end - start) : strlen(start);
    if (size && start[size - 1] == '\r')
      --size;
    char line[512];
    if (size >= sizeof(line)) {
      /* 无法完整读取的媒体行不可沿用上一段的能力。 */
      if (strncmp(start, "m=", 2) == 0) {
        if (sdp_remb_section_matches(&section)) {
          *caps = section.caps;
          return 0;
        }
        memset(&section, 0, sizeof(section));
        in_media = 1;
      }
      start = end ? end + 1 : start + strlen(start);
      continue;
    }
    memcpy(line, start, size);
    line[size] = '\0';
    if (strncmp(line, "m=", 2) == 0) {
      if (sdp_remb_section_matches(&section)) {
        *caps = section.caps;
        return 0;
      }
      memset(&section, 0, sizeof(section));
      in_media = 1;
      section.receives = session_receives;
      section.caps.abs_send_time_id = session_ast_id;
      section.is_video = strncmp(line, "m=video ", 8) == 0;
      if (section.is_video) {
        unsigned port = 0;
        int offset = 0;
        if (sscanf(line, "m=video %u %*s %n", &port, &offset) == 1 && offset > 0 && port <= 65535) {
          section.port_zero = (port == 0);
          const char* token = line + offset;
          while (*token) {
            char* next;
            long pt = strtol(token, &next, 10);
            if (next == token || (*next && *next != ' '))
              break;
            if (pt == h264_pt)
              section.has_pt = 1;
            token = next;
            while (*token == ' ')
              ++token;
          }
        } else {
          section.is_video = 0;
        }
      }
    } else if (strcmp(line, "a=sendonly") == 0 || strcmp(line, "a=inactive") == 0) {
      if (in_media)
        section.receives = 0;
      else
        session_receives = 0;
    } else if (strcmp(line, "a=recvonly") == 0 || strcmp(line, "a=sendrecv") == 0) {
      if (in_media)
        section.receives = 1;
      else
        session_receives = 1;
    } else if (strncmp(line, "a=extmap:", 9) == 0) {
      uint8_t id = sdp_remb_extmap_id(line);
      /* 媒体级同 URI 的拒绝方向或不支持 ID 必须覆盖会话级继承。 */
      if (id || strstr(line, SDP_ABS_SEND_TIME_URI)) {
        if (in_media)
          section.caps.abs_send_time_id = id;
        else
          session_ast_id = id;
      }
    } else if (section.is_video) {
      if (strcmp(line, "a=bundle-only") == 0)
        section.bundle_only = 1;
      int pt;
      char encoding[32];
      if (sscanf(line, "a=rtpmap:%d %31s", &pt, encoding) == 2 && pt == h264_pt &&
          (strcmp(encoding, "H264/90000") == 0 || strcmp(encoding, "h264/90000") == 0))
        section.has_h264 = 1;
      char payload[16], feedback[32], extra[2];
      if (sscanf(line, "a=rtcp-fb:%15s %31s %1s", payload, feedback, extra) == 2 &&
          strcmp(feedback, "goog-remb") == 0) {
        char* tail;
        long feedback_pt = strtol(payload, &tail, 10);
        if (strcmp(payload, "*") == 0 || (*payload && *tail == '\0' && feedback_pt == h264_pt))
          section.caps.goog_remb = 1;
      }
    }
    start = end ? end + 1 : start + strlen(start);
  }
  if (sdp_remb_section_matches(&section))
    *caps = section.caps;
  return 0;
}
