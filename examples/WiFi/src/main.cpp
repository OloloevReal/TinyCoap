#include <Arduino.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <WiFiUDP.h>

//#define TINY_GSM_DEBUG Serial
#define ESP_CONNECT_TIMEOUT 15000
#define LED 2

#include "TinyCoapProto.h"

Coap CoapMessage;
WiFiUDP udp;

char ssid[] = "YOUR WIFI SSID";
char pass[] = "YOUR WIFI PASS";

char CoAP_Host[] = "YOUR COAP SERVER";
uint16_t CoAP_Port = 5683;

void connect_wl(){
  Serial.println("Starting Wifi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < ESP_CONNECT_TIMEOUT)
  {
      Serial.printf("Wifi status: %d\r\n", WiFi.status());
      delay(200);
  };
}

wl_status_t status_wl(){
    return WiFi.status();
}

void setup(){
    Serial.begin(115200);

    const int freq = 5;
    const int ledChannel = 0;
    const int resolution = 16;

    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(LED, ledChannel);
    ledcWrite(ledChannel, 3000);

    connect_wl();
    if (status_wl() == wl_status_t::WL_CONNECTED){
        Serial.printf("Connection success! IP - %s\r\n", WiFi.localIP().toString().c_str());
    }else{
        Serial.printf("Connection failed, going to sleep!");
        esp_sleep_enable_timer_wakeup(10 * 1000 * 1000);
        esp_deep_sleep_start();
    }
}

CoapPacket cp;
uint8_t buffer[BUF_MAX_SIZE];

void loop(){
    if(status_wl() == wl_status_t::WL_CONNECTED){    
        udp.beginPacket(CoAP_Host, CoAP_Port);
        IPAddress ip = udp.remoteIP();
        CoapMessage.ping(ip, CoAP_Port, cp);
        int len = cp.ToArray(buffer);
        Serial.printf("Send CoAP message, Code: %d\t MsgID: %d\r\n", cp.code, cp.messageid);
        udp.write(buffer, len);
        udp.endPacket();

        udp.beginPacket(CoAP_Host, CoAP_Port);
        ip = udp.remoteIP();
        CoapMessage.get(ip, CoAP_Port, cp, "data/get");
        cp.SetQueryString("test1=adlajsdljalsd&test3=adlajsdljalsd123123");
        len = cp.ToArray(buffer);
        Serial.printf("Send CoAP message, Code: %d\t MsgID: %d\r\n", cp.code, cp.messageid);
        udp.write(buffer, len);
        udp.endPacket();
    }
    delay(5000);
}