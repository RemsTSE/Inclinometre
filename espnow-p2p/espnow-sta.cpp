#include "espnow-sta.h"

recvCB EspNowSta::recvCallBack;
sendCB EspNowSta::sendCallBack;
peerList EspNowSta::peerlist;
bool EspNowSta::isBroadcasting;

EspNowSta::EspNowSta() {
    isBroadcasting = false;
    recvCallBack = NULL;
    sendCallBack = NULL;
    memset(peerlist.list, 0, sizeof(peerlist.list));
}

EspNowSta::~EspNowSta() {
}

int8_t EspNowSta::init(void) {
    WiFi.mode(WIFI_STA);
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());

    WiFi.disconnect();
    if (esp_now_init() == ESP_OK) {
        Serial.println("ESPNow Init Success");
    } else {
        Serial.println("ESPNow Init Failed");
        // Retry InitESPNow, add a counte and then restart?
        // InitESPNow();
        // or Simply Restart
        ESP.restart();
    }

    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);

    if (!initBroadcastPeer()) {
        return INIT_ERROR;
    }

    return INIT_SUCCESS;
}

void EspNowSta::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    if (sendCallBack != NULL) {
        sendCallBack(mac_addr, status);
    }
}

void EspNowSta::onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    // Add an identification byte to restrict p2p connection
    const uint8_t identificationByte = 0xaa;

    if (isBroadcasting) {
        if (data_len == 3 && data[0] == 0xaa && data[1] == 0x77 && data[2] == identificationByte) {
            Serial.printf("recv peer confirm: ");
            Serial.println(macStr);
            if (ensurePeer(mac_addr) == ADD_PEER_SUCCESS) {
                addToList(mac_addr, true);
            } 
        }
    }

    if ( data_len == 3 &&data[0] == 0xaa && data[1] == 0x66 && data[2] == identificationByte) {
        Serial.printf("recv peer broadcast: ");
        Serial.println(macStr);
        if (peerIndex(mac_addr, false) == -1) {
            addToList(mac_addr, false);
        }
    }

    Serial.printf("recv data from %s: ", macStr);
    Serial.printf("len: %d", data_len);
    for (int i = 0; i < data_len; i++) {
        Serial.printf(" %02x", data[i]);
    }
    Serial.println();

    if (recvCallBack != NULL) {
        recvCallBack(mac_addr, data, data_len);
    }
}

bool EspNowSta::addrEql(const uint8_t *addr1, const uint8_t *addr2) {
    for (int i = 0; i < 6; i++) {
        if (addr1[i] != addr2[i]) {
            return false;
        }
    }
    return true;
}

int EspNowSta::peerIndex(const uint8_t *peer_addr, bool createIfNotFound) {
    int i;
    for (i = 0; i < peerlist.count; i++) {
        if (addrEql(peer_addr, peerlist.list[i].peer_addr)) {
            return i;
        }
    }
    if (!createIfNotFound) {
        return -1;
    }
    for (i = 0; i < 6; i++) {
        peerlist.list[peerlist.count].peer_addr[i] = peer_addr[i];
    }
    peerlist.count++;
    return peerlist.count - 1;
}

void EspNowSta::addToList(const uint8_t *peer_addr, bool isPaired) {
    int index = peerIndex(peer_addr, true);
    peerlist.isPaired[index] = isPaired;
}

int8_t EspNowSta::ensurePeer(const uint8_t *peer_addr) {
    Serial.print("Peer Status: ");
    // check if the peer exists
    if (esp_now_is_peer_exist(peer_addr)) {
        // Peer already paired.
        Serial.println("Already Paired");
        return ADD_PEER_NOP;
    } else {
        esp_now_peer_info_t peer;
        memset(&peer, 0, sizeof(peer));
        for (int i = 0; i < 6; ++i) {
            peer.peer_addr[i] = peer_addr[i];
        }
        peer.channel = CHANNEL;  // pick a channel
        peer.encrypt = 0;        // no encryption
        peer.ifidx = WIFI_IF_STA;
        // Peer not paired, attempt pair
        esp_err_t addStatus = esp_now_add_peer(&peer);
        if (addStatus == ESP_OK) {
            // Pair success
            Serial.println("Pair success");
            return ADD_PEER_SUCCESS;
        } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
            // How did we get so far!!
            Serial.println("ESPNOW Not Init");
            return ADD_PEER_ERROR;
        } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
            Serial.println("Invalid Argument");
            return ADD_PEER_ERROR;
        } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
            Serial.println("Peer list full");
            return ADD_PEER_ERROR;
        } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
            Serial.println("Out of memory");
            return ADD_PEER_ERROR;
        } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
            Serial.println("Peer Exists");
            return ADD_PEER_SUCCESS;
        } else {
            Serial.println("Not sure what happened");
            return ADD_PEER_ERROR;
        }
    }
}

bool EspNowSta::initBroadcastPeer() {
    const uint8_t peer_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    int8_t res = ensurePeer(peer_addr);

    return res != ADD_PEER_ERROR;
}

void EspNowSta::sendData(uint8_t *peer_addr, void *buf, int len) {
    esp_now_send(peer_addr, (uint8_t *) buf, len);
}

void EspNowSta::broadcast() {
    uint8_t peer_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    const uint8_t identificationByte = 0xaa;
    uint8_t req[] = {0xaa, 0x66, identificationByte};
    sendData(peer_addr, req, sizeof(req));
}

void EspNowSta::multicastSendData(void *buf, int len) {
    for (int i = 0; i < peerlist.count; i++) {
        if (peerlist.isPaired[i]) {
            sendData(peerlist.list[i].peer_addr, buf, len);
        }
    }
}

void EspNowSta::ackPeer(uint8_t *peer_addr) {
    int8_t res = ensurePeer(peer_addr);
    if (res == ADD_PEER_SUCCESS) {
        addToList(peer_addr, true);
    }
    if (res != ADD_PEER_ERROR) {
        const uint8_t identificationByte = 0xaa;
        uint8_t ackdata[] = {0xaa, 0x77 ,identificationByte};
        Serial.print("macaddress");
        for (int i = 0; i < 6; i++) {
            Serial.printf("%x:", peer_addr[i]);
        }
        Serial.println();
        sendData(peer_addr, ackdata, sizeof(ackdata));
    }
}
