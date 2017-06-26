#ifndef _FRAME_H
#define _FRAME_H

typedef struct {
 signed   int rssi:8; // signal intensity of packet
 unsigned int rate:4;
 unsigned int is_group:1;
 unsigned int :1;
 unsigned int sig_mode:2; // 0:is 11n packet; 1:is not 11n packet;
 unsigned int legacy_length:12; // if not 11n packet, shows length of packet.
 unsigned int damatch0:1;
 unsigned int damatch1:1;
 unsigned int bssidmatch0:1;
 unsigned int bssidmatch1:1;
 unsigned int mcs:7; // if is 11n packet, shows the modulation and code used (range from 0 to 76)
 unsigned int cwb:1; // if is 11n packet, shows if is HT40 packet or not
 unsigned int ht_length:16;// if is 11n packet, shows length of packet.
 unsigned int smoothing:1;
 unsigned int not_sounding:1;
 unsigned int :1;
 unsigned int aggregation:1;
 unsigned int stbc:2;
 unsigned int fec_coding:1; // if is 11n packet, shows if is LDPC packet or not.
 unsigned int sgi:1;
 unsigned int rxend_state:8;
 unsigned int ampdu_cnt:8;
 unsigned int channel:4; //which channel this packet in.
 unsigned int :12;
} rx_control_t;

typedef struct {
  unsigned int version:2;
  unsigned int type:2;
  unsigned int subtype:4;
  unsigned int to_ds:1;
  unsigned int from_ds:1;
  unsigned int more_flag:1;
  unsigned int retry:1;
  unsigned int power:1;
  unsigned int more:1;
  unsigned int w:1;
  unsigned int o:1;

  unsigned int assc_id:16;
  uint8_t addr1[6];
  uint8_t addr2[6];
  uint8_t addr3[6];
  unsigned seq_ctl:16;
  uint8_t addr4[6];
} frame_header_t;

typedef struct {
  rx_control_t rx;
  frame_header_t frame;
} frame_buffer_t;

#endif
