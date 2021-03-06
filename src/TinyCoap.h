#ifndef TINY_COAP_H
#define TINY_COAP_H

#include "Arduino.h"
#include "TinyCoapProto.h"

//#define TINY_GSM_DEBUG Serial
#include <TinyGsmClient.h>

#ifndef COAP_TIMEOUT_MS
#define COAP_TIMEOUT_MS     6000UL
#endif
#define COAP_RECEIVE_BUFFER 512


class TinyCoap {
public:
    TinyCoap()
        :modem(NULL), usedWaitResponse(true){}

    bool begin(TinyGsm&    gsm,
            const char* apn,
            const char* user,
            const char* pass,
            const char* domain,
            uint16_t    port   = COAP_DEFAULT_PORT){

        bool state = false;
        config(gsm, domain, port);
        unsigned long started = millis();
        while(!state && 
            (millis() - started < COAP_TIMEOUT_MS * 40)){
            state = connectNetwork(apn, user, pass);
            if (!state){
                DBG(F("Modem restarting ..."));
                modem->restart();        
            }
        }

        if (!state) return false;

        state = false;
        started = millis();
        while(!state && 
            (millis() - started < COAP_TIMEOUT_MS * 10)){      
            state = client.connect(ip.toString().c_str(), port, 10);
        }

        return state;
    }

    bool get(const char *url);
    bool get(const char *url, const char *query);
    bool post(const char *url, char *payload, int payloadlen, COAP_CONTENT_TYPE payloadtype = COAP_CONTENT_TYPE::COAP_NONE);
    bool post(const char *url, char *payload, int payloadlen, const char *query, COAP_CONTENT_TYPE payloadtype = COAP_CONTENT_TYPE::COAP_NONE);
    bool ping();
    void setWaitResponse(bool state);

private:
    bool connectNetwork(const char* apn, const char* user, const char* pass);
    void config(TinyGsm&    gsm,
                const char* domain,
                uint16_t    port = COAP_DEFAULT_PORT);
    bool waitResponce(){return waitResponce(0);}
    bool waitResponce(uint16_t messageid);

private:
    const char*         domain;
    IPAddress           ip;
    uint16_t            port;
    TinyGsm*            modem;
    TinyGsmClientUDP    client;
    Coap                coapMsg;
    CoapPacket          cp;
    uint8_t             Buffer[COAP_RECEIVE_BUFFER];
    bool                usedWaitResponse;
};

void TinyCoap::setWaitResponse(bool state){
    this->usedWaitResponse = state;
}

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
                const char* domain,
                uint16_t    port){
    this->domain = domain;
    this->port = port;
    modem = &gsm;
    client.init(modem);
}

bool TinyCoap::get(const char *url){
    return get(url, NULL);
}

bool TinyCoap::get(const char *url, const char *query){
    this->coapMsg.get(this->ip.toString().c_str(), this->port, this->cp,  url);
    if(query != NULL){
        this->cp.SetQueryString(query);
    }
    if(client.write(this->cp.ToHexString().c_str()) > 0){
        return this->usedWaitResponse? waitResponce(this->cp.messageid):false;
    }else{
        DBG(F("Write message failed"));
    }
    return false;
}

bool TinyCoap::post(const char *url, char *payload, int payloadlen, COAP_CONTENT_TYPE payloadtype){
    return post(url, payload, payloadlen, NULL, payloadtype);
}

bool TinyCoap::post(const char *url, char *payload, int payloadlen, const char *query, COAP_CONTENT_TYPE payloadtype){
    this->coapMsg.post(this->ip.toString().c_str(), this->port, this->cp, url, payload, payloadlen, payloadtype);
    if(query != NULL){
        this->cp.SetQueryString(query);
    }
    if(client.write(this->cp.ToHexString().c_str()) > 0){
        return this->usedWaitResponse? waitResponce(this->cp.messageid):false;
    }else{
        DBG(F("Write message failed"));
    }
    return false;
}

bool TinyCoap::ping(){
    this->coapMsg.ping(this->ip.toString().c_str(), this->port, this->cp);
    if(client.write(this->cp.ToHexString().c_str()) > 0){
        return waitResponce();
    }else{
        DBG(F("Write message failed"));
    }
    return false; 
}

bool TinyCoap::waitResponce(uint16_t messageid){
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
        if(pkt.messageid == messageid)
            return true;
        return false;
    }else{
        DBG(F("Receive timeout exceeded"));
        return false;
    }
}
#endif