#include <M5Stack.h>
#include <Ticker.h>
#include "espnow-sta.h"
#include "imu.h"

#define BACKGROUND_COLOR BLACK
#define FOREGROUND_COLOR WHITE

EspNowSta espnow;
int choice;
int lastPeerCount;
int messageCount;
Ticker broadcastTicker;
Ticker ttlTicker;
Ticker dataTicker;
IMU imu;

void sendData() {
    imu.getData();
    int16_t roll = imu.sendRoll();
    int16_t pitch = imu.sendPitch();
    char str[7];
    sprintf(str, "%d, %d", pitch, roll);
    espnow.multicastSendData(str, 7);
}

void setLcd() {
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.clear(BACKGROUND_COLOR);
    M5.Lcd.setTextColor(FOREGROUND_COLOR);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print("espnow sta: ");
    M5.Lcd.println(WiFi.macAddress());
}

void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
    const uint8_t identificationByte = 0xaa;
    if (data_len == 3 && data[0] == 0xaa && data[1] == 0x66 && data[2] == identificationByte) {
        if (lastPeerCount < espnow.peerlist.count) {
            lastPeerCount = espnow.peerlist.count;
            espnow.ackPeer(const_cast<uint8_t*>(mac_addr));
            printPeerList();
        }
    } else if (data_len == 3 && data[0] == 0xaa && data[1] == 0x77 && data[2] == identificationByte) {
        printPeerList();
    } else {
        messageCount++;
        printPeerList();
        M5.Lcd.setCursor(0, 180);
        M5.Lcd.printf("Nb msgs: %d\n%02x:%02x:%02x:%02x:%02x:%02x> %s\r\n", messageCount, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], data);
        Serial.printf("< | Nb msgs: %d\n%02x:%02x:%02x:%02x:%02x:%02x> %s\r\n", messageCount, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], data);
    }
}

void broadcast() {
    espnow.broadcast();
}

void updateTTL() {
    for (int i = 0; i < espnow.peerlist.count; i++) {
        espnow.peerlist.ttl[i]--;
        Serial.printf("< | ttl[%d]: %d\r\n", i, espnow.peerlist.ttl[i]);
        if (espnow.peerlist.ttl[i] == 0) {
            espnow.removePeer(espnow.peerlist.list[i].peer_addr);
            printPeerList();
        }
    }
}
void setup() {
    M5.begin();
    M5.IMU.Init();
    M5.Power.begin();
    choice = 0;
    lastPeerCount = 0;
    messageCount = 0;
    espnow.init();
    espnow.setRecvCallBack(onDataRecv);
    espnow.setBroadcasting(true);

    printPeerList();

    broadcastTicker.attach_ms(3000, broadcast);
    ttlTicker.attach_ms(1000, updateTTL);
    dataTicker.attach_ms(1000, sendData);
}

void printPeerList() {
    setLcd();
    M5.Lcd.setCursor(0, 50);
    M5.Lcd.printf("available peers:\r\n");
    for (int i = 0; i < espnow.peerlist.count; i++) {
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
            espnow.peerlist.list[i].peer_addr[0],
            espnow.peerlist.list[i].peer_addr[1],
            espnow.peerlist.list[i].peer_addr[2],
            espnow.peerlist.list[i].peer_addr[3],
            espnow.peerlist.list[i].peer_addr[4],
            espnow.peerlist.list[i].peer_addr[5]);
        if (espnow.peerlist.isPaired[i]) {
            M5.Lcd.setTextColor(GREEN);
        }
        M5.Lcd.println(macStr);
        M5.Lcd.setTextColor(FOREGROUND_COLOR);
    }
    if (espnow.peerlist.count > 0) {
        M5.Lcd.fillCircle(220, choice * 15 + 74, 3, RED);
    }
}

void loop() {
    M5.update();
    if (M5.BtnA.wasPressed()) {
        imu.getData();
        int16_t roll = imu.sendRoll();
        // convert to number to string
        Serial.println(roll);
        char str[3];
        sprintf(str, "%d", roll);
        espnow.multicastSendData(str, 3);
    }
    if (M5.BtnB.wasPressed()) {
        // send imu data
        imu.getData();
        int16_t pitch = imu.sendPitch();
        // convert to number to string
        Serial.println(pitch);
        char str[3];
        sprintf(str, "%d", pitch);
        espnow.multicastSendData(str, 3);
    }
    if (M5.BtnC.wasPressed()) {
        char *str = "hi";
        espnow.multicastSendData(str, 3);
    }
}
