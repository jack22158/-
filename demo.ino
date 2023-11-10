#include <dummy.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "sdk_structs.h"
#include "ieee80211_structs.h"
#include "string_utils.h"
#include <string.h>
#include <stdio.h>
#include <string.h>

#define MAX_SIZE 200 // 假设数组的最大大小为100
int Maxtimes=0;
char strArr[MAX_SIZE][100];  // 字符串数
int counts[MAX_SIZE];        // 字符串出现次数数
int rxrssis[MAX_SIZE];
int strCount = 0;            // 字符串数组中已有的字符串数量
int channel = 1;
void channelHop();
void countString(char *str,int rssis) {
	// 先遍历字符串数组
	int i = 0, exists = 0;
	for (i = 0; i < strCount; i++) {
		if (strcmp(strArr[i], str) == 0) {
			// 如果存在则将出现次数
			counts[i]++;
            rxrssis[i] = rssis;
			exists = 1;
			break;
		}
	}

	if (!exists) {
		// 如果不存在则加入字符串数
        strncpy(strArr[strCount],str,sizeof(strArr[strCount]));
        rxrssis[strCount] = rssis;
		counts[strCount] = 1;
		strCount++;
	}
}
wifi_promiscuous_pkt_type_t packet_type_parser(uint16_t len)
{
    switch(len)
    {
      // If only rx_ctrl is returned, this is an unsupported packet
      case sizeof(wifi_pkt_rx_ctrl_t):
      return WIFI_PKT_MISC;

      // Management packet
      case sizeof(wifi_pkt_mgmt_t):
      return WIFI_PKT_MGMT;

      // Data packet
      default:
      return WIFI_PKT_DATA;
    }
}
static void getMAC(char *addr, uint8_t* data, uint16_t offset) {
  sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", data[offset+0], data[offset+1], data[offset+2], data[offset+3], data[offset+4], data[offset+5]);
}
void wifi_sniffer_packet_handler(uint8_t *buff, uint16_t len)
{
  // First layer: type cast the received buffer into our generic SDK structure
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
  // Second layer: define pointer to where the actual 802.11 packet is within the structure
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  // Third layer: define pointers to the 802.11 packet header and payload
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  const uint8_t *data = ipkt->payload;

  // Pointer to the frame control section within the packet header
  const wifi_header_frame_control_t *frame_ctrl = (wifi_header_frame_control_t *)&hdr->frame_ctrl;

  // Parse MAC addresses contained in packet header into human-readable strings
  char addr1[] = "00:00:00:00:00:00\0";
  char addr2[] = "00:00:00:00:00:00\0";
  char addr3[] = "00:00:00:00:00:00\0";

  mac2str(hdr->addr1, addr1);
  mac2str(hdr->addr2, addr2);
  mac2str(hdr->addr3, addr3);
    
    int rxrssi = ppkt->rx_ctrl.rssi;
  // Print ESSID if beacon
  if (frame_ctrl->type == WIFI_PKT_MGMT && frame_ctrl->subtype == PROBE_REQ)
//   if(frame_ctrl->type == WIFI_PKT_DATA)
  {
    const wifi_mgmt_beacon_t *beacon_frame = (wifi_mgmt_beacon_t*) ipkt->payload;
    char ssid[32] = {0};

    if (beacon_frame->tag_length >= 32)
    {
      strncpy(ssid, beacon_frame->ssid, 31);
    }
    else
    {
      strncpy(ssid, beacon_frame->ssid, beacon_frame->tag_length);
    }
    char addr[] = "00:00:00:00:00:00";
    getMAC(addr,buff,10);
    countString(addr2,rxrssi);
}

}

void setup()
{
  Serial.begin(115200);
  delay(10);
  wifi_set_opmode(STATION_MODE);
  wifi_set_channel(1);
  wifi_promiscuous_enable(0);
  WiFi.disconnect();
  delay(10);
  wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler);
  delay(10);
  wifi_promiscuous_enable(1);

}




void loop()
{
if(Maxtimes==1300){
for (int i = 0; i < strCount; i++)
{
	Serial.printf("\nMac:%s\t", strArr[i]);
	// Serial.printf("%.5f\t", counts[i]/100);
    Serial.printf("times:%d\t", counts[i]);
    Serial.printf("rssi:%d\t",rxrssis[i]);
}
Maxtimes=1;
Serial.printf("\n");
Serial.printf("\n");
for(int j=0;j<MAX_SIZE;j++){
        counts[j] = 0;
    }
}

delay(10);
if (channel > 13) {
    channel = 1;
  }
wifi_set_channel(channel++);
Maxtimes++;
}