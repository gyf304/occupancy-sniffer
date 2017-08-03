#include "os_type.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "comm.h"
#include "config.h"
#include "endian.h"
#include "xtea.h"

#define xstr(s) str(s)
#define str(s) #s

#define HXDT_HTTP_HEADERS \
"User-Agent: ESP8266\r\n"\
"Content-Type: application/octet-stream\r\n"\
"Connection: Close\r\n"

#define HXDT_VER_INFO_SIZE 4
#define HXDT_RAW_LENGTH_INFO_SIZE 4
#define HXDT_IV_SIZE XTEA_BLOCK_SIZE
#define HXDT_CBC_MAC_SIZE XTEA_BLOCK_SIZE
#define HXDT_PAYLOAD_LENGTH_INFO_SIZE 4
#define HXDT_PLAINTEXT_OVERHEAD_SIZE (HXDT_VER_INFO_SIZE+HXDT_RAW_LENGTH_INFO_SIZE+HXDT_CBC_MAC_SIZE+HXDT_IV_SIZE)
#define HXDT_OVERHEAD_SIZE (HXDT_PLAINTEXT_OVERHEAD_SIZE+HXDT_PAYLOAD_LENGTH_INFO_SIZE)

struct hxdt_state {
  char* hostname;
  uint8_t* buffer;
  uint32_t buffer_size;
  uint16_t port;
  uint32_t len;
  uint32_t content_length_pos;
  uint32_t payload_pos;
  uint8_t encrypt_key[XTEA_KEY_SIZE];
  uint8_t auth_key[XTEA_KEY_SIZE];
  uint8_t auth_iv[XTEA_BLOCK_SIZE];
} __attribute__ ((aligned (4)));

struct comm_state {
  uint8_t protocol;
} __attribute__ ((aligned (4)));

static inline bool hxdt_write_buf(struct hxdt_state* state, void* str, uint32_t len);
static inline bool hxdt_write_str(struct hxdt_state* state, char* str);
static uint8_t hxdt_init( struct hxdt_state* state, 
                          const struct hxdt_info* info, 
                          const uint32_t buflen);
static uint8_t hxdt_write (struct hxdt_state* state, uint8_t* buf, uint32_t size);
static uint8_t hxdt_finalize (struct hxdt_state* state);
static uint8_t hxdt_send (struct hxdt_state* state);
#define hxdt_write_str_lit(buf, s) hxdt_write_buf((buf), (s), sizeof(s)-1)

uint8_t ICACHE_FLASH_ATTR
comm_create(comm_state_t* state_ptr, const comm_info_t* info, uint32_t buflen)
{
  switch (info->protocol) {
    case COMM_PROTOCOL_HXDT: {
      uint32_t size = sizeof(struct comm_state) + sizeof(struct hxdt_state);
      comm_state_t state = os_malloc(size);
      if (!state) return COMM_MALLOC_ERROR;
      os_memset(state, 0, size);
      state->protocol = info->protocol;
      uint8_t err = hxdt_init((struct hxdt_state*)(((uint8_t*)state) + sizeof(struct comm_state)), &info->hxdt, buflen);
      if (err == COMM_OK) *state_ptr = state; 
      else os_free(state);
      return err;
      break;
    }
    default:
      return COMM_UNSUPPORTED_PROTOCOL;
      break;
  }
}

uint8_t ICACHE_FLASH_ATTR
comm_write(comm_state_t state, uint8_t* buf, uint32_t buflen)
{
  os_printf("writing len %du\n", buflen);
  switch (state->protocol) {
    case COMM_PROTOCOL_HXDT:
      return hxdt_write((struct hxdt_state*)(((uint8_t*)state) + sizeof(struct comm_state)), buf, buflen);
      break;
    default:
      return COMM_GENERIC_ERROR;
      break;
  }
}

uint8_t ICACHE_FLASH_ATTR
comm_destroy(comm_state_t state)
{
  uint8_t err = COMM_OK;
  switch (state->protocol) {
    case COMM_PROTOCOL_HXDT:
      err = hxdt_finalize((struct hxdt_state*)(((uint8_t*)state) + sizeof(struct comm_state)));
      break;
    default:
      err = COMM_GENERIC_ERROR;
      break;
  }
  os_free(state);
  return err;
}

uint8_t ICACHE_FLASH_ATTR
comm_send(comm_state_t state)
{
  uint8_t err = COMM_OK;
  switch (state->protocol) {
    case COMM_PROTOCOL_HXDT:
      err = hxdt_send((struct hxdt_state*)(((uint8_t*)state) + sizeof(struct comm_state)));
      break;
    default:
      err = COMM_GENERIC_ERROR;
      break;
  }
  return err;
}

static inline bool hxdt_write_buf(struct hxdt_state* state, void* str, uint32_t len)
{
  if ((len + state->len + XTEA_BLOCK_SIZE + 1) >= state->buffer_size) return false;
  uint8_t* buf = (uint8_t*)state->buffer;
  os_memcpy(&buf[state->len], str, len);
  state->len += len;
  return true;
}

static inline bool hxdt_write_str(struct hxdt_state* state, char* str)
{
  uint32_t len = os_strlen(str);
  return hxdt_write_buf(state, (uint8_t*)str, len);
}

static uint8_t ICACHE_FLASH_ATTR
hxdt_init(struct hxdt_state* state, const struct hxdt_info* info, const uint32_t buflen)
{
  state->buffer = os_malloc(buflen);
  if (state->buffer == NULL) return COMM_MALLOC_ERROR;
  os_memset(state->buffer, 0, buflen);
  uint32_t hostname_len = os_strlen(info->hostname) + 1;
  state->hostname = os_malloc(hostname_len);
  if (state->hostname == NULL) return COMM_MALLOC_ERROR;
  os_memcpy(state->hostname, info->hostname, hostname_len);
  os_memcpy(&state->encrypt_key[0], info->encrypt_key, XTEA_KEY_SIZE);
  os_memcpy(&state->auth_key[0], info->auth_key, XTEA_KEY_SIZE);
  os_memcpy(&state->auth_iv[0], info->auth_iv, XTEA_BLOCK_SIZE);
  state->port = info->port;
  state->len = 0;
  state->buffer_size = buflen;
  if (!hxdt_write_str_lit(state, "POST ")) return COMM_BUFFER_OVERFLOW;
  if (!hxdt_write_str(state, info->path)) return COMM_BUFFER_OVERFLOW;
  if (!hxdt_write_str_lit(state, " HTTP/1.1\r\n")) return COMM_BUFFER_OVERFLOW;
  if (!hxdt_write_str_lit(state, HXDT_HTTP_HEADERS)) return COMM_BUFFER_OVERFLOW;
  if (!hxdt_write_str_lit(state, "Host: ")) return COMM_BUFFER_OVERFLOW;
  if (!hxdt_write_str(state, info->hostname)) return COMM_BUFFER_OVERFLOW;
  if (state->port != 80) {
    char portstr[8];
    os_memset(&portstr[0], 0, 8);
    os_sprintf(&portstr[0], ":%d", state->port);
    if (!hxdt_write_str(state, &portstr[0])) return COMM_BUFFER_OVERFLOW;
  }
  if (!hxdt_write_str_lit(state, "\r\nContent-Length: ")) return COMM_BUFFER_OVERFLOW;
  state->content_length_pos = state->len;
  if (!hxdt_write_buf(state, "            ", 12-state->len%4)) return COMM_BUFFER_OVERFLOW;
  if (!hxdt_write_str_lit(state, "\r\n\r\n")) return COMM_BUFFER_OVERFLOW;
  // ver/mode:4 , raw_length:4 , cbcmac:8 | (encrypted) payload_length:4
  if (!hxdt_write_buf(state, (uint8_t[HXDT_OVERHEAD_SIZE]){0}, HXDT_OVERHEAD_SIZE)) return COMM_BUFFER_OVERFLOW;
  state->payload_pos = state->len;
  #ifdef DEBUG
    os_printf("HTTP INIT: \r\n");
    os_printf("%s", state->buffer);
    os_printf("PTR: 0x%08x\n", (uint32_t)&state->buffer[state->len]);
  #endif
  return COMM_OK;
}

