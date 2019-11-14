#ifndef TINY_COAP_PROTO_H
#define TINY_COAP_PROTO_H

#include "Arduino.h"

#define COAP_HEADER_SIZE        4
#define COAP_OPTION_HEADER_SIZE 1
#define COAP_PAYLOAD_MARKER     0xFF
#define MAX_OPTION_NUM          10
#define BUF_MAX_SIZE            250
#define COAP_DEFAULT_PORT       5683

#define RESPONSE_CODE(class, detail) ((class << 5) | (detail))
#define COAP_OPTION_DELTA(v, n) (v < 13 ? (*n = (0xFF & v)) : (v <= 0xFF + 13 ? (*n = 13) : (*n = 14)))

typedef enum : uint8_t {
    COAP_CON = 0,
    COAP_NONCON = 1,
    COAP_ACK = 2,
    COAP_RESET = 3
} COAP_TYPE;

typedef enum : uint8_t {
    COAP_EMPTY = 0,
    COAP_GET = 1,
    COAP_POST = 2,
    COAP_PUT = 3,
    COAP_DELETE = 4
} COAP_METHOD;

typedef enum : uint8_t {
    COAP_CREATED = RESPONSE_CODE(2, 1),
    COAP_DELETED = RESPONSE_CODE(2, 2),
    COAP_VALID = RESPONSE_CODE(2, 3),
    COAP_CHANGED = RESPONSE_CODE(2, 4),
    COAP_CONTENT = RESPONSE_CODE(2, 5),
    COAP_BAD_REQUEST = RESPONSE_CODE(4, 0),
    COAP_UNAUTHORIZED = RESPONSE_CODE(4, 1),
    COAP_BAD_OPTION = RESPONSE_CODE(4, 2),
    COAP_FORBIDDEN = RESPONSE_CODE(4, 3),
    COAP_NOT_FOUNT = RESPONSE_CODE(4, 4),
    COAP_METHOD_NOT_ALLOWD = RESPONSE_CODE(4, 5),
    COAP_NOT_ACCEPTABLE = RESPONSE_CODE(4, 6),
    COAP_PRECONDITION_FAILED = RESPONSE_CODE(4, 12),
    COAP_REQUEST_ENTITY_TOO_LARGE = RESPONSE_CODE(4, 13),
    COAP_UNSUPPORTED_CONTENT_FORMAT = RESPONSE_CODE(4, 15),
    COAP_INTERNAL_SERVER_ERROR = RESPONSE_CODE(5, 0),
    COAP_NOT_IMPLEMENTED = RESPONSE_CODE(5, 1),
    COAP_BAD_GATEWAY = RESPONSE_CODE(5, 2),
    COAP_SERVICE_UNAVALIABLE = RESPONSE_CODE(5, 3),
    COAP_GATEWAY_TIMEOUT = RESPONSE_CODE(5, 4),
    COAP_PROXYING_NOT_SUPPORTED = RESPONSE_CODE(5, 5)
} COAP_RESPONSE_CODE;

typedef enum : uint8_t {
    COAP_IF_MATCH = 1,
    COAP_URI_HOST = 3,
    COAP_E_TAG = 4,
    COAP_IF_NONE_MATCH = 5,
    COAP_URI_PORT = 7,
    COAP_LOCATION_PATH = 8,
    COAP_URI_PATH = 11,
    COAP_CONTENT_FORMAT = 12,
    COAP_MAX_AGE = 14,
    COAP_URI_QUERY = 15,
    COAP_ACCEPT = 17,
    COAP_LOCATION_QUERY = 20,
    COAP_PROXY_URI = 35,
    COAP_PROXY_SCHEME = 39
} COAP_OPTION_NUMBER;

typedef enum : int8_t {
    COAP_NONE = -1,
    COAP_TEXT_PLAIN = 0,
    COAP_APPLICATION_LINK_FORMAT = 40,
    COAP_APPLICATION_XML = 41,
    COAP_APPLICATION_OCTET_STREAM = 42,
    COAP_APPLICATION_EXI = 47,
    COAP_APPLICATION_JSON = 50
} COAP_CONTENT_TYPE;

