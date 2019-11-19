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

char CoAP_Host[] = "YOUR COAP SERVER"; //"coap.me"
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


uint8_t buffer[BUF_MAX_SIZE] = {0,};

int8_t udp_send(const char *host, uint16_t port, CoapPacket &Packet){
    udp.beginPacket(host, port);
    IPAddress ip = udp.remoteIP();
    int len = Packet.ToArray(buffer);
    udp.write(buffer, len);
    return udp.endPacket();
}

bool udp_receive(CoapPacket &cp, unsigned long udp_timeout_ms = 2000UL){   
    bool received = false;
    int receivedLen = -1;
    unsigned long started = millis();
    while(!received && (millis() - started) < udp_timeout_ms){
        int packetLen = udp.parsePacket();
        if (packetLen > 0){
            receivedLen = udp.read(buffer, packetLen >= BUF_MAX_SIZE?BUF_MAX_SIZE:packetLen);
            received = true;
        }
    }

    if(received&&receivedLen > 0){
        return CoapMessage.parsePackets(buffer, receivedLen, cp);
    }
    return false;
}

void loop(){
    if(status_wl() == wl_status_t::WL_CONNECTED){    
        CoapPacket cp;
        String s = "{\"n\":\"temp\" \
            ,\"d\":21.5 \
            ,\"ac\":\"JLjsl\" \
            ,\"ab\":\"JLjsl\" \
            ,\"ad\":\"JLjsl\" \
            ,\"ae\":\"JLjsl\" \
            ,\"af\":\"JLjsl\" \
            }";

        //CoapMessage.get(CoAP_Host, CoAP_Port, cp, "test");
        CoapMessage.post(CoAP_Host, CoAP_Port, cp, "test", (char*)s.c_str(), s.length(), COAP_CONTENT_TYPE::COAP_APPLICATION_JSON);
        cp.SetQueryString("id=71747859");
        cp.SetQueryString("hash=ecd71870d1963316a97e3ac3408c9835ad8cf0f3c1bc703527c30265534f75ae");
        Serial.printf("Send CoAP message, Code: %d\t MsgID: %d\r\n", cp.code, cp.messageid);
        udp_send(CoAP_Host, CoAP_Port, cp);

        if(udp_receive(cp)){
            Serial.printf("Received CoAP message, Code: %d\t MsgID: %d\r\n", cp.code, cp.messageid);
        }else{
            Serial.println("Parse received data failed!");
        }

        // CoapMessage.ping(CoAP_Host, CoAP_Port, cp);
        // Serial.printf("Send CoAP message, Code: %d\t MsgID: %d\r\n", cp.code, cp.messageid);
        // udp_send(CoAP_Host, CoAP_Port, cp);
    }
    delay(5000);
}