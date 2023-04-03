#include <M5Stack.h>
#include <Ticker.h>
#include "espnow-sta.h"
#include "imu.h"

#define BACKGROUND_COLOR BLACK
#define FOREGROUND_COLOR WHITE
#define TOLERANCE 15

#define  PEER_TTL 5000 // Time in ms a peer can be offline before being removed

EspNowSta espnow;
uint8_t lastPeerCount;
uint16_t messageCount;
Ticker broadcastTicker;
Ticker dataTicker;
Ticker removeOfflinePeersTicker;
IMU imu;

void removePeer(uint8_t index)
{
    if (index < espnow.peerlist.count)
    {
        espnow.peerlist.count--;
        lastPeerCount--;
        for (uint8_t i = index; i < espnow.peerlist.count; i++)
        {
            espnow.peerlist.list[i] = espnow.peerlist.list[i + 1];
            espnow.peerlist.isPaired[i] = espnow.peerlist.isPaired[i + 1];
            espnow.peerlist.pitch[i] = espnow.peerlist.pitch[i + 1];
            espnow.peerlist.roll[i] = espnow.peerlist.roll[i + 1];
            espnow.peerlist.lastSeen[i] = espnow.peerlist.lastSeen[i + 1];
        }
    }
    M5.Lcd.clear();
    printPeerList();
    esp_now_del_peer(espnow.peerlist.list[index].peer_addr);
}

void removeOfflinePeers()
{
    uint32_t currentTime = millis();
    for (uint8_t i = 0; i < espnow.peerlist.count; i++)
    {
        if (currentTime - espnow.peerlist.lastSeen[i] > PEER_TTL)
        {
            removePeer(i);
            i--;
        }
    }
}

void sendData()
{
    imu.getData();
    int16_t pitch = imu.getPitch();
    int16_t roll = imu.getRoll();

    uint8_t identifier = 0xaa;
    uint8_t data[5] = {identifier, (uint8_t)(roll >> 8), (uint8_t)roll, (uint8_t)(pitch >> 8), (uint8_t)pitch};
    espnow.multicastSendData(data, sizeof(data));
}

void setLcd()
{
    M5.Lcd.setTextColor(FOREGROUND_COLOR, DARKCYAN);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf(" Device %21s", "Pitch Roll\n\r\n");
    M5.Lcd.setTextColor(FOREGROUND_COLOR, BACKGROUND_COLOR);
    M5.Lcd.print(WiFi.macAddress());
    imu.getData();
    M5.Lcd.printf("% 4d % 4d\r\n\n", imu.getPitch(), imu.getRoll());
}

void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
    // handle ack, broadcast, and multicast that received from peer
    bool isBroadcastData = (data_len == 3 && data[0] == 0xaa && data[1] == 0x66 && data[2] == 0xbb);
    bool isAckData = (data_len == 3 && data[0] == 0xaa && data[1] == 0x77 && data[2] == 0xbb);
    bool isMulticastData = (data_len == 5 && data[0] == 0xaa);

    if (isBroadcastData)
    {
        if (lastPeerCount < espnow.peerlist.count)
        {
            lastPeerCount = espnow.peerlist.count;
            espnow.ackPeer(const_cast<uint8_t *>(mac_addr));
            printPeerList();
        }
    }
    else if (isAckData)
    {
        printPeerList();
    }
    else if (isMulticastData)
    {
        messageCount++;
        int16_t roll = (data[1] << 8) | data[2];
        int16_t pitch = (data[3] << 8) | data[4];
        uint8_t peerIdx = espnow.peerIndex(const_cast<uint8_t *>(mac_addr), false);
        if (peerIdx == -1)
            return;
        espnow.peerlist.pitch[peerIdx] = pitch;
        espnow.peerlist.roll[peerIdx] = roll;
        espnow.peerlist.lastSeen[peerIdx] = millis();

        printPeerList();
        M5.Lcd.setCursor(0, 200);
        M5.Lcd.printf("Nb msgs: %d\r\n", messageCount);
    }
}

void broadcast()
{
    espnow.broadcast();
}

void setup()
{
    M5.begin();
    M5.IMU.Init();
    M5.Power.begin();
    M5.Lcd.fillScreen(BACKGROUND_COLOR);
    M5.Lcd.setTextSize(2);
    lastPeerCount = 0;
    messageCount = 0;
    espnow.init();
    espnow.setRecvCallBack(onDataRecv);
    espnow.setBroadcasting(true);

    printPeerList();

    broadcastTicker.attach_ms(3000, broadcast);
    dataTicker.attach_ms(1000, sendData);
    removeOfflinePeersTicker.attach_ms(5000, removeOfflinePeers);
}

void printPeerList()
{
    setLcd();
    M5.Lcd.setTextColor(FOREGROUND_COLOR, DARKCYAN);
    M5.Lcd.printf("Peers:\r\n\n");
    M5.Lcd.setTextColor(FOREGROUND_COLOR, BACKGROUND_COLOR);
    for (int i = 0; i < espnow.peerlist.count; i++)
    {
        char macStr[18];
        if (espnow.peerlist.isPaired[i])
            M5.Lcd.setTextColor(GREEN, BACKGROUND_COLOR);
        M5.Lcd.printf("%02x:%02x:%02x:%02x:%02x:%02x",
                      espnow.peerlist.list[i].peer_addr[0],
                      espnow.peerlist.list[i].peer_addr[1],
                      espnow.peerlist.list[i].peer_addr[2],
                      espnow.peerlist.list[i].peer_addr[3],
                      espnow.peerlist.list[i].peer_addr[4],
                      espnow.peerlist.list[i].peer_addr[5]);
        imu.getData();
        uint16_t pitchDiff = abs(espnow.peerlist.pitch[i] - imu.getPitch());
        uint16_t rollDiff = abs(espnow.peerlist.roll[i] - imu.getRoll());
        Serial.printf("pitchDiff: %d, rollDiff: %d\r\n", pitchDiff, rollDiff);
        // change color if the difference is too big
        if (pitchDiff > TOLERANCE)
            M5.Lcd.setTextColor(RED, BACKGROUND_COLOR);
        else
            M5.Lcd.setTextColor(GREEN, BACKGROUND_COLOR);
        M5.Lcd.printf("% 4d", espnow.peerlist.pitch[i]);

        if (rollDiff > TOLERANCE)
            M5.Lcd.setTextColor(RED, BACKGROUND_COLOR);
        else
            M5.Lcd.setTextColor(GREEN, BACKGROUND_COLOR);
        M5.Lcd.printf(" % 4d\r\n", espnow.peerlist.roll[i]);

        if (TOLERANCE < pitchDiff && TOLERANCE < rollDiff)
        {
            // generer une note
            M5.Speaker.tone(1000, 100);
        }
        M5.Lcd.setTextColor(FOREGROUND_COLOR, BACKGROUND_COLOR);
    }
}

void loop()
{
    M5.update();
}
