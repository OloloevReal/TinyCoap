#ifndef TINY_COAP_H
#define TINY_COAP_H

#include "Arduino.h"
#include "TinyCoapProto.h"

#ifndef TINY_GSM_USE_HEX
#define TINY_GSM_USE_HEX
#endif

#define TINY_GSM_USE_HEX
#define TINY_GSM_MODEM_SIM7020E
//#define TINY_GSM_DEBUG Serial
#include <TinyGsmClient.h>

#ifndef COAP_TIMEOUT_MS
#define COAP_TIMEOUT_MS     3000UL
#endif
#define COAP_RECEIVE_BUFFER 512


class TinyCoap {
public:
    TinyCoap()
        :modem(NULL){}

    void begin(const char* auth,
            TinyGsm&    gsm,
            const char* apn,
            const char* user,
            const char* pass,
            const char* domain,
            uint16_t    port   = COAP_DEFAULT_PORT){

        config(gsm, auth, domain, port);
        unsigned long started = millis();
        while(connectNetwork(apn, user, pass) != true && 
            (millis() - started < COAP_TIMEOUT_MS * 40)){
            DBG(F("Modem restarting ..."));
            modem->restart();        
        }

        started = millis();
        while(client.connect(ip.toString().c_str(), port, 10) != true && 
            (millis() - started < COAP_TIMEOUT_MS * 10)){      
        }
                
    }

    bool get(const char *url);
    bool post(const char *url, char *payload, int payloadlen, COAP_CONTENT_TYPE payloadtype = COAP_CONTENT_TYPE::COAP_NONE);
    bool ping();

private:
    bool connectNetwork(const char* apn, const char* user, const char* pass);
    void config(TinyGsm&    gsm,
                const char* auth,
                const char* domain,
                uint16_t    port = COAP_DEFAULT_PORT);
    bool waitResponce();

private:
    const char*         domain;
    IPAddress           ip;
    uint16_t            port;
    TinyGsm*            modem;
    TinyGsmClientUDP    client;
    Coap                coapMsg;
    uint8_t             Buffer[COAP_RECEIVE_BUFFER];
};

bool TinyCoap::connectNetwork(const char* apn, const char* user, const char* pass){
    DBG(F("Modem init ..."));
    if (!modem->begin()){
        DBG(F("Modem init failed"));
        return false;
    }

    DBG(F("Registration to network..."));
    if (modem->waitForNetwork()) {
        String op = modem->getOperator();
        DBG(F("Network: "), op);
    } else {
        DBG(F("Register in network failed"));
        return false;
    }

    DBG(F("Connecting to "), apn, F(" ..."));
    if (!modem->gprsConnect(apn, user, pass)) {
        DBG(F("Connect NB-IOT failed"));
        return false;
    }

    IPAddress ip;
    if(!ip.fromString(this->domain)){
        DBG(F("Query DNS: "), this->domain);
        String s = modem->queryDNS(this->domain);
        DBG(F("DNS result: "), s);
        if(s.length() < 8){
            DBG(F("DNS result failed"));
            return false;
        }
        ip.fromString(s);
    }
    this->ip = ip;
    DBG(F("IP Address: "), this->ip.toString());
    return true;
}

void TinyCoap::config(TinyGsm&    gsm,
                const char* auth,
                const char* domain,
                uint16_t    port){
    this->domain = domain;
    this->port = port;
    modem = &gsm;
    client.init(modem);
}

bool TinyCoap::get(const char *url){
    String msg = this->coapMsg.get(this->ip, this->port, url);
    if(client.write(msg.c_str()) > 0){
        return waitResponce();
    }else{
        DBG(F("Write message failed"));
    }
    return false;
}

bool TinyCoap::post(const char *url, char *payload, int payloadlen, COAP_CONTENT_TYPE payloadtype){
    String msg = this->coapMsg.post(this->ip, this->port, url, payload, payloadlen, payloadtype);
    if(client.write(msg.c_str()) > 0){
        return waitResponce();
    }else{
        DBG(F("Write message failed"));
    }
    return false;
}

bool TinyCoap::waitResponce(){
    unsigned long started = millis();
    while(client.available() == 0  && (millis() - started < COAP_TIMEOUT_MS * 3)){
        DBG(F("Waiting response ..."));
    }
    int len = client.available();
    if(len > 0){
        if(len > COAP_RECEIVE_BUFFER){
            DBG(F("The received data is more than COAP_RECEIVE_BUFFER"));
            return false;
        }
        client.readBytes(Buffer, len);
        CoapPacket pkt;
        uint16_t lenPkg = 0;
        lenPkg = Buffer[0] | Buffer[0 + 1];
        if(!this->coapMsg.parsePackets(Buffer + 2, lenPkg, pkt)){ //buf + 2 because skip first two bytes, it is a length received data
            DBG(F("Failed to parse received message"));
            return false;
        }
        return true;
    }else{
        DBG(F("Receive timeout exceeded"));
        return false;
    }
}

bool TinyCoap::ping(){
    String msg = this->coapMsg.ping(this->ip, this->port);
    if(client.write(msg.c_str()) > 0){
        return waitResponce();
    }else{
        DBG(F("Write message failed"));
    }
    return false;
    
}


#endif