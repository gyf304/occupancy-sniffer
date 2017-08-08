#include <stddef.h>
#include "os_type.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "comm.h"
#include "config.h"
#include "endian.h"
#include "xtea.h"
#include "espconn.h"
#include "comm_hxdt.h"

#define HXDT_HTTP_HEADERS \
"POST %s HTTP/1.1\r\n"\
"Host: %s\r\n"\
"User-Agent: ESP8266\r\n"\
"Content-Type: application/octet-stream\r\n"\
"Connection: Close\r\n"\
"Content-Length: %d\r\n\r\n"

#define ROUND_DOWN(N,S) ((N / S) * S)
#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define HXDT_OVERHEAD_SIZE (ROUND_UP(256 + sizeof(struct hxdt_header), 4))
#define HXDT_RECV_BUFFER_SIZE 4096

struct hxdt_header {
  uint32_t version;
  uint32_t xtea_length;
  uint32_t payload_length;
  uint8_t ctr_iv[8];
  uint8_t cbc_mac[8];
};

struct hxdt_state {
  struct espconn tcp_conn;
  esp_tcp tcp_info;
  struct comm_state* cstate;
  uint8_t* send_buffer;
  uint32_t send_buffer_len;
  uint8_t* recv_buffer;
  uint32_t recv_buffer_len;
  bool recv_error;
  ip_addr_t ip;
  os_timer_t timer;
  uint8_t status;
};

enum {
  HXDT_STATUS_NONE = 0,
  HXDT_STATUS_TCP_CONN,
  HXDT_STATUS_OK,
  HXDT_STATUS_DNS_ERROR
};

void comm_hxdt_dns_cb(const char *name, ip_addr_t *ipaddr, void *arg);
void comm_hxdt_connect_cb(void *arg);
void comm_hxdt_recv_cb(void *arg, char* buf, uint16_t len);
void comm_hxdt_sent_cb(void *arg);
void comm_hxdt_discon_cb(void *arg);
void comm_hxdt_recon_cb(void *arg, int8_t err);

void comm_hxdt_free(struct hxdt_state* pstate);

void ICACHE_FLASH_ATTR
comm_send_hxdt(comm_state_t cstate)
{
  #define hxdt_error(err) if (cstate->cb) (*cstate->cb)(err, NULL, 0)
  struct hxdt_state* pstate = os_zalloc(sizeof(struct hxdt_state));
  if (pstate == NULL) {
    hxdt_error(COMM_MALLOC_ERROR);
    return;
  }
  pstate->cstate = cstate;
  const struct hxdt_info* info = cstate->info.hxdt;
  const uint32_t xtea_size = ROUND_UP(cstate->size, XTEA_BLOCK_SIZE);
  // round up to XTEA_BLOCK_SIZE
  // initialize tcp buffer
  uint8_t* hxdt_buffer = os_malloc(HXDT_OVERHEAD_SIZE + xtea_size);
  pstate->send_buffer = hxdt_buffer;
  // write to tcp buffer
  int http_header_len = os_sprintf(
    (char*)hxdt_buffer, 
    HXDT_HTTP_HEADERS, 
    info->path, 
    info->hostname, 
    sizeof(struct hxdt_header) + xtea_size);
  if (http_header_len < 0) {
    hxdt_error(COMM_GENERIC_ERROR);
    comm_hxdt_free(pstate);
    return;
  }
  // construct header
  struct hxdt_header* header = (void*)((char*)hxdt_buffer + http_header_len);
  uint8_t* cryptogram_ptr = ((uint8_t*)header) + sizeof(struct hxdt_header);
  header->version = htobe32(0);
  // write data lengths
  header->xtea_length = htobe32(xtea_size);
  header->payload_length = htobe32(cstate->size);
  // generate iv
  os_memset(&header->ctr_iv[0], 0, XTEA_BLOCK_SIZE);
  os_get_random(&header->ctr_iv[0], XTEA_BLOCK_SIZE / 2);
  // copy content
  os_memcpy(cryptogram_ptr, cstate->buffer, cstate->size);
  // pad
  os_memset(cryptogram_ptr + cstate->size, 0, xtea_size - cstate->size);
  // calculate tcp buffer size...
  pstate->send_buffer_len = http_header_len + sizeof(struct hxdt_header) + xtea_size;
  // encrypt using xtea-ctr
  xtea_ctr_info_t ctr_info;
  xtea_cbc_mac_info_t mac_info;
  ctr_info.key = &info->encrypt_key[0];
  ctr_info.iv = &header->ctr_iv[0];
  mac_info.key = &info->auth_key[0];
  mac_info.iv = &info->auth_iv[0];
  os_printf("Encryptkey: %s\n", ctr_info.key);
  // encrypt
  if (XTEA_SUCCESS != xtea_ctr(&ctr_info, cryptogram_ptr, cryptogram_ptr, xtea_size)) {
    hxdt_error(COMM_GENERIC_ERROR);
    comm_hxdt_free(pstate);
    return;
  }
  // calculate cbc-mac
  if (XTEA_SUCCESS != xtea_cbc_mac(&mac_info, &header->cbc_mac[0], cryptogram_ptr, xtea_size)) {
    hxdt_error(COMM_GENERIC_ERROR);
    comm_hxdt_free(pstate);
    return;
  }
  // find dns for host
  pstate->ip.addr = 0;
  switch (
    espconn_gethostbyname(&pstate->tcp_conn, 
    &info->hostname[0], &pstate->ip, comm_hxdt_dns_cb)) {
    case ESPCONN_OK:
      // host already found..., manually call cb
      pstate->status = HXDT_STATUS_TCP_CONN;
      comm_hxdt_dns_cb(NULL, &pstate->ip, (void*)pstate);
      break;
    case ESPCONN_INPROGRESS:
      // ip will be provided upon callback...
      os_printf("dns wait\n");
      pstate->status = HXDT_STATUS_TCP_CONN;
      break;
    case ESPCONN_ARG:
      // badthing happened..
      hxdt_error(COMM_GENERIC_ERROR);
      comm_hxdt_free(pstate);
      break;
    }
  return;
  
  // handed off to comm_hxdt_dns_cb
  #undef hxdt_error
}

void ICACHE_FLASH_ATTR
comm_hxdt_dns_cb(const char *name, ip_addr_t *ipaddr, void *arg)
{
  struct hxdt_state* pstate = (void*)((char*)arg - offsetof(struct hxdt_state, tcp_conn));
  struct comm_state* cstate = pstate->cstate;
  const struct hxdt_info* info = cstate->info.hxdt;
  os_printf("dns cb\n");
  #define hxdt_error(err) if (cstate->cb) (*cstate->cb)(err, NULL, 0)
  //if (ipaddr == NULL) pstate->status = HXDT_STATUS_ERROR;
  if (pstate->ip.addr == 0) {
    // got invalid ip
    hxdt_error(COMM_GENERIC_ERROR);
    comm_hxdt_free(pstate);
    return;
  }
  uint8_t* ip = (void*)ipaddr;
  os_printf("IP: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
  // configure tcp
  pstate->recv_buffer = os_malloc(HXDT_RECV_BUFFER_SIZE);
  pstate->recv_buffer_len = 0;
  pstate->recv_error = false;
  pstate->tcp_conn.proto.tcp = &pstate->tcp_info;
  pstate->tcp_conn.type = ESPCONN_TCP;
  pstate->tcp_conn.state = ESPCONN_NONE;
  pstate->tcp_conn.proto.tcp->remote_port = info->port;
  pstate->tcp_conn.proto.tcp->local_port = espconn_port();
  os_memcpy(&pstate->tcp_conn.proto.tcp->remote_ip[0], &pstate->ip, 4);
  espconn_regist_connectcb(&pstate->tcp_conn, comm_hxdt_connect_cb);
  espconn_regist_reconcb(&pstate->tcp_conn, comm_hxdt_recon_cb);
  // connect
  espconn_connect(&pstate->tcp_conn);
  return;
  #undef hxdt_error
  // handed off to connect_cb or recon_cb
}

void ICACHE_FLASH_ATTR
comm_hxdt_connect_cb(void *arg)
{
  struct hxdt_state* pstate = (void*)((char*)arg - offsetof(struct hxdt_state, tcp_conn));
  struct comm_state* cstate = pstate->cstate;
  os_printf("tcp conn\n");
  #define hxdt_error(err) if (cstate->cb) (*cstate->cb)(err, NULL, 0)
  espconn_regist_recvcb(&pstate->tcp_conn, comm_hxdt_recv_cb);
  espconn_regist_sentcb(&pstate->tcp_conn, comm_hxdt_sent_cb);
  espconn_regist_disconcb(&pstate->tcp_conn, comm_hxdt_discon_cb);
  espconn_send(&pstate->tcp_conn, pstate->send_buffer, pstate->send_buffer_len);
  #undef hxdt_error
  return;
  // handed off to sent_cb, recv_cb, and discon_cb
}

void ICACHE_FLASH_ATTR
comm_hxdt_recon_cb(void *arg, int8_t err)
{
  struct hxdt_state* pstate = (void*)((char*)arg - offsetof(struct hxdt_state, tcp_conn));
  struct comm_state* cstate = pstate->cstate;
  os_printf("tcp recon\n");
  #define hxdt_error(err) if (cstate->cb) (*cstate->cb)(err, NULL, 0)
  hxdt_error(COMM_GENERIC_ERROR);
  comm_hxdt_free(pstate);
  #undef hxdt_error
}

void ICACHE_FLASH_ATTR
comm_hxdt_recv_cb(void *arg, char* buf, uint16_t len)
{
  struct hxdt_state* pstate = (void*)((char*)arg - offsetof(struct hxdt_state, tcp_conn));
  struct comm_state* cstate = pstate->cstate;
  #define hxdt_error(err) if (cstate->cb) (*cstate->cb)(err, NULL, 0)
  // if overflow, set flag.
  if (pstate->recv_error || HXDT_RECV_BUFFER_SIZE - pstate->recv_buffer_len <= len) {
    pstate->recv_error = true;
    return;
  }
  os_memcpy(&pstate->recv_buffer[pstate->recv_buffer_len], buf, len);
  return;
  #undef hxdt_error
}

void ICACHE_FLASH_ATTR
comm_hxdt_sent_cb(void *arg)
{
  // nothing to see here.
}

void ICACHE_FLASH_ATTR
comm_hxdt_discon_cb(void *arg)
{
  struct hxdt_state* pstate = (void*)((char*)arg - offsetof(struct hxdt_state, tcp_conn));
  struct comm_state* cstate = pstate->cstate;
  #define hxdt_error(err) if (cstate->cb) (*cstate->cb)(err, NULL, 0)
  os_printf("tcp discon\n");
  if (pstate->recv_error) {
    hxdt_error(COMM_BUFFER_OVERFLOW);
    comm_hxdt_free(pstate);
    return;
  }
  if (pstate->recv_buffer_len < 5) {
    // length is less than HTTP minimum;
    hxdt_error(COMM_GENERIC_ERROR);
    comm_hxdt_free(pstate);
    return;
  }
  // look for HTTP 200
  if (os_memcmp(pstate->recv_buffer, "200", 3)) {
    hxdt_error(COMM_GENERIC_ERROR);
    comm_hxdt_free(pstate);
    return;
  }
  // seek for \r\n
  uint32_t start_idx = 0;
  for (uint32_t i = 0; i<pstate->recv_buffer_len-3; i++) {
    if (0 == os_memcmp(&pstate->recv_buffer[i], "\r\n\r\n", 4))
    {
      start_idx = i+4;
    }
  }
  if (start_idx == 0) {
    // did not find \r\n\r\n sequence, bad.
    hxdt_error(COMM_GENERIC_ERROR);
    comm_hxdt_free(pstate);
    return;
  }
  uint32_t hxdt_len = pstate->recv_buffer_len - start_idx;
  // verify that hxdt_len is of the correct size;
  if (hxdt_len == 0) {
    hxdt_error(COMM_OK); // size of 0 is valid: empty response.
    comm_hxdt_free(pstate);
    return;
  }
  // to be implemented: XTEA reading...
  hxdt_error(COMM_OK);
  comm_hxdt_free(pstate);
  return;
  #undef hxdt_error
}

void ICACHE_FLASH_ATTR
comm_hxdt_free(struct hxdt_state* pstate)
{
  if (pstate) {
    if (pstate->send_buffer) os_free(pstate->send_buffer);
    if (pstate->recv_buffer) os_free(pstate->recv_buffer);
    os_free(pstate);
  }
}