static const char hexmap1[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

String hexStr(const char *data, uint16_t len)
{
    String s = "";
    for (int i = 0; i < len; ++i) {
        s += hexmap1[(data[i] & 0xF0) >> 4];
        s += hexmap1[data[i] & 0x0F];
    }
    return s;
}

class CoapOption {
    public:
    uint8_t number;
    uint8_t length;
    uint8_t *buffer;
};

class Coap;

class CoapPacket {
private:
    IPAddress *ip;
    uint16_t port;
public:
    friend Coap;
    uint8_t type;
    uint8_t code;
    uint8_t *token;
    uint8_t tokenlen;
    uint8_t *payload;
    uint8_t payloadlen;
    uint16_t messageid;
    
    uint8_t optionnum;
    CoapOption options[MAX_OPTION_NUM];
public:
    int ToArray(uint8_t *buffer);
    String ToHexString();
    void SetQueryString(const char *q);
private:
    void AddOption(uint8_t _number, uint8_t *_buffer, uint8_t _length);
    void AddOption(CoapOption *option);
};

void CoapPacket::SetQueryString(const char* q){
    char *line_copy = (char*)malloc(strlen(q) + 1); 
    strcpy(line_copy, q);
    char *token1 = strtok(line_copy, "&");
    while(token1 != NULL){
        AddOption(COAP_URI_QUERY, (uint8_t *)token1, strlen(token1));
        token1 = strtok(NULL, "&");
    }
    free(line_copy);
};

void CoapPacket::AddOption(uint8_t _number, uint8_t *_buffer, uint8_t _length){
    //Serial.printf("AddOption: %d\t%d\t%s\r\n", _number, _length, _buffer);
    if(optionnum < MAX_OPTION_NUM){
        int i = optionnum - 1;
        while( i >= 0){
            if(options[i].number > _number){
                options[i + 1] = options[i];
            }else{
                break;
            }
            --i;
        }
        options[i + 1].buffer = _buffer;
        options[i + 1].length = _length;
        options[i + 1].number = _number;
        optionnum++;
    }
};

void CoapPacket::AddOption(CoapOption *option){
    AddOption(option->number, option->buffer, option->length);
};

class Coap {
public:
    bool get(IPAddress& ip, uint16_t port,  CoapPacket &Packet, const char *url){
        return this->createMsg(ip, port, Packet, url, COAP_CON, COAP_GET, NULL, 0, NULL, 0);
    }
    bool put(IPAddress& ip, uint16_t port, CoapPacket &Packet, const char *url, char *payload, int payloadlen){
        return this->createMsg(ip, port, Packet, url, COAP_CON, COAP_PUT, NULL, 0, (uint8_t *)payload, strlen(payload));
    }
    bool post(IPAddress& ip, uint16_t port, CoapPacket &Packet, const char *url, char *payload, int payloadlen, COAP_CONTENT_TYPE payloadtype){
        return this->createMsg(ip, port, Packet, url, COAP_CON, COAP_POST, NULL, 0, (uint8_t *)payload, payloadlen, payloadtype);
    }
    bool ping(IPAddress& ip, uint16_t port, CoapPacket &Packet){
        return this->createMsg(ip, port, Packet, NULL, COAP_CON, COAP_EMPTY, NULL, 0, NULL, 0);
    }
    bool parsePackets(uint8_t *buffer, size_t size, CoapPacket &Packet);

private:
    bool createMsg(IPAddress &ip, uint16_t port, 
                    CoapPacket &Packet, 
                    const char *url,
                    COAP_TYPE type, COAP_METHOD method, 
                    uint8_t *token, uint8_t tokenlen, 
                    uint8_t *payload, uint32_t payloadlen, COAP_CONTENT_TYPE payloadtype = COAP_CONTENT_TYPE::COAP_NONE);
    int parseOption(CoapOption *option, uint16_t *running_delta, uint8_t **buf, size_t buflen);
};


String CoapPacket::ToHexString(){
    uint8_t buffer[BUF_MAX_SIZE];
    int len = this->ToArray(buffer);
    if (len > 0){
        return hexStr((char *)buffer, len);
    }
    return "";
}

int CoapPacket::ToArray(uint8_t *buffer){
    uint8_t *p = buffer;
    uint16_t running_delta = 0;
    uint16_t packetSize = 0;
    CoapPacket packet = *this;

    // make coap packet base header
    *p = 0x01 << 6;
    *p |= (packet.type & 0x03) << 4;
    *p++ |= (packet.tokenlen & 0x0F);
    *p++ = packet.code;
    *p++ = (packet.messageid >> 8);
    *p++ = (packet.messageid & 0xFF);
    p = buffer + COAP_HEADER_SIZE;
    packetSize += 4;

    // make token
    if (packet.token != NULL && packet.tokenlen <= 0x0F) {
        memcpy(p, packet.token, packet.tokenlen);
        p += packet.tokenlen;
        packetSize += packet.tokenlen;
    }

    // make option header
    for (int i = 0; i < packet.optionnum; i++)  {
        uint32_t optdelta;
        uint8_t len, delta;

        if (packetSize + 5 + packet.options[i].length >= BUF_MAX_SIZE) {
            return -1;
        }
        optdelta = packet.options[i].number - running_delta;
        COAP_OPTION_DELTA(optdelta, &delta);
        COAP_OPTION_DELTA((uint32_t)packet.options[i].length, &len);

        *p++ = (0xFF & (delta << 4 | len));
        if (delta == 13) {
            *p++ = (optdelta - 13);
            packetSize++;
        } else if (delta == 14) {
            *p++ = ((optdelta - 269) >> 8);
            *p++ = (0xFF & (optdelta - 269));
            packetSize+=2;
        } if (len == 13) {
            *p++ = (packet.options[i].length - 13);
            packetSize++;
        } else if (len == 14) {
            *p++ = (packet.options[i].length >> 8);
            *p++ = (0xFF & (packet.options[i].length - 269));
            packetSize+=2;
        }

        memcpy(p, packet.options[i].buffer, packet.options[i].length);
        p += packet.options[i].length;
        packetSize += packet.options[i].length + 1;
        running_delta = packet.options[i].number;
    }

    // make payload
    if (packet.payloadlen > 0) {
        if ((packetSize + 1 + packet.payloadlen) >= BUF_MAX_SIZE) {
            return -1;
        }
        *p++ = 0xFF;
        memcpy(p, packet.payload, packet.payloadlen);
        packetSize += 1 + packet.payloadlen;
    }
    return packetSize;
}

bool Coap::parsePackets(uint8_t *buffer, size_t size, CoapPacket &Packet) {
    if (size < COAP_HEADER_SIZE || (((buffer[0] & 0xC0) >> 6) != 1)) {
        return false;
    }
    CoapPacket packet;
    packet.type = (buffer[0] & 0x30) >> 4;
    packet.tokenlen = buffer[0] & 0x0F;
    packet.code = buffer[1];
    packet.messageid = 0xFF00 & (buffer[2] << 8);
    packet.messageid |= 0x00FF & buffer[3];

    if (packet.tokenlen == 0)  packet.token = NULL;
    else if (packet.tokenlen <= 8)  packet.token = buffer + 4;
    else {
        return false;
    }

    packet.payload = NULL;
    packet.payloadlen = 0;
            
    if (COAP_HEADER_SIZE + packet.tokenlen < size) {
        int optionIndex = 0;
        uint16_t delta = 0;
        uint8_t *end = buffer + size;
        uint8_t *p = buffer + COAP_HEADER_SIZE + packet.tokenlen;
        while (optionIndex < MAX_OPTION_NUM && *p != 0xFF && p < end) {
            if (0 != parseOption(&packet.options[optionIndex], &delta, &p, end - p))
                return false;
            optionIndex++;
        }
        packet.optionnum = optionIndex;

        if (p + 1 < end && *p == 0xFF) {
            packet.payload = p + 1;
            packet.payloadlen = end - (p + 1);
        }
        else {
            packet.payload = NULL;
            packet.payloadlen = 0;
        }
    }
    Packet = packet;
    return true;
}

bool Coap::createMsg(IPAddress &ip, uint16_t port, CoapPacket &Packet, const char *url, COAP_TYPE type, COAP_METHOD method, uint8_t *token, uint8_t tokenlen, uint8_t *payload, uint32_t payloadlen, COAP_CONTENT_TYPE payloadtype){
    // fill packet
    Packet.ip = &ip;

    Packet.type = type;
    Packet.code = method;
    Packet.token = token;
    Packet.tokenlen = tokenlen;
    Packet.payload = payload;
    Packet.payloadlen = payloadlen;
    Packet.optionnum = 0;
#ifdef ESP32
    Packet.messageid = esp_random();
#else
    Packet.messageid = rand();
#endif

    String ipaddress = Packet.ip->toString();
    Packet.AddOption(COAP_URI_HOST, (uint8_t *)ipaddress.c_str(), ipaddress.length());

    Packet.port = lowByte(port)<<8 | highByte(port);
    Packet.AddOption(COAP_URI_PORT, (uint8_t *)&Packet.port, 2);

    
    if(method!=COAP_EMPTY){
        // parse url
        int idx = 0;
        for (int i = 0; i < strlen(url); i++) {
            if (url[i] == '/') {
                Packet.AddOption(COAP_URI_PATH, (uint8_t *)(url + idx), i - idx);
                idx = i + 1;
            }
        }

        if (idx <= strlen(url)) {
            Packet.AddOption(COAP_URI_PATH, (uint8_t *)(url + idx), strlen(url) - idx);
        }
    }

    if(payloadtype != COAP_CONTENT_TYPE::COAP_NONE){
        Packet.AddOption(COAP_CONTENT_FORMAT, (uint8_t*)&payloadtype, 1);
    }
    return true;
}

int Coap::parseOption(CoapOption *option, uint16_t *running_delta, uint8_t **buf, size_t buflen) {
    uint8_t *p = *buf;
    uint8_t headlen = 1;
    uint16_t len, delta;

    if (buflen < headlen) return -1;

    delta = (p[0] & 0xF0) >> 4;
    len = p[0] & 0x0F;

    if (delta == 13) {
        headlen++;
        if (buflen < headlen) return -1;
        delta = p[1] + 13;
        p++;
    }
    else if (delta == 14) {
        headlen += 2;
        if (buflen < headlen) return -1;
        delta = ((p[1] << 8) | p[2]) + 269;
        p += 2;
    }
    else if (delta == 15) return -1;

    if (len == 13) {
        headlen++;
        if (buflen < headlen) return -1;
        len = p[1] + 13;
        p++;
    }
    else if (len == 14) {
        headlen += 2;
        if (buflen < headlen) return -1;
        len = ((p[1] << 8) | p[2]) + 269;
        p += 2;
    }
    else if (len == 15)
        return -1;

    if ((p + 1 + len) >(*buf + buflen))  return -1;
    option->number = delta + *running_delta;
    option->buffer = p + 1;
    option->length = len;
    *buf = p + 1 + len;
    *running_delta += delta;

    return 0;
}

#endif