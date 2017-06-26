

int main() {
  uint8_t key[16];
  uint8_t nonce[8];
  uint8_t text[256];
  uint8_t out[256];
  uint8_t mac[8];
  memset(&key[0], 0, 16);
  memset(&nonce[0], 0, 8);
  memset(&text[0], 0, 256);
  xtea_ctr_info_t ctr_info;
  xtea_cbc_mac_info_t mac_info;
  key[0] = 1;
  ctr_info.key = &key[0];
  ctr_info.iv  = &nonce[0];
  mac_info.key = &key[0];
  mac_info.iv  = &nonce[0];
  xtea_ctr(&ctr_info, &out[0], &text[0], 256);
  xtea_cbc_mac(&mac_info, &mac[0], &text[0], 16);
  for(int i=0; i<64; i++) {
    printf("%02x", out[i]);
  }
  printf("\n");
  for(int i=0; i<8; i++) {
    printf("%02x", mac[i]);
  }
  printf("\n");
  return 0;
}
