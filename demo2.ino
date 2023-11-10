#include <Arduino.h>
#include <Ticker.h>
extern "C" {
  #include <user_interface.h>
}
#define DATA_LENGTH           112

#define TYPE_MANAGEMENT       0x00
#define TYPE_CONTROL          0x01
#define TYPE_DATA             0x02
#define SUBTYPE_PROBE_REQUEST 0x04
#define MAX_SIZE 200 
int Maxtimes=0;
char Mac[MAX_SIZE][100];  
char ssids[MAX_SIZE][100];
int counts[MAX_SIZE];        
int rxrssis[MAX_SIZE];
int strCount = 0;      
Ticker timer;      
struct RxControl {
 signed rssi:8; 
 unsigned rate:4;
 unsigned is_group:1;
 unsigned:1;
 unsigned sig_mode:2;
 unsigned legacy_length:12; 
 unsigned damatch0:1;
 unsigned damatch1:1;
 unsigned bssidmatch0:1;
 unsigned bssidmatch1:1;
 unsigned MCS:7; 
 unsigned CWB:1; 
 unsigned HT_length:16;
 unsigned Smoothing:1;
 unsigned Not_Sounding:1;
 unsigned:1;
 unsigned Aggregation:1;
 unsigned STBC:2;
 unsigned FEC_CODING:1; 
 unsigned SGI:1;
 unsigned rxend_state:8;
 unsigned ampdu_cnt:8;
 unsigned channel:4; 
 unsigned:12;
};
void countString(char *str,char *ssid,int rssis) {
	int i = 0, exists = 0;
	for (i = 0; i < strCount; i++) {
		if (strcmp(Mac[i], str) == 0) {
			counts[i]++;
            rxrssis[i] = rssis;
            strncpy(ssids[i],ssid,sizeof(ssids[i]));
			exists = 1;
			break;
		}
	}
	if (!exists) {
        strncpy(Mac[strCount],str,sizeof(Mac[strCount]));
        rxrssis[strCount] = rssis;
		counts[strCount] = 1;
		strCount++;
	}
}
struct SnifferPacket{
    struct RxControl rx_ctrl;
    uint8_t data[DATA_LENGTH];
    uint16_t cnt;
    uint16_t len;
};
static void showMetadata(SnifferPacket *snifferPacket);
static void ICACHE_FLASH_ATTR sniffer_callback(uint8_t *buffer, uint16_t length);
static char* printDataSpan(uint16_t start, uint16_t size, uint8_t* data);
static void getMAC(char *addr, uint8_t* data, uint16_t offset);
void channelHop();

static void showMetadata(SnifferPacket *snifferPacket) {

  unsigned int frameControl = ((unsigned int)snifferPacket->data[1] << 8) + snifferPacket->data[0];

  uint8_t version      = (frameControl & 0b0000000000000011) >> 0;
  uint8_t frameType    = (frameControl & 0b0000000000001100) >> 2;
  uint8_t frameSubType = (frameControl & 0b0000000011110000) >> 4;
  uint8_t toDS         = (frameControl & 0b0000000100000000) >> 8;
  uint8_t fromDS       = (frameControl & 0b0000001000000000) >> 9;
  if (frameType != TYPE_MANAGEMENT ||
      frameSubType != SUBTYPE_PROBE_REQUEST)
        return;
    char addr[] = "00:00:00:00:00:00";
    getMAC(addr, snifferPacket->data, 10);
    uint8_t SSID_length = snifferPacket->data[25];
    char * ssid;
    ssid = printDataSpan(26, SSID_length, snifferPacket->data);
    countString(addr,ssid,snifferPacket->rx_ctrl.rssi);
}
static void ICACHE_FLASH_ATTR sniffer_callback(uint8_t *buffer, uint16_t length) {
  struct SnifferPacket *snifferPacket = (struct SnifferPacket*) buffer;
  showMetadata(snifferPacket);
}

static char* printDataSpan(uint16_t start, uint16_t size, uint8_t* data) {
    char *datat;
    int j = 0;
  for(uint16_t i = start; i < DATA_LENGTH && i < start+size; i++) {
    datat[j] = data[i];
    j++;
  }
  return datat;
}

static void getMAC(char *addr, uint8_t* data, uint16_t offset) {
  sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", data[offset+0], data[offset+1], data[offset+2], data[offset+3], data[offset+4], data[offset+5]);
}

#define CHANNEL_HOP_INTERVAL_MS   1000
static os_timer_t channelHop_timer;
void channelHop()
{
  uint8 new_channel = wifi_get_channel() + 1;
  if (new_channel > 13) {
    new_channel = 1;
  }
  wifi_set_channel(new_channel);
}


void ICACHE_RAM_ATTR onTimerISR() {
    for (int i = 0; i < strCount; i++)
{
	Serial.printf("\nMac:%s\t", Mac[i]);
	Serial.printf("ssid:%s\t", ssids[i]);
    Serial.printf("times:%d\t", counts[i]);
    Serial.printf("rssi:%d\t",rxrssis[i]);
}
for(int i = 0; i < strCount; i++)counts[i]=0;
}
#define DISABLE 0
#define ENABLE  1

void setup() {
  Serial.begin(115200);
  delay(10);
  wifi_set_opmode(STATION_MODE);
  wifi_set_channel(1);
  wifi_promiscuous_enable(DISABLE);
  delay(10);
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  delay(10);
  wifi_promiscuous_enable(ENABLE);
  timer.attach(1, onTimerISR); 
  os_timer_disarm(&channelHop_timer);
  os_timer_setfn(&channelHop_timer, (os_timer_func_t *) channelHop, NULL);
  os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL_MS, 1);
}

void loop() {
  delay(10);
}