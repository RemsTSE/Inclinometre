#include <M5Stack.h>
#include <Ticker.h>
#include "espnow-sta.h"

#define BACKGROUND_COLOR BLACK
#define FOREGROUND_COLOR WHITE

EspNowSta espnow;
int choice;
int lastPeerCount;
int messageCount;
Ticker broadcastTicker;

void setLcd() {
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.clear(BACKGROUND_COLOR);
    M5.Lcd.setTextColor(FOREGROUND_COLOR);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print("espnow sta: ");
    M5.Lcd.println(WiFi.macAddress());
}

void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
    const uint8_t identificationByte = 0xAA;
    if (data_len == 2 && data[0] == 0xaa && data[1] == 0x66) {
        if (lastPeerCount < espnow.peerlist.count) {
            lastPeerCount = espnow.peerlist.count;
            printPeerList();
        }
    } else if (data_len == 3 && data[0] == 0xaa && data[1] == 0x77 && data[2] == identificationByte) {
        printPeerList();
    } else {
        messageCount++;
        printPeerList();
        M5.Lcd.setCursor(0, 180);
      	M5.Lcd.printf("Nb msgs: %d\n%02x:%02x:%02x:%02x:%02x:%02x> %s\r\n", messageCount, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], data);
    }
}

void broadcast() {
    espnow.broadcast();
}

void setup() {
    M5.begin();
    choice = 0;
    lastPeerCount = 0;
    messageCount = 0;
    espnow.init();
    espnow.setRecvCallBack(onDataRecv);
    espnow.setBroadcasting(true);

    printPeerList();

    broadcastTicker.attach_ms(1000, broadcast);
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
        if (espnow.peerlist.count > 0) {
            choice++;
            choice = choice % espnow.peerlist.count;
            printPeerList();
        }
    }
    if (M5.BtnB.wasPressed()) {
        if (espnow.peerlist.count > 0) {
            espnow.ackPeer(espnow.peerlist.list[choice].peer_addr);
            printPeerList();
        }
    }
    if (M5.BtnC.wasPressed()) {
        char *str = "hi";
        espnow.multicastSendData(str, 3);
    }
}