static uint8_t ICACHE_FLASH_ATTR
hxdt_finalize (struct hxdt_state* state)
{
  os_free(state->hostname);
  os_free(state->buffer);
  return COMM_OK;
}

static uint8_t ICACHE_FLASH_ATTR
hxdt_write (struct hxdt_state* state, uint8_t* buf, uint32_t size)
{
  if (hxdt_write_buf(state, buf, size)) return COMM_OK;
  return COMM_BUFFER_OVERFLOW;
}

uint8_t ICACHE_FLASH_ATTR
hxdt_send(struct hxdt_state* state)
{
  const uint32_t payload_length = state->len - state->payload_pos;
  const uint32_t xtea_payload_length = payload_length + HXDT_PAYLOAD_LENGTH_INFO_SIZE;
  const uint32_t xtea_padding_length = xtea_payload_length % XTEA_BLOCK_SIZE ? (XTEA_BLOCK_SIZE-xtea_payload_length%XTEA_BLOCK_SIZE) : 0;
  const uint32_t xtea_padded_payload_length = xtea_payload_length + xtea_padding_length;
  const uint32_t raw_length = xtea_padded_payload_length + HXDT_CBC_MAC_SIZE;
  const uint32_t payload_length_pos = state->payload_pos - HXDT_PAYLOAD_LENGTH_INFO_SIZE;
  const uint32_t iv_pos = payload_length_pos - HXDT_IV_SIZE;
  const uint32_t cbc_mac_pos = iv_pos - HXDT_CBC_MAC_SIZE;
  const uint32_t raw_length_pos = cbc_mac_pos - HXDT_RAW_LENGTH_INFO_SIZE;
  const uint32_t ver_info_pos = raw_length_pos - HXDT_VER_INFO_SIZE;
  const uint32_t content_length = xtea_padded_payload_length + HXDT_PLAINTEXT_OVERHEAD_SIZE;
  uint32_t payload_length_be;
  uint32_t raw_length_be;
  endian_convert(&payload_length_be, &payload_length, sizeof(uint32_t));
  endian_convert(&raw_length_be, &raw_length, sizeof(uint32_t));
  os_memcpy(&state->buffer[payload_length_pos], &payload_length_be, sizeof(uint32_t));
  os_memcpy(&state->buffer[raw_length_pos], &raw_length_be, sizeof(uint32_t));
  os_memset(&state->buffer[state->len], 0, XTEA_BLOCK_SIZE); // write 0 to padding.
  os_sprintf((char*)&state->buffer[state->content_length_pos], "%d", content_length);
  
  if (!os_get_random(&state->buffer[iv_pos], HXDT_IV_SIZE)) return COMM_GENERIC_ERROR;

  xtea_ctr_info_t ctr_info;
  xtea_cbc_mac_info_t mac_info;

  ctr_info.key = &state->encrypt_key[0];
  ctr_info.iv = &state->buffer[iv_pos];

  mac_info.key = &state->auth_key[0];
  mac_info.iv = &state->auth_iv[0];

  if (XTEA_SUCCESS != xtea_ctr(&ctr_info, &state->buffer[payload_length_pos], 
      &state->buffer[payload_length_pos], xtea_padded_payload_length))
    return COMM_GENERIC_ERROR;

  if (XTEA_SUCCESS != xtea_cbc_mac(&mac_info, &state->buffer[cbc_mac_pos], 
      &state->buffer[payload_length_pos], xtea_padded_payload_length))
    return COMM_GENERIC_ERROR;

  #ifdef DEBUG
    os_printf("HTTP SEND: \r\n");
    os_printf("%s\n", state->buffer);
  #endif

  return COMM_OK;
}
