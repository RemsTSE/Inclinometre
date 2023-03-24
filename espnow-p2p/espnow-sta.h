#ifndef __ESPNOW_STA_H
#define __ESPNOW_STA_H

#include <esp_now.h>
#include <WiFi.h>

#define CHANNEL          1

#define ADD_PEER_SUCCESS 1
#define ADD_PEER_ERROR -1
#define ADD_PEER_NOP 0

#define INIT_SUCCESS 1
#define INIT_ERROR -1

typedef void (*recvCB)(const uint8_t *mac_addr, const uint8_t *data, int data_len);
typedef void (*sendCB)(const uint8_t *, esp_now_send_status_t);

struct peerList {
    esp_now_peer_info_t list[2];
    char isPaired[2];
    uint8_t count = 0;
    uint8_t ttl[2];
};

class EspNowSta {
public:
    EspNowSta();
    ~EspNowSta();
    void setRecvCallBack(recvCB cb) {
        recvCallBack = cb;
    };
    void setSendCallBack(sendCB cb) {
        sendCallBack = cb;
    };
    void setBroadcasting(bool value) {
        isBroadcasting = value;
    }
    int8_t init();
    void broadcast();
    void updateTTL();
    void ackPeer(uint8_t *peer_addr);
    static void sendData(uint8_t *peer_addr, void *buff, int len);
    void multicastSendData(void *buf, int len);
    static peerList peerlist;
    void removePeer(uint8_t *peer_addr);

protected:
    static bool isBroadcasting;
    static recvCB recvCallBack;
    static sendCB sendCallBack;
    static bool addrEql(const uint8_t *addr1, const uint8_t *addr2);
    static int peerIndex(const uint8_t *peer_addr, bool createIfNotFound);
    static void addToList(const uint8_t *peer_addr, bool isPaired);
    static int8_t ensurePeer(const uint8_t *peer_addr);
    static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
    static bool initBroadcastPeer();
};

#endif
