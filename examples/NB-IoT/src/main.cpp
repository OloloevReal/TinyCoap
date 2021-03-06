#include <Arduino.h>

/*
ESP32 connecte to SIM7020E:
    GPIO-5  - EN power SIM7020E
    GPIO-18 - PWR pin SIM7020E
    GPIO-17 - RX SIM7020E
    GPIO-16 - TX SIM7020E   
*/

//#define TINY_GSM_DEBUG Serial
#define TINY_GSM_MODEM_SIM7020E
#define LED 2

#include "TinyCoap.h"
TinyCoap tinyCoap;

#define SerialAT Serial2
TinyGsm modem(SerialAT);

char host[] = "";

void modemOpen(){
    Serial.println(F("Activate external power source"));
    gpio_pad_select_gpio(GPIO_NUM_5);
    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_5, HIGH);

    Serial.println(F("Switch on power SIM7020E"));
    gpio_pad_select_gpio(GPIO_NUM_18);
    gpio_set_direction(GPIO_NUM_18, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_18, HIGH);
    delay(800);
    gpio_set_level(GPIO_NUM_18, LOW);
    delay(2000);
    SerialAT.begin(115200);
}

void modemClosed(){
    Serial.println(F("Switch off power SIM7020E"));
    gpio_pad_select_gpio(GPIO_NUM_18);
    gpio_set_direction(GPIO_NUM_18, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_18, HIGH);
    delay(1000);
    gpio_set_level(GPIO_NUM_18, LOW);
    SerialAT.flush();
}

void setup(){
    Serial.begin(115200);
    Serial.println(F("Booted"));

    const int freq = 5;
    const int ledChannel = 0;
    const int resolution = 16;

    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(LED, ledChannel);
    ledcWrite(ledChannel, 3000);
    modemOpen();


    Serial.println(F("Starting SIM7020E modem, init connection"));
    tinyCoap.begin(modem, NULL, NULL, NULL, host);
    Serial.print(F("[MODEM] IMEI: "));
    Serial.println(modem.getIMEI().c_str());
    Serial.print(F("[MODEM] IMSI: "));
    Serial.println(modem.getIMSI().c_str());
    Serial.print(F("[MODEM] IsGprsConnected: "));
    Serial.println(modem.isGprsConnected());
    Serial.print(F("[MODEM] IP: "));
    Serial.println(modem.localIP().toString().c_str());

    
}

void loop(){
    String s = "{\"n\":\"temp\" \
                ,\"d\":21.5 \
                ,\"ac\":\"JLjsl\" \
                ,\"ab\":\"JLjsl\" \
                ,\"ad\":\"JLjsl\" \
                ,\"ae\":\"JLjsl\" \
                ,\"af\":\"JLjsl\" \
                }";

    tinyCoap.setWaitResponse(true);
    Serial.printf("Send message: %s\r\n", tinyCoap.get("data")?"Success":"Failed");
    tinyCoap.setWaitResponse(false);
    Serial.printf("Send message: %s\r\n", tinyCoap.get("data", "id=71747859&hash=ecd71870d1963316a97e3ac3408c9835ad8cf0f3c1bc703527c30265534f75ae")?"Success":"Failed");
    Serial.printf("Send message: %s\r\n", tinyCoap.post("data", (char*)s.c_str(), s.length(), "id=71747859&hash=ecd71870d1963316a97e3ac3408c9835ad8cf0f3c1bc703527c30265534f75ae", COAP_CONTENT_TYPE::COAP_APPLICATION_JSON)?"Success":"Failed");
    delay(5000);
}