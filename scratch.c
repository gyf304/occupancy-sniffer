//Main code function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
  // struct ip_info info;
  // wifi_get_ip_info(STATION_IF, &info);
  // if (wifi_station_get_connect_status() == STATION_GOT_IP && info.ip.addr != 0) {
  //   os_printf("IPOK\n");
  // } else {
  //   os_printf("IPWAIT\n");
  // }
  // os_delay_us(50000);
  system_os_post(USER_TASK_PRIO_0, 0, 0 );
}


void ICACHE_FLASH_ATTR
client_setup(){
  os_printf("CONNTEST\n");
  wifi_set_opmode(STATION_MODE);
  const static char ssid[32] = SSID;
  const static char password[64] = PASSWORD;
  wifi_promiscuous_enable(0);
  struct station_config conf;
  os_memset(&conf, 0, sizeof(struct station_config));
  conf.bssid_set = 0;
  os_memcpy(&conf.ssid, ssid, 32);
  os_memcpy(&conf.password, password, 64);
  conf.bssid_set = 0;
  wifi_station_set_config(&conf);
  wifi_station_clear_cert_key();
  wifi_station_clear_enterprise_ca_cert();
  wifi_station_set_wpa2_enterprise_auth(1); 
  wifi_station_set_enterprise_username(PEAP_USERNAME, sizeof(PEAP_USERNAME)-1);
  wifi_station_set_enterprise_password(PEAP_PASSWORD, sizeof(PEAP_PASSWORD)-1);
  wifi_station_connect();
  os_printf("conn attempt\n");
}
