/*
 * This is ESP-RFID-Tool-v2. It's  Einstein2150's fork of the original software from Corey Harding.
 * This software requires the advanced hardware platform of the classic ESP-RFID-Tool, specifically the ESP-RFID-Tool v2 PRO.
 * It is available at esp-rfid.foto-video-it.de
 * The ESP-RFID-Tool v2 PRO features a highly robust DC-DC voltage converter with an input range from 5 V to 30 V.
 * In addition, it integrates a newly developed MOSFET based replay mechanism that ensures reliable and stable modulation of the Wiegand data lines.
 * 
 * ESP-RFID-Tool
 * by Corey Harding of www.Exploit.Agency / www.LegacySecurityGroup.com
 * ESP-RFID-Tool Software is distributed under the MIT License. The license and copyright notice can not be removed and must be distributed alongside all future copies of the software.
 * MIT License
    
    Copyright (c) [2018] [Corey Harding]
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.



    compile-info:
    esp8266-bordmanager version: 2.4.0
    ArduinoJson: 5.13.5
*/
#include "HelpText.h"
#include "License.h"
#include "version.h"
#include "strrev.h"
#include "aba2str.h"
#include "api_server.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ArduinoJson.h> // ArduinoJson library 5.11.0 by Benoit Blanchon https://github.com/bblanchon/ArduinoJson
#include <ESP8266FtpServer.h> // https://github.com/exploitagency/esp8266FTPServer/tree/feature/bbx10_speedup
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include "hexmagic.h"
#include "css.h"

#define DATA0 14 //D5 green
#define DATA1 12 //D6 white
#define DATA0_MOS 16 //D0 for v2-Board MOSFET data0 line green
#define DATA1_MOS 5 //D1 for v2-Board MOSFET data1 line white

#define LED_BUILTIN 2
#define RESTORE_DEFAULTS_PIN 4 //GPIO 4
int jumperState = 0; //For restoring default settings
#include "WiegandNG.h" //https://github.com/jpliew/Wiegand-NG-Multi-Bit-Wiegand-Library-for-Arduino

// Port for web server
ESP8266WebServer server(80);
ESP8266WebServer httpServer(1337);
ESP8266HTTPUpdateServer httpUpdater;
FtpServer ftpSrv;
const byte DNS_PORT = 53;
DNSServer dnsServer;

HTTPClient http;

const char* update_path = "/update";
int accesspointmode;
char ssid[32];
char password[64];
int channel;
int hidden;
char local_IPstr[16];
char gatewaystr[16];
char subnetstr[16];
char update_username[32];
char update_password[64];
char ftp_username[32];
char ftp_password[64];
int ftpenabled;
int ledenabled;
char logname[31];
unsigned int bufferlength;
unsigned int rxpacketgap;
int txdelayus;
int txdelayms;
int safemode;

int dos=0;
int TXstatus=0;
String pinHTML;

String newUIDHex = "";
String newReplayBinary = "";
String newBitstream = "";
unsigned int newBitCount = 0;
String newUIDFormat = "";

#include "pinSEND.h"

String dataCONVERSION="";

String mapKeyToBits(char key, int bits) {
    if (bits == 4) {
        switch (key) {
            case '0': return "0000";
            case '1': return "0001";
            case '2': return "0010";
            case '3': return "0011";
            case '4': return "0100";
            case '5': return "0101";
            case '6': return "0110";
            case '7': return "0111";
            case '8': return "1000";
            case '9': return "1001";
            case '*': case 'A': return "1010";
            case '#': case 'B': return "1011";
            case 'C': return "1100";
            case 'D': return "1101";
            case 'E': return "1110";
            case 'F': return "1111";
        }
    }
    if (bits == 8) {
        switch (key) {
            case '0': return "11110000";
            case '1': return "11100001";
            case '2': return "11010010";
            case '3': return "11000011";
            case '4': return "10110100";
            case '5': return "10100101";
            case '6': return "10010110";
            case '7': return "10000111";
            case '8': return "01111000";
            case '9': return "01101001";
            case '*': case 'A': return "01011010";
            case '#': case 'B': return "01001011";
            case 'C': return "00111100";
            case 'D': return "00101101";
            case 'E': return "00011110";
            case 'F': return "00001111";
        }
    }
    return "";
}

String mapBitsToKey(uint8_t bits, uint8_t rawByte) {
    switch (bits) {
        // 0
        case 0b00000000: return "0";   // 4-bit
        case 0b11110000: return "0";   // 8-bit
        // 1
        case 0b00000001: return "1";
        case 0b11100001: return "1";
        // 2
        case 0b00000010: return "2";
        case 0b11010010: return "2";
        // 3
        case 0b00000011: return "3";
        case 0b11000011: return "3";
        // 4
        case 0b00000100: return "4";
        case 0b10110100: return "4";
        // 5
        case 0b00000101: return "5";
        case 0b10100101: return "5";
        // 6
        case 0b00000110: return "6";
        case 0b10010110: return "6";
        // 7
        case 0b00000111: return "7";
        case 0b10000111: return "7";
        // 8
        case 0b00001000: return "8";
        case 0b01111000: return "8";
        // 9
        case 0b00001001: return "9";
        case 0b01101001: return "9";
        // *
        case 0b00001010: return "*";
        case 0b01011010: return "*";
        // #
        case 0b00001011: return "# or Enter";
        case 0b01001011: return "# or Enter";
        // F1
        case 0b00001100: return "F1";
        case 0b00111100: return "F1";
        // F2
        case 0b00001101: return "F2";
        case 0b00101101: return "F2";
        // F3
        case 0b00001110: return "F3";
        case 0b00011110: return "F3";
        // F4
        case 0b00001111: return "F4"; // beide identisch → aber korrekt, da 8-bit Code = 00001111
    }
    
    uint8_t key = rawByte & 0x0F;

    switch (key) {
        case 0x0: return "0";
        case 0x1: return "1";
        case 0x2: return "2";
        case 0x3: return "3";
        case 0x4: return "4";
        case 0x5: return "5";
        case 0x6: return "6";
        case 0x7: return "7";
        case 0x8: return "8";
        case 0x9: return "9";
        case 0xA: return "*";
        case 0xB: return "#";
        case 0xC: return "F1";
        case 0xD: return "F2";
        case 0xE: return "F3";
        case 0xF: return "F4";
    }

    return "?";
}

WiegandNG wg;

void LogWiegand(WiegandNG &tempwg) {
  volatile unsigned char *buffer=tempwg.getRawData();
  unsigned int bufferSize = tempwg.getBufferSize();
  unsigned int countedBits = tempwg.getBitCounted();

  unsigned int countedBytes = (countedBits/8);
  if ((countedBits % 8)>0) countedBytes++;
  //unsigned int bitsUsed = countedBytes * 8;

  bool binChunk2exists=false;
  volatile unsigned long cardChunk1 = 0;
  volatile unsigned long cardChunk2 = 0;
  String cardChunkConcat = ""; //new
  volatile unsigned long binChunk2 = 0;
  volatile unsigned long binChunk1 = 0;
  String binChunk3="";
  bool unknown=false;
  binChunk2exists=false;
  int binChunk2len=0;
  int j=0;

  uint8_t rawByte = binChunk1;
  
  for (unsigned int i=bufferSize-countedBytes; i< bufferSize;i++) {
    unsigned char bufByte=buffer[i];
    for(int x=0; x<8;x++) {
      if ( (((bufferSize-i) *8)-x) <= countedBits) {
        j++;
        if((bufByte & 0x80)) {  //write 1
          if(j<23) {
            binChunk1 = binChunk1 << 1;
            binChunk1 |= 1;
          }
          else if(j<=52) {
            binChunk2exists=true;
            binChunk2len++;
            binChunk2 = binChunk2 << 1;
            binChunk2 |= 1;
          }
          else if(j>52){
            binChunk3=binChunk3+"1";
          }
        }
        else {  //write 0
          if(j<23) {
            binChunk1 = binChunk1 << 1;
          }
          else if(j<=52){
            binChunk2exists=true;
            binChunk2len++;
            binChunk2 = binChunk2 << 1;
          }
          else if(j>52){
            binChunk3=binChunk3+"0";
          }
        }
      }
      bufByte<<=1;
    }
  }
  j=0;

  switch (countedBits) {  //Add the preamble to known cards
    case 26:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 2){
          bitWrite(cardChunk1, i, 1); // Write preamble 1's to the 13th and 2nd bits
        }
        else if(i > 2) {
          bitWrite(cardChunk1, i, 0); // Write preamble 0's to all other bits above 1
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 20)); // Write remaining bits to cardChunk1 from binChunk1
        }
        if(i < 20) {
          bitWrite(cardChunk2, i + 4, bitRead(binChunk1, i)); // Write the remaining bits of binChunk1 to cardChunk2
        }
        if(i < 4) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i)); // Write the remaining bit of cardChunk2 with binChunk2 bits
        }
      }
      break;
    case 27:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 3){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 3) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 19));
        }
        if(i < 19) {
          bitWrite(cardChunk2, i + 5, bitRead(binChunk1, i));
        }
        if(i < 5) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 28:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 4){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 4) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 18));
        }
        if(i < 18) {
          bitWrite(cardChunk2, i + 6, bitRead(binChunk1, i));
        }
        if(i < 6) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 29:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 5){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 5) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 17));
        }
        if(i < 17) {
          bitWrite(cardChunk2, i + 7, bitRead(binChunk1, i));
        }
        if(i < 7) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 30:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 6){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 6) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 16));
        }
        if(i < 16) {
          bitWrite(cardChunk2, i + 8, bitRead(binChunk1, i));
        }
        if(i < 8) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 31:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 7){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 7) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 15));
        }
        if(i < 15) {
          bitWrite(cardChunk2, i + 9, bitRead(binChunk1, i));
        }
        if(i < 9) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 32:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 8){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 8) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 14));
        }
        if(i < 14) {
          bitWrite(cardChunk2, i + 10, bitRead(binChunk1, i));
        }
        if(i < 10) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 33:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 9){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 9) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 13));
        }
        if(i < 13) {
          bitWrite(cardChunk2, i + 11, bitRead(binChunk1, i));
        }
        if(i < 11) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 34:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 10){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 10) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 12));
        }
        if(i < 12) {
          bitWrite(cardChunk2, i + 12, bitRead(binChunk1, i));
        }
        if(i < 12) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 35:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 11){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 11) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 11));
        }
        if(i < 11) {
          bitWrite(cardChunk2, i + 13, bitRead(binChunk1, i));
        }
        if(i < 13) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 36:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 12){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 12) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 10));
        }
        if(i < 10) {
          bitWrite(cardChunk2, i + 14, bitRead(binChunk1, i));
        }
        if(i < 14) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 37:
      for(int i = 19; i >= 0; i--) {
        if(i == 13){
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 9));
        }
        if(i < 9) {
          bitWrite(cardChunk2, i + 15, bitRead(binChunk1, i));
        }
        if(i < 15) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    default:  //unknown card
      unknown=true;
      //String binChunk3 is like cardChunk0
      cardChunk1=binChunk2;
      cardChunk2=binChunk1;
      break;
  }

  File f = SPIFFS.open("/"+String(logname), "a"); //Open the log in append mode to store capture
  int preambleLen;
  if (unknown==true && countedBits!=4 && countedBits!=8 && countedBits!=248) {
    f.println();
    f.print(F("Unknown "));
    preambleLen=0;
  }
  else {
    preambleLen=(44-countedBits);
  }
  f.println();
  f.print(String()+countedBits+F(" bit card,"));

  if (countedBits==4||countedBits==8) {
    f.print(F("Possible keypad entry,"));
  }

  if (countedBits==248) {
    f.print(F("possible magstripe card,"));
  }
  String magstripe="";

  if (unknown!=true) {
    f.print(String()+preambleLen+F(" bit preamble,"));
  }
  
  f.print(F("Binary:"));

  //f.print(" ");  //debug line
  if (binChunk2exists==true && unknown!=true) {
    for(int i = (((countedBits+preambleLen)-countedBits)+(countedBits-24)); i--;) {
      if (i==((((countedBits+preambleLen)-countedBits)+(countedBits-24))-preambleLen-1) && unknown!=true) {
        f.print(" ");
      }
      f.print(bitRead(cardChunk1, i));
      if(i == 0){
        break;
      }
    }
  }
  
  if ((countedBits>=24) && unknown!=true) {
    for(int i = 24; i--;) {
      f.print(bitRead(cardChunk2, i));
      if(i == 0){
        break;
      }
    }
  }
  else if ((countedBits>=23) && unknown==true) {
    int i;
    if (countedBits>=52) {
      i=22;
    }
    else {
      i =(countedBits-binChunk2len);
    }
    for(i; i--;) {
      f.print(bitRead(binChunk1, i));
      if (countedBits==248) {
        magstripe+=bitRead(binChunk1, i);
      }
      if(i == 0){
        break;
      }
    }
  }
  else {
    for(int i = countedBits; i--;) {
      f.print(bitRead(binChunk1, i));
      if(i == 0){
        break;
      }
    }
  }

  if (binChunk2exists==true && unknown==true) {
    int i;
    if (countedBits>=52) {
      i=30;
    }
    else {
      i=(binChunk2len);
    }
    for(i; i--;) {
      f.print(bitRead(binChunk2, i));
      if (countedBits==248) {
        magstripe+=bitRead(binChunk2, i);
      }
      if(i == 0){
        break;
      }
    }
  }

  if (countedBits>52) {
    f.print(binChunk3);
    if (countedBits==248) {
        magstripe+=binChunk3;
    }
  }

  if (countedBits<=52 && unknown!=true) {
    //f.print(",HEX:");
    //if (binChunk2exists==true) {
    //  f.print(cardChunk1, HEX);
    //  Serial.print("cardChunk1+2: ");
    //  Serial.print(cardChunk1, HEX);
    //}
    //f.print(" "); //debug line
    //f.println(cardChunk2, HEX);
    //Serial.println(cardChunk2, HEX);
  
    cardChunkConcat = String(cardChunk1, HEX) + String(cardChunk2, HEX); //concat the two hex-chunks to one bin
    //Serial.print("cardChunkConcatString: ");
    //Serial.println(cardChunkConcat);

    cardChunkConcat = hexToBinary(cardChunkConcat);
    //Serial.print("cardChunkConcatString2BIN: ");
    //Serial.println(cardChunkConcat);

    String cardBinReplay = cardChunkConcat.substring(6, cardChunkConcat.length());

    cardChunkConcat = cardChunkConcat.substring(7, cardChunkConcat.length() - 1);
    //Serial.print("cardChunkConcatString2BIN-Cutted: ");
    //Serial.println(cardChunkConcat);

    cardChunkConcat = binaryToHex(cardChunkConcat);
    //Serial.print("cardChunkConcat-reHEXed: ");
    //Serial.println(cardChunkConcat);
    //f.print("--> Clean-HEX:");
    //f.print(cardChunkConcat);
    
    cardChunkConcat = reverseHex(cardChunkConcat);
    //Serial.print("Card-UID: ");
    //Serial.println(cardChunkConcat);
    //Serial.println();
    //f.print(", Card-UID:");
    //f.print(cardChunkConcat);
    //f.print("<button onclick=\"window.location.href='/api/tx/bin?binary=");
    //f.print(cardBinReplay);
    //f.print("&pulsewidth=40&interval=2000&wait=100000&prettify=1'\" style=\"width: 200px; height: 25px;\" >Replay</button>");
    //f.println("");

    //RAW-WIEGAND and UID-Parsing
    f.print("<br>");
    f.println("<br>Data-Analysis:<br>");

    f.print("RAW Bits: ");
    f.print(newBitstream);
    f.print("<br>");

    f.print("RAW UID: ");
    f.print(newUIDHex);
    f.print("<br>");

    f.print("RAW Format: ");
    f.print(newUIDFormat);
    f.print("<br>");
    
    f.print("<button onclick=\"window.location.href='/api/txinstant/bin?binary=");
    //f.print(cardBinReplay);
    f.print(newBitstream);
    f.print("&pulsewidth=40&interval=2000&wait=100000&prettify=1'\" style=\"width: 200px; height: 25px;\" >Replay</button>");
    f.print("<br>");  
  }
  
  else if (countedBits == 4 || countedBits == 8) {
    f.print(",Keypad Code:");
    f.print(mapBitsToKey(binChunk1, binChunk1));

    f.print(",HEX:");
    if (countedBits == 8) {
        char hexCHAR[3];
        sprintf(hexCHAR, "%02X", binChunk1);
        f.println(hexCHAR);

        f.print("<button onclick=\"window.location.href='/api/txinstant/bin?binary=");
        f.print(hexToBinary(hexCHAR));
        f.print("&pulsewidth=40&interval=2000&wait=100000&prettify=1'\" style=\"width: 200px; height: 25px;\" >Replay</button>");
    }
    else if (countedBits == 4) {
        f.println(binChunk1, HEX);

        String keyHex = String(binChunk1, HEX);
        f.print("<button onclick=\"window.location.href='/api/txinstant/bin?binary=");
        f.print(hexToBinary(keyHex));
        f.print("&pulsewidth=40&interval=2000&wait=100000&prettify=1'\" style=\"width: 200px; height: 25px;\" >Replay</button>");
    }
  }
  
  else if (countedBits==248) {
    f.println(",");
  }
  else {
    f.println("");
  }

  if (countedBits==248) {
    int startSentinel=magstripe.indexOf("11010");
    int endSentinel=(magstripe.lastIndexOf("11111")+4);
    int magStart=0;
    int magEnd=1;
    //f.print("<pre>");
  
    f.print(" * Trying \"Forward\" Swipe,");
    magStart=startSentinel;
    magEnd=endSentinel;
    f.println(aba2str(magstripe,magStart,magEnd,"\"Forward\" Swipe"));
    
    f.print(" * Trying \"Reverse\" Swipe,");
    char magchar[249];
    magstripe.toCharArray(magchar,249);
    magstripe=String(strrev(magchar));
    //f.println(String()+"Reverse: "+magstripe);
    magStart=magstripe.indexOf("11010");
    magEnd=(magstripe.lastIndexOf("11111")+4);
    f.println(aba2str(magstripe,magStart,magEnd,"\"Reverse\" Swipe"));
  
    //f.print("</pre>");
    //f.println(String()+F(" * You can verify the data at the following URL: <a target=\"_blank\" href=\"https://www.legacysecuritygroup.com/aba-decode.php?binary=")+magstripe+F("\">https://www.legacysecuritygroup.com/aba-decode.php?binary=")+magstripe+F("</a>"));
  }

//Debug
//  f.print(F("Free heap:"));
//  f.println(ESP.getFreeHeap(),DEC);
  
  unknown=false;
  binChunk3="";
  binChunk2exists=false;
  binChunk1 = 0; binChunk2 = 0;
  cardChunk1 = 0; cardChunk2 = 0;
  binChunk2len=0;

  f.close(); //done
}

#include "api.h"

void settingsPage()
{
  if(!server.authenticate(update_username, update_password))
    return server.requestAuthentication();
  String accesspointmodeyes;
  String accesspointmodeno;
  if (accesspointmode==1){
    accesspointmodeyes=" checked=\"checked\"";
    accesspointmodeno="";
  }
  else {
    accesspointmodeyes="";
    accesspointmodeno=" checked=\"checked\"";
  }
  String ftpenabledyes;
  String ftpenabledno;
  if (ftpenabled==1){
    ftpenabledyes=" checked=\"checked\"";
    ftpenabledno="";
  }
  else {
    ftpenabledyes="";
    ftpenabledno=" checked=\"checked\"";
  }
  String ledenabledyes;
  String ledenabledno;
  if (ledenabled==1){
    ledenabledyes=" checked=\"checked\"";
    ledenabledno="";
  }
  else {
    ledenabledyes="";
    ledenabledno=" checked=\"checked\"";
  }
  String hiddenyes;
  String hiddenno;
  if (hidden==1){
    hiddenyes=" checked=\"checked\"";
    hiddenno="";
  }
  else {
    hiddenyes="";
    hiddenno=" checked=\"checked\"";
  }
  String safemodeyes;
  String safemodeno;
  if (safemode==1){
    safemodeyes=" checked=\"checked\"";
    safemodeno="";
  }
  else {
    safemodeyes="";
    safemodeno=" checked=\"checked\"";
  }
  server.send(200, "text/html", 
  String()+
  F(
    "<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - Settings")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br>"
 "<h1>ESP-RFID-Tool Settings</h1>"
  "<a href=\"/restoredefaults\"><button>Restore Default Configuration</button></a>"
  "<hr>"
  "<FORM action=\"/settings\"  id=\"configuration\" method=\"post\">"
  "<P>"
  "<b>WiFi Configuration:</b><br><br>"
  "<b>Network Type</b><br>"
  )+
  F("Access Point Mode: <INPUT type=\"radio\" name=\"accesspointmode\" value=\"1\"")+accesspointmodeyes+F("><br>"
  "Join Existing Network: <INPUT type=\"radio\" name=\"accesspointmode\" value=\"0\"")+accesspointmodeno+F("><br><br>"
  "<b>Hidden<br></b>"
  "Yes <INPUT type=\"radio\" name=\"hidden\" value=\"1\"")+hiddenyes+F("><br>"
  "No <INPUT type=\"radio\" name=\"hidden\" value=\"0\"")+hiddenno+F("><br><br>"
  "SSID: <input type=\"text\" name=\"ssid\" value=\"")+ssid+F("\" maxlength=\"31\" size=\"31\"><br>"
  "Password: <input type=\"password\" name=\"password\" value=\"")+password+F("\" maxlength=\"64\" size=\"31\"><br>"
  "Channel: <select name=\"channel\" form=\"configuration\"><option value=\"")+channel+"\" selected>"+channel+F("</option><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option></select><br><br>"
  "IP: <input type=\"text\" name=\"local_IPstr\" value=\"")+local_IPstr+F("\" maxlength=\"16\" size=\"31\"><br>"
  "Gateway: <input type=\"text\" name=\"gatewaystr\" value=\"")+gatewaystr+F("\" maxlength=\"16\" size=\"31\"><br>"
  "Subnet: <input type=\"text\" name=\"subnetstr\" value=\"")+subnetstr+F("\" maxlength=\"16\" size=\"31\"><br><br>"
  "<hr>"
  "<b>Web Interface Administration Settings:</b><br><br>"
  "Username: <input type=\"text\" name=\"update_username\" value=\"")+update_username+F("\" maxlength=\"31\" size=\"31\"><br>"
  "Password: <input type=\"password\" name=\"update_password\" value=\"")+update_password+F("\" maxlength=\"64\" size=\"31\"><br><br>"
  "<hr>"
  "<b>FTP Server Settings</b><br>"
  "<small>Changes require a reboot.</small><br>"
  "Enabled <INPUT type=\"radio\" name=\"ftpenabled\" value=\"1\"")+ftpenabledyes+F("><br>"
  "Disabled <INPUT type=\"radio\" name=\"ftpenabled\" value=\"0\"")+ftpenabledno+F("><br>"
  "FTP Username: <input type=\"text\" name=\"ftp_username\" value=\"")+ftp_username+F("\" maxlength=\"31\" size=\"31\"><br>"
  "FTP Password: <input type=\"password\" name=\"ftp_password\" value=\"")+ftp_password+F("\" maxlength=\"64\" size=\"31\"><br><br>"
  "<hr>"
  "<b>Power LED:</b><br>"
  "<small>Changes require a reboot.</small><br>"
  "Enabled <INPUT type=\"radio\" name=\"ledenabled\" value=\"1\"")+ledenabledyes+F("><br>"
  "Disabled <INPUT type=\"radio\" name=\"ledenabled\" value=\"0\"")+ledenabledno+F("><br><br>"
  "<hr>"
  "<b>RFID Capture Log:</b><br>"
  "<small>Useful to change this value to differentiate between facilities during various security assessments.</small><br>"
  "File Name: <input type=\"text\" name=\"logname\" value=\"")+logname+F("\" maxlength=\"30\" size=\"31\"><br>"
  "<hr>"
  "<b>Experimental Settings:</b><br>"
  "<small>Changes require a reboot.</small><br>"
  "<small>Default Buffer Length is 256 bits with an allowed range of 52-4096 bits."
  "<br>Default TX mode timing is 40us Wiegand Data Pulse Width and a 2ms Wiegand Data Interval with an allowed range of 0-1000."
  "<br>Changing these settings may result in unstable performance.</small><br>"
  "Wiegand RX Buffer Length: <input type=\"number\" name=\"bufferlength\" value=\"")+bufferlength+F("\" maxlength=\"30\" size=\"31\" min=\"52\" max=\"4096\"> bit(s)<br>"
  "Wiegand RX Packet Length: <input type=\"number\" name=\"rxpacketgap\" value=\"")+rxpacketgap+F("\" maxlength=\"30\" size=\"31\" min=\"1\" max=\"4096\"> millisecond(s)<br>"
  "TX Wiegand Data Pulse Width: <input type=\"number\" name=\"txdelayus\" value=\"")+txdelayus+F("\" maxlength=\"30\" size=\"31\" min=\"0\" max=\"1000\"> microsecond(s)<br>"
  "TX Wiegand Data Interval: <input type=\"number\" name=\"txdelayms\" value=\"")+txdelayms+F("\" maxlength=\"30\" size=\"31\" min=\"0\" max=\"1000\"> millisecond(s)<br>"
  "<hr>"
  "<b>Safe Mode:</b><br>"
  "<small>Enable to reboot the device after every capture.<br>Disable to avoid missing quick consecutive captures such as keypad entries.</small><br>"
  "Enabled <INPUT type=\"radio\" name=\"safemode\" value=\"1\"")+safemodeyes+F("><br>"
  "Disabled <INPUT type=\"radio\" name=\"safemode\" value=\"0\"")+safemodeno+F("><br><br>"
  "<hr>"
  "<INPUT type=\"radio\" name=\"SETTINGS\" value=\"1\" hidden=\"1\" checked=\"checked\">"
  "<INPUT type=\"submit\" value=\"Apply Settings\">"
  "</FORM>"
  "<br><a href=\"/reboot\"><button>Reboot Device</button></a>"
  "</P>"
  "</body>"
  "</html>"
  )
  );
}

void handleSettings()
{
  if (server.hasArg("SETTINGS")) {
    handleSubmitSettings();
  }
  else {
    settingsPage();
  }
}

void returnFail(String msg)
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}

void handleSubmitSettings()
{
  String SETTINGSvalue;

  if (!server.hasArg("SETTINGS")) return returnFail("BAD ARGS");
  
  SETTINGSvalue = server.arg("SETTINGS");
  accesspointmode = server.arg("accesspointmode").toInt();
  server.arg("ssid").toCharArray(ssid, 32);
  server.arg("password").toCharArray(password, 64);
  channel = server.arg("channel").toInt();
  hidden = server.arg("hidden").toInt();
  server.arg("local_IPstr").toCharArray(local_IPstr, 16);
  server.arg("gatewaystr").toCharArray(gatewaystr, 16);
  server.arg("subnetstr").toCharArray(subnetstr, 16);
  server.arg("update_username").toCharArray(update_username, 32);
  server.arg("update_password").toCharArray(update_password, 64);
  server.arg("ftp_username").toCharArray(ftp_username, 32);
  server.arg("ftp_password").toCharArray(ftp_password, 64);
  ftpenabled = server.arg("ftpenabled").toInt();
  ledenabled = server.arg("ledenabled").toInt();
  server.arg("logname").toCharArray(logname, 31);
  bufferlength = server.arg("bufferlength").toInt();
  rxpacketgap = server.arg("rxpacketgap").toInt();
  txdelayus = server.arg("txdelayus").toInt();
  txdelayms = server.arg("txdelayms").toInt();
  safemode = server.arg("safemode").toInt();
  
  if (SETTINGSvalue == "1") {
    saveConfig();
    server.send(200, "text/html", F("<a href=\"/\"><- BACK TO MAIN-MENU</a><br><br><a href=\"/reboot\"><button>Reboot Device</button></a><br><br>Settings have been saved.<br>Some setting may require manually rebooting before taking effect.<br>If network configuration has changed then be sure to connect to the new network first in order to access the web interface."));
    delay(50);
    loadConfig();
  }
  else if (SETTINGSvalue == "0") {
    settingsPage();
  }
  else {
    returnFail("Bad SETTINGS value");
  }
}

bool loadDefaults() {
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["version"] = version;
  json["accesspointmode"] = "1";
  json["ssid"] = "ESP-RFID-Tool";
  json["password"] = "";
  json["channel"] = "6";
  json["hidden"] = "0";
  json["local_IP"] = "192.168.1.1";
  json["gateway"] = "192.168.1.1";
  json["subnet"] = "255.255.255.0";
  json["update_username"] = "admin";
  json["update_password"] = "rfidtool";
  json["ftp_username"] = "ftp-admin";
  json["ftp_password"] = "rfidtool";
  json["ftpenabled"] = "0";
  json["ledenabled"] = "1";
  json["logname"] = "log.txt";
  json["bufferlength"] = "256";
  json["rxpacketgap"] = "15";
  json["txdelayus"] = "40";
  json["txdelayms"] = "2";
  json["safemode"] = "0";
  File configFile = SPIFFS.open("/esprfidtool.json", "w");
  json.printTo(configFile);
  configFile.close();
  jsonBuffer.clear();
  loadConfig();
  return 1;
}

bool loadConfig() {
  File configFile = SPIFFS.open("/esprfidtool.json", "r");
  if (!configFile) {
    delay(3500);
    loadDefaults();
  }

  size_t size = configFile.size();

  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());
  
  if (!json["version"]) {
    delay(3500);
    loadDefaults();
    ESP.restart();
  }

  //Resets config to factory defaults on an update.
  if (json["version"]!=version) {
    delay(3500);
    loadDefaults();
    ESP.restart();
  }

  strcpy(ssid, (const char*)json["ssid"]);
  strcpy(password, (const char*)json["password"]);
  channel = json["channel"];
  hidden = json["hidden"];
  accesspointmode = json["accesspointmode"];
  strcpy(local_IPstr, (const char*)json["local_IP"]);
  strcpy(gatewaystr, (const char*)json["gateway"]);
  strcpy(subnetstr, (const char*)json["subnet"]);

  strcpy(update_username, (const char*)json["update_username"]);
  strcpy(update_password, (const char*)json["update_password"]);

  strcpy(ftp_username, (const char*)json["ftp_username"]);
  strcpy(ftp_password, (const char*)json["ftp_password"]);
  ftpenabled = json["ftpenabled"];
  ledenabled = json["ledenabled"];
  strcpy(logname, (const char*)json["logname"]);
  bufferlength = json["bufferlength"];
  rxpacketgap = json["rxpacketgap"];
  txdelayus = json["txdelayus"];
  txdelayms = json["txdelayms"];
  safemode = json["safemode"];
 
  IPAddress local_IP;
  local_IP.fromString(local_IPstr);
  IPAddress gateway;
  gateway.fromString(gatewaystr);
  IPAddress subnet;
  subnet.fromString(subnetstr);

/*
  Serial.println(accesspointmode);
  Serial.println(ssid);
  Serial.println(password);
  Serial.println(channel);
  Serial.println(hidden);
  Serial.println(local_IP);
  Serial.println(gateway);
  Serial.println(subnet);
*/
  WiFi.persistent(false);
  //ESP.eraseConfig();
// Determine if set to Access point mode
  if (accesspointmode == 1) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);

//    Serial.print("Starting Access Point ... ");
//    Serial.println(WiFi.softAP(ssid, password, channel, hidden) ? "Success" : "Failed!");
    WiFi.softAP(ssid, password, channel, hidden);

//    Serial.print("Setting up Network Configuration ... ");
//    Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Success" : "Failed!");
    WiFi.softAPConfig(local_IP, gateway, subnet);

//    WiFi.reconnect();

//    Serial.print("IP address = ");
//    Serial.println(WiFi.softAPIP());
  }
// or Join existing network
  else if (accesspointmode != 1) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
//    Serial.print("Setting up Network Configuration ... ");
    WiFi.config(local_IP, gateway, subnet);
//    WiFi.config(local_IP, gateway, subnet);

//    Serial.print("Connecting to network ... ");
//    WiFi.begin(ssid, password);
    WiFi.begin(ssid, password);
    WiFi.reconnect();

//    Serial.print("IP address = ");
//    Serial.println(WiFi.localIP());
  }
  configFile.close();
  jsonBuffer.clear();
  return true;
}

bool saveConfig() {
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["version"] = version;
  json["accesspointmode"] = accesspointmode;
  json["ssid"] = ssid;
  json["password"] = password;
  json["channel"] = channel;
  json["hidden"] = hidden;
  json["local_IP"] = local_IPstr;
  json["gateway"] = gatewaystr;
  json["subnet"] = subnetstr;
  json["update_username"] = update_username;
  json["update_password"] = update_password;
  json["ftp_username"] = ftp_username;
  json["ftp_password"] = ftp_password;
  json["ftpenabled"] = ftpenabled;
  json["ledenabled"] = ledenabled;
  json["logname"] = logname;
  json["bufferlength"] = bufferlength;
  json["rxpacketgap"] = rxpacketgap;
  json["txdelayus"] = txdelayus;
  json["txdelayms"] = txdelayms;
  json["safemode"] = safemode;

  File configFile = SPIFFS.open("/esprfidtool.json", "w");
  json.printTo(configFile);
  configFile.close();
  jsonBuffer.clear();
  return true;
}

File fsUploadFile;
String webString;

void ListLogs(){
  String directory;
  directory="/";
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  String total;
  total=fs_info.totalBytes;
  String used;
  used=fs_info.usedBytes;
  String freespace;
  freespace=fs_info.totalBytes-fs_info.usedBytes;
  Dir dir = SPIFFS.openDir(directory);
     String FileList = String()+F("<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - Logs")+"<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br>Select Logfile to display:<br><br><table border='1'><tr><td><b>Display File Content</b></td><td><b>Size in Bytes</b></td><td><b>Download File</b></td><td><b>Delete File</b></td></tr>";
  while (dir.next()) {
    String FileName = dir.fileName();
    File f = dir.openFile("r");
    FileList += " ";
    //if((!FileName.startsWith("/payloads/"))&&(!FileName.startsWith("/esploit.json"))&&(!FileName.startsWith("/esportal.json"))&&(!FileName.startsWith("/esprfidtool.json"))&&(!FileName.startsWith("/config.json"))) FileList += "<tr><td><a href=\"/viewlog?payload="+FileName+"\">"+FileName+"</a></td>"+"<td>"+f.size()+"</td><td><a href=\""+FileName+"\"><button>Download File</button></td><td><a href=\"/deletelog?payload="+FileName+"\"><button>Delete File</button></td></tr>";
    if((!FileName.startsWith("/payloads/"))&&(!FileName.startsWith("/esploit.json"))&&(!FileName.startsWith("/latestlog.json"))&&(!FileName.startsWith("/eventlog.json"))&&(!FileName.startsWith("/esportal.json"))&&(!FileName.startsWith("/esprfidtool.json"))&&(!FileName.startsWith("/config.json"))) FileList += "<tr><td><a href=\"/viewlog?payload="+FileName+"\"><button>"+FileName+"</button></a></td><td>"+String(f.size())+"</td><td><a href=\""+FileName+"\"><button>Download File</button></td><td><a href=\"/deletelog?payload="+FileName+"\"><button>Delete File</button></td></tr>";
    f.close();
  }
  FileList += "</table>";
  server.send(200, "text/html", FileList);
}

bool RawFile(String rawfile) {
  if (SPIFFS.exists(rawfile)) {
    if(!server.authenticate(update_username, update_password)){
      server.requestAuthentication();}
    File file = SPIFFS.open(rawfile, "r");
    size_t sent = server.streamFile(file, "application/octet-stream");
    file.close();
    return true;
  }
  return false;
}

void ViewLog(){
/*  webString="";
  String payload;
  String ShowPL;
  payload += server.arg(0);
  File f = SPIFFS.open(payload, "r");
  String webString = f.readString();
  f.close();
  ShowPL = String()+F(
    "<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - View Logs")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br>"
    "<button onclick=\"window.location.href='/logs'\">List Exfiltrated Data</button>"
    "<button onclick=\"window.location.href='/experimental'\">TX Mode</button>"
    "<button onclick=\"window.location.href='/data-convert'\">Data Conversion Tools</button>"

    "<FORM action=\"/api/tx/bin\" id=\"api_tx\" method=\"get\"  target=\"_blank\">"
      "<small>Binary: </small><INPUT form=\"api_tx\" type=\"text\" name=\"binary\" value=\"\" pattern=\"[01,]{1,}\" required title=\"Allowed characters(0,1,\",\"), must not be empty\" minlength=\"1\" size=\"52\"> "
      "<INPUT form=\"api_tx\" type=\"submit\" value=\"Transmit\"><br>"
      "<small>Pulse Width: </small><INPUT form=\"api_tx\" type=\"number\" name=\"pulsewidth\" value=\"40\" minlength=\"1\" min=\"0\" size=\"8\"><small>us</small> "
      "<small>Data Interval: </small><INPUT form=\"api_tx\" type=\"number\" name=\"interval\" value=\"2000\" minlength=\"1\" min=\"0\" size=\"8\"><small>us</small> "
      "<small>Delay Between Packets: </small><INPUT form=\"api_tx\" type=\"number\" name=\"wait\" value=\"100000\" minlength=\"1\" min=\"0\" size=\"8\"><small>us</small><br>"
      "<INPUT form=\"api_tx\" type=\"hidden\" name=\"prettify\" id=\"prettify\" value=\"1\">"
    "</FORM>"
    "<small>Use commas to separate the binary for transmitting multiple packets(useful for sending multiple keypresses for imitating keypads)</small><br>"
    "<hr>"
    "<a href=\"")+payload+F("\"><button>Download File</button><a><a href=\"/deletelog?payload=")+payload+F("\"><button>Delete File</button></a>"
    "<pre>")
    +payload+
    F("\n"
    "Note: Preambles shown are only a guess based on card length and may not be accurate for every card format.\n"
    "-----\n")
    +webString+
    F("</pre></body></html>")
    ;
  webString="";
  server.send(200, "text/html", ShowPL);
*/

  String payload;
  payload += server.arg(0);
  File f = SPIFFS.open(payload, "r");


  if (f) {
      server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
      server.sendHeader("Pragma", "no-cache");
      server.sendHeader("Expires", "-1");
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/html","");
      server.sendContent(String()+F(
    "<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - View Logs")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br>"
    "<button onclick=\"window.location.href='/logs'\">List Exfiltrated Data</button>"
    "<button onclick=\"window.location.href='/experimental'\">TX Mode</button>"
    "<button onclick=\"window.location.href='/data-convert'\">Data Conversion Tools</button>"

    "<FORM action=\"/api/tx/bin\" id=\"api_tx\" method=\"get\"  target=\"_blank\">"
      "<small>Binary: </small><INPUT form=\"api_tx\" type=\"text\" name=\"binary\" value=\"\" pattern=\"[01,]{1,}\" required title=\"Allowed characters(0,1,\",\"), must not be empty\" minlength=\"1\" size=\"52\"> "
      "<INPUT form=\"api_tx\" type=\"submit\" value=\"Transmit\"><br>"
      "<small>Pulse Width: </small><INPUT form=\"api_tx\" type=\"number\" name=\"pulsewidth\" value=\"40\" minlength=\"1\" min=\"0\" size=\"8\"><small>us</small> "
      "<small>Data Interval: </small><INPUT form=\"api_tx\" type=\"number\" name=\"interval\" value=\"2000\" minlength=\"1\" min=\"0\" size=\"8\"><small>us</small> "
      "<small>Delay Between Packets: </small><INPUT form=\"api_tx\" type=\"number\" name=\"wait\" value=\"100000\" minlength=\"1\" min=\"0\" size=\"8\"><small>us</small><br>"
      "<INPUT form=\"api_tx\" type=\"hidden\" name=\"prettify\" id=\"prettify\" value=\"1\">"
    "</FORM>"
    "<small>Use commas to separate the binary for transmitting multiple packets(useful for sending multiple keypresses for imitating keypads)</small><br>"
    "<hr>"
    "<a href=\"")+payload+F("\"><button>Download File</button><a><a href=\"/deletelog?payload=")+payload+F("\"><button>Delete File</button></a>"
    "<pre>")
    +payload+
    F("\n"
    "Note: Preambles shown are only a guess based on card length and may not be accurate for every card format.\n"
    "-----\n"));
  
    while (f.available()) {
      char buffer[64];
      int bytesRead = f.readBytes(buffer, sizeof(buffer));

      buffer[bytesRead] = '\0';  // Nullterminator hinzufügen
      String sanitizedBuffer = sanitizeString(buffer, bytesRead);
      server.sendContent(sanitizedBuffer);
        //server.sendContent(String(buffer));  // Füge den Puffer zum Antwortinhalt hinzu
        }
    f.close();
    server.sendContent(F("---END-OF-LOGFILE---\n"));
    server.sendContent(F("</body></html>")); // html closing
    server.sendContent(F("")); // this tells web client that transfer is done
    server.client().stop();
  }
}

// Start Networking
void setup() {
  Serial.begin(9600);
  Serial.println(F("....."));
  Serial.println(String()+F("ESP-RFID-Tool v")+version);
  //SPIFFS.format();
  
  SPIFFS.begin();
  
 //loadDefaults(); //uncomment to restore default settings if double reset fails for some reason

//Jump RESTORE_DEFAULTS_PIN to GND while powering on device to reset the device to factory defaults
  pinMode(RESTORE_DEFAULTS_PIN, INPUT_PULLUP);
  jumperState = digitalRead(RESTORE_DEFAULTS_PIN);
  if (jumperState == LOW) {
    Serial.println(String()+F("Pin ")+RESTORE_DEFAULTS_PIN+F("Grounded"));
    Serial.println(F("Loading default config..."));
    loadDefaults();
  }
  
  loadConfig();

  if(!wg.begin(DATA0,DATA1,bufferlength,rxpacketgap)) {       
    Serial.println(F("Could not begin Wiegand logging,"));            
    Serial.println(F("Out of memory!"));
  }

//RAW-WIEGAND-DATA
wg.onRawData([](volatile unsigned char* data, unsigned int bits) {

    newBitCount     = bits;
    newBitstream    = "";
    newReplayBinary = "";
    newUIDHex       = "";
    newUIDFormat    = "";

    Serial.print("RAW CALLBACK TRIGGERED, bits=");
    Serial.println(bits);

    // Buffergröße holen
    unsigned int bufferSize = wg.getBufferSize();

for (int i = 0; i < bufferSize; i++) {
    char buf[4];
    sprintf(buf, "%02X", data[i]);
    Serial.print(buf);
    Serial.print(" ");
}
Serial.println();

// 2. Bit-Dump des kompletten Buffers
for (int byte = 0; byte < bufferSize; byte++) {
    for (int bit = 7; bit >= 0; bit--) {
        Serial.print((data[byte] >> bit) & 1);
    }
    Serial.print(" ");
}
Serial.println();
Serial.println("========================");
    
    int countedBytes = (bits + 7) / 8;
    int offset = bufferSize - countedBytes;

    // Bit-Leser
    auto getBit = [&](int pos) {
        int byteIndex = offset + (pos >> 3);
        int bitIndex  = 7 - (pos & 7);
        return (data[byteIndex] >> bitIndex) & 1;
    };

    // --- 1. Echte Bits = letzte N Bits im 40-Bit-Block ---
    // Buffer enthält 40 Bits → echte Bits = raw[40 - bits .. 39]
    int start = 40 - bits;
    String realBits = "";
    for (int i = start; i < 40; i++) {
        realBits += getBit(i) ? '1' : '0';
    }

    newBitstream = realBits;

    Serial.print("RAW BITSTREAM: ");
    Serial.println(newBitstream);

    // --- 2. UID extrahieren je nach Bitlänge ---
    uint32_t rawUID = 0;

    if (bits == 34) {
        // Parity vorne = Bit 0
        // Nutzbits = Bits 1..32
        for (int i = 1; i <= 32; i++) {
            rawUID = (rawUID << 1) | (newBitstream[i] == '1');
        }
        newUIDFormat = "Wiegand 34 (UID32 Little-Endian)";
    }
    else if (bits == 32) {
        // Direkt 32 Nutzbits
        for (int i = 0; i < 32; i++) {
            rawUID = (rawUID << 1) | (newBitstream[i] == '1');
        }
        newUIDFormat = "Wiegand 32 (UID32 Little-Endian)";
    }
    else if (bits == 35) {
        // Wiegand 35:
        // Bit 0 = Leading Parity
        // Bits 1..32 = UID32
        // Bits 33..34 = Trailing Parity
        for (int i = 1; i <= 32; i++) {
            rawUID = (rawUID << 1) | (newBitstream[i] == '1');
        }
        newUIDFormat = "Wiegand 35 (UID32 Little-Endian)";
    }
else if (bits == 8) {
    // Key liegt im letzten Byte des Buffers
    unsigned int bufferSize = wg.getBufferSize();
    uint8_t rawByte = data[bufferSize - 1];

    // Bitstream nur für Anzeige
    String realBits = "";
    for (int i = 7; i >= 0; i--) {
        realBits += ((rawByte >> i) & 1) ? '1' : '0';
    }
    newBitstream = realBits;

    // Taste über dein zentrales Mapping
    newUIDHex   = mapBitsToKey(rawByte, rawByte);
    newUIDFormat = "Keypad 8-bit";

    Serial.print("KEYPAD 8-bit: ");
    Serial.println(newUIDHex);
    return;
}

else if (bits == 4) {
    unsigned int bufferSize = wg.getBufferSize();
    uint8_t rawByte = data[bufferSize - 1];

    String realBits = "";
    for (int i = 3; i >= 0; i--) {
        realBits += ((rawByte >> i) & 1) ? '1' : '0';
    }
    newBitstream = realBits;

    newUIDHex    = mapBitsToKey(rawByte, rawByte);
    newUIDFormat = "Keypad 4-bit";

    Serial.print("KEYPAD 4-bit: ");
    Serial.println(newUIDHex);
    return;
}


    else {
        newUIDHex = "UNKNOWN";
        newUIDFormat = "Unsupported (" + String(bits) + " bits)";
        Serial.println("Unsupported bit length");
        return;
    }


    // --- 3. Little-Endian konvertieren ---
    uint32_t uid =
        ((rawUID & 0x000000FF) << 24) |
        ((rawUID & 0x0000FF00) << 8)  |
        ((rawUID & 0x00FF0000) >> 8)  |
        ((rawUID & 0xFF000000) >> 24);

    char buf[20];
    sprintf(buf, "%08X", uid);

    newUIDHex = String(buf);

    Serial.print("RAW UID: ");
    Serial.println(newUIDHex);
});



//Set up Web Pages
  server.on("/",[]() {
  /*  FSInfo fs_info;
    SPIFFS.info(fs_info);
    String total;
    total=fs_info.totalBytes;
    String used;
    used=fs_info.usedBytes;
    String freespace;
    freespace=fs_info.totalBytes-fs_info.usedBytes;*/
    server.send(200, "text/html", String()+F("<html>")+css("ESP-RFID-Tool-v2")+F("<body><h1><b>ESP-RFID-Tool v")+version+F("</b></h1>"
    "<img src=\"data:image/png;base64,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\" /><br>"
    //"File System Info Calculated in Bytes<br>"
    //"<b>Total:</b> ")+total+" <b>Free:</b> "+freespace+" "+" <b>Used:</b> "+used+F("<br>-----<br>"
    //"<a href=\"/status\">Device and Wiring Status</a><br>-<br>"
    "<br>"
    "<div id=\"livebox\" style=\"padding:10px; border:1px solid #ccc; margin-top:20px;\">"
    "<b>Last Event:</b><br>"
    "Bits: <span id=\"liveBits\"></span><br>"
    "Format: <span id=\"liveFormat\"></span><br>"
    "UID/Key: <span id=\"liveUID\"></span><br>"
    "Bitstream: <span id=\"liveStream\"></span><br>"
    "</div>"
    "<br>"
    "<strong>Main-Functions</strong><br>"
    "<button onclick=\"window.location.href='/logs'\">LOG + AUTOREPLAY</button>"
    "<button onclick=\"window.location.href='/keypad'\">Keypad</button>"
    "<button onclick=\"window.location.href='/wiegand'\">Cardpad</button>"
    "<button onclick=\"window.location.href='/experimental'\">Manual Mode</button>"
    "<br>"
    "<strong>Setup and Configuration</strong><br>"
    "<button onclick=\"window.location.href='/status'\">Device and Wiring Status</button>"
    "<button onclick=\"window.location.href='/data-convert'\">Data Conversion Tools</button>"
    "<button onclick=\"window.location.href='/settings'\">Configure Settings</button>"
    "<button onclick=\"window.location.href='/format'\">Format File System</button>"
    "<button onclick=\"window.location.href='/firmware'\">Upgrade Firmware</button>"
    "<br>"
    "<strong>Help</strong><br>"
    "<button onclick=\"window.location.href='/api/help'\">API Info</button>"
    "<button onclick=\"window.location.href='/help'\">Help</button>"
    "<br>"
    "Raik Schneider (Einstein2150)<br>"
    "<a href=\"https://github.com/Einstein2150\">https://github.com/Einstein2150</a><br>"
    "<br>"
    "Original Software created by Corey Harding<br>"
    "<br>"
    //"www.RFID-Tool.com<br>"
    //"www.LegacySecurityGroup.com / www.Exploit.Agency</i><br>"
    //"-----<br>"
    "<script>"
    "function poll(){"
    "fetch('/api/lastread').then(r=>r.json()).then(data=>{"
    "document.getElementById('liveBits').innerText=data.bits;"
    "document.getElementById('liveFormat').innerText=data.format;"
    "document.getElementById('liveUID').innerText=data.uid;"
    "document.getElementById('liveStream').innerText=data.bitstream;"
    "}).catch(err=>console.log(err));"
    "}"
    "setInterval(poll,500);"
    "poll();"
    "</script>"
    "</body></html>"));
  });

  server.onNotFound([]() {
    if (!RawFile(server.uri()))
      server.send(404, "text/plain", F("Error 404 File Not Found"));
  });


  server.on("/status", []() {
    wg.pause();
    String DATA0_stat = "";
    String DATA1_stat = "";
    int DATA0_val = digitalRead(DATA0);
    int DATA1_val = digitalRead(DATA1);
    pinMode(DATA0, INPUT);
    pinMode(DATA0_MOS, INPUT);
    pinMode(DATA1, INPUT);
    pinMode(DATA1_MOS, INPUT);
    wg.clear();

    if(DATA0_val==1) {DATA0_stat = "connected";} else {DATA0_stat = "NOT connected - check wiring!";}
    if(DATA1_val==1) {DATA1_stat = "connected";} else {DATA1_stat = "NOT connected - check wiring!";}  

    int CPUfreq = ESP.getCpuFreqMHz();
    int FreeHeap = ESP.getFreeHeap();
    unsigned long uptime = millis();
    uptime = uptime / 1000 / 60;
    uint32_t chipID = ESP.getChipId();
    uint32_t flashSize = ESP.getFlashChipSize(); //bytes
    const char* sdkVersion = ESP.getSdkVersion();
    
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    String total;
    total=fs_info.totalBytes;
    String used;
    used=fs_info.usedBytes;
    String freespace;
    freespace=fs_info.totalBytes-fs_info.usedBytes;
     
    server.send(200, "text/html", String()+F(
        "<!DOCTYPE HTML>"
        "<html>")+css("ESP-RFID-Tool-v2 - Device Status")+F(""
        "<body>"
        "<button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br>"
        "<h2>Wiegand-GPIO-status:</h2>"
        "DATA0 (Green) at D5: <strong>")+DATA0_stat+F("</strong><br>"
        "DATA1 (White) at D6: <strong>")+DATA1_stat+F("</strong><br>"
        "<p></p>"
        "<p></p>"
        "<h2>Internal device infos:</h2>"
        "Uptime: <strong>")+uptime+F(" Minutes</strong><br>"        
        "CPU-Frequency: <strong>")+CPUfreq+F(" Mhz</strong><br>"
        "Free heap: <strong>")+FreeHeap+F(" Byte</strong><br>"
        "Flash-size: <strong>")+flashSize+F(" Byte</strong><br>"
        "Board-ID: <strong>")+chipID+F("</strong><br>"
        "SDK-Version: <strong>")+sdkVersion+F("</strong><br>"
        "<p></p>"
        "<h2>File System Info:</h2>"
        "Total: <strong>")+total+F(" Byte</strong><br>"
        "Free: <strong>")+freespace+F(" Byte</strong><br>"
        "Used: <strong>")+used+F(" Byte</strong><br>"
        "</body></html>"));
  });
  
  server.on("/settings", handleSettings);


server.on("/keypad", []() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html",
    String()+F("<!DOCTYPE HTML><html>")
    + css("ESP-RFID-Tool-v2 - Keypad")
    + F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br>")
  );
  // --- STYLE ---
  server.sendContent(F(
    "<style>"
    ".keypad { display: grid; grid-template-columns: repeat(3, 60px); gap: 10px; margin-top:10px; }"
    ".key { width: 60px; height: 60px; font-size: 24px; }"
    "</style>"
  ));
  // --- TITLE ---
  server.sendContent(F("<b>Transmit PIN via Keypad:</b><br><br>"));
  // --- BIT SELECTOR ---
  server.sendContent(F(
    "<label><input type='radio' name='pinbits' value='4' checked onclick='setBits(4)'> 4‑bit</label> "
    "<label><input type='radio' name='pinbits' value='8' onclick='setBits(8)'> 8‑bit</label><br><br>"
  ));
  // --- KEYPAD ---
  server.sendContent(F("<div class='keypad'>"));
  const char* keys[] = {"1","2","3","4","5","6","7","8","9","*","0","#"};
  for (int i = 0; i < 12; i++) {
    server.sendContent(
      String()+"<button class='key' onclick=\"sendKey('"+keys[i]+"')\">"+keys[i]+"</button>"
    );
  }
  server.sendContent(F("</div>"));
  // --- JAVASCRIPT ---
  server.sendContent(F(
    "<script>"
    "let pinbits = 4;"
    "function setBits(b){ pinbits = b; }"
    "function sendKey(k){"
    "  fetch('/api/pininstant?key=' + encodeURIComponent(k) + '&bits=' + pinbits);"
    "}"
    "</script>"
  ));
  server.sendContent(F("</body></html>"));
});


server.on("/wiegand", []() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html",
    String()+F("<!DOCTYPE HTML><html>")
    + css("ESP-RFID-Tool-v2 - Wiegand Encoder")
    + F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br>")
  );
  // --- STYLE ---
  server.sendContent(F(
    "<style>"
    "input { width: 200px; height: 30px; font-size: 18px; margin-top: 10px; }"
    "select { width: 120px; height: 30px; font-size: 18px; margin-top: 10px; }"
    "button.send { width: 200px; height: 40px; font-size: 20px; margin-top: 20px; }"
    "</style>"
  ));
  // --- TITLE ---
  server.sendContent(F("<b>Wiegand UID Encoder</b><br><br>"));
  // --- UID INPUT ---
  server.sendContent(F(
    "UID (HEX):<br>"
    "<input id='uid' type='text' placeholder='e.g. 1A2B3C4D'><br><br>"
  ));
  // --- FORMAT SELECTOR ---
  server.sendContent(F(
    "Format:<br>"
    "<select id='format'>"
    "<option value='32'>Wiegand 32</option>"
    "<option value='34'>Wiegand 34</option>"
    "<option value='35'>Wiegand 35</option>"
    "</select><br><br>"
  ));
  // --- SEND BUTTON ---
  server.sendContent(F(
    "<button class='send' onclick='sendWiegand()'>SEND</button>"
  ));
  // --- JAVASCRIPT ---
  server.sendContent(F(
    "<script>"
    "function sendWiegand(){"
    "  let uid = document.getElementById(\"uid\").value;"
    "  let format = document.getElementById(\"format\").value;"
    "  if(uid.length == 0){ alert(\"Please enter UID\"); return; }"
    "  fetch('/api/wiegandencode?uid=' + encodeURIComponent(uid) + '&format=' + format);"
    "}"
    "</script>"
  ));
  server.sendContent(F("</body></html>"));
});


  server.on("/firmware", [](){
    server.send(200, "text/html",
     String() +
      F("<!DOCTYPE HTML><html>") +
      css("ESP-RFID-Tool-v2 - Firmware") +
      F("<body>"
        "<button onclick=\"window.location.href='/'\">&lt;- BACK TO MAIN-MENU</button><br>"
        "Download the latest Firmware for the ESP-RFID Tool v2 PRO from "
        "<a href=\"https://github.com/Einstein2150/ESP-RFID-Tool-v2\">"
        "https://github.com/Einstein2150/ESP-RFID-Tool-v2</a><br>"
        "Click \"Browse\", select the firmware binary you downloaded, then click \"Update\".<br>"
        "You may need to manually reboot the device to reconnect.<br>"
        "<iframe style=\"border:0;height:100%;\" src=\"http://") +
     local_IPstr +
     F(":1337/update\">"
       "<a href=\"http://") +
     local_IPstr +
     F(":1337/update\">Click here to Upload Firmware</a>"
       "</iframe>"
        "</body></html>")
    ); 
//   String()+F("<html><body style=\"height: 100%;\"><a href=\"/\"><- BACK TO MAIN-MENU</a><br><br>Open Arduino IDE.<br>Pull down \"Sketch\" Menu then select \"Export Compiled Binary\".<br>On this page click \"Browse\", select the binary you exported earlier, then click \"Update\".<br>You may need to manually reboot the device to reconnect.<br><iframe style =\"border: 0; height: 100%;\" src=\"http://")+local_IPstr+F(":1337/update\"><a href=\"http://")+local_IPstr+F(":1337/update\">Click here to Upload Firmware</a></iframe></body></html>"));
  });

  server.on("/restoredefaults", [](){
    server.send(200, "text/html", css("ESP-RFID-Tool-v2 - Restore")+F("<html><body>This will restore the device to the default configuration.<br><br>Are you sure?<br><br><a href=\"/restoredefaults/yes\">YES</a> - <a href=\"/\">NO</a></body></html>"));
  });

  server.on("/restoredefaults/yes", [](){
    if(!server.authenticate(update_username, update_password))
      return server.requestAuthentication();
    server.send(200, "text/html", String()+F("<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - Restore")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br><br>Network<br>---<br>SSID: <b>ESP-RFID-Tool</b><br><br>Administration<br>---<br>USER: <b>admin</b> PASS: <b>rfidtool</b>"));
    delay(50);
    loadDefaults();
    ESP.restart();
  });

  server.on("/deletelog", [](){
    String deletelog;
    deletelog += server.arg(0);
    server.send(200, "text/html", String()+F("<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - Delete Log")+F("<html><body>This will delete the file: ")+deletelog+F(".<br><br>Are you sure?<br><br><a href=\"/deletelog/yes?payload=")+deletelog+F("\">YES</a> - <a href=\"/\">NO</a></body></html>"));
  });

  server.on("/viewlog", ViewLog);

  server.on("/deletelog/yes", [](){
    if(!server.authenticate(update_username, update_password))
      return server.requestAuthentication();
    String deletelog;
    deletelog += server.arg(0);
    if (!deletelog.startsWith("/payloads/")) server.send(200, "text/html", String()+F("<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - Delete Log")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br><br><a href=\"/logs\">List Exfiltrated Data</a><br><br>Deleting file: ")+deletelog);
    delay(50);
    SPIFFS.remove(deletelog);
  });

  server.on("/format", [](){
    server.send(200, "text/html", String()+F("<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - Format")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br><br>This will reformat the SPIFFS File System.<br><br>Are you sure?<br><br><a href=\"/format/yes\">YES</a> - <a href=\"/\">NO</a></body></html>"));
  });

  server.on("/logs", ListLogs);

  server.on("/reboot", [](){
    if(!server.authenticate(update_username, update_password))
    return server.requestAuthentication();
    server.send(200, "text/html", String()+F("<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - Reboot")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br><br>Rebooting Device..."));
    delay(50);
    ESP.restart();
  });
  
  server.on("/format/yes", [](){
    if(!server.authenticate(update_username, update_password))
      return server.requestAuthentication();
    server.send(200, "text/html", String()+F("<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - Format")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br><br>Formatting file system: This may take up to 90 seconds"));
    delay(50);
//    Serial.print("Formatting file system...");
    SPIFFS.format();
//    Serial.println(" Success");
    saveConfig();
  });
  
  server.on("/help", []() {
    server.send_P(200, "text/html", HelpText);
    //server.send(200, "text/html",String()+F("<html>")+css("ESP-RFID-Tool-v2 Help")+HelpText);
  });
  
  server.on("/license", []() {
    server.send_P(200, "text/html", License);
  });

  server.on("/data-convert", [](){

    if (server.hasArg("bin2hexHTML")) {

      int bin2hexBUFFlen=(((server.arg("bin2hexHTML")).length())+1);
      char bin2hexCHAR[bin2hexBUFFlen];
      (server.arg("bin2hexHTML")).toCharArray(bin2hexCHAR,bin2hexBUFFlen);

      dataCONVERSION+=String()+F("Binary: ")+bin2hexCHAR+F("<br><br>");

      String hexTEMP="";

      int binCOUNT=(bin2hexBUFFlen-1);
      for (int currentBINpos=0; currentBINpos<binCOUNT; currentBINpos=currentBINpos+4) {
        char hexCHAR[2];
        char tempNIBBLE[5];
        strncpy(tempNIBBLE, &bin2hexCHAR[currentBINpos], 4);
        tempNIBBLE[4]='\0';
        sprintf(hexCHAR, "%X", (strtol(tempNIBBLE, NULL, 2)));
        hexTEMP+=hexCHAR;
      }

      dataCONVERSION+=String()+F("Hexadecimal: ")+hexTEMP+F("<br><small>You may want to drop the leading zero(if there is one) and if your cloning software does not handle it for you.</small><br><br>");
      hexTEMP="";
      
      dataCONVERSION+=F("<br><br>");
      
      bin2hexBUFFlen=0;
    }

    if (server.hasArg("hex2binHTML")) {

      int hex2binBUFFlen=(((server.arg("hex2binHTML")).length())+1);
      char hex2binCHAR[hex2binBUFFlen];
      (server.arg("hex2binHTML")).toCharArray(hex2binCHAR,hex2binBUFFlen);

      dataCONVERSION+=String()+F("Hexadecimal: ")+hex2binCHAR+F("<br><br>");

      String binTEMP="";

      int charCOUNT=(hex2binBUFFlen-1);
      for (int currentHEXpos=0; currentHEXpos<charCOUNT; currentHEXpos++) {
        char binCHAR[5];
        char tempHEX[2];
        strncpy(tempHEX, &hex2binCHAR[currentHEXpos], 1);
        tempHEX[1]='\0';
        int decimal=(unsigned char)strtoul(tempHEX, NULL, 16);
        itoa(decimal,binCHAR,2);
        while (strlen(binCHAR) < 4) {
          char *dup;
          sprintf(binCHAR,"%s%s","0",(dup=strdup(binCHAR)));
          free(dup);
        }
        binTEMP+=binCHAR;
      }

      dataCONVERSION+=String()+F("Binary: ")+binTEMP+F("<br><br>");
      binTEMP="";
      
      dataCONVERSION+=F("<br><br>");
      
      hex2binBUFFlen=0;
    }
    
    if (server.hasArg("abaHTML")) {
      String abaHTML=(server.arg("abaHTML"));

      dataCONVERSION="Trying \"Forward\" Swipe<br>";
      dataCONVERSION+=("Forward Binary:"+abaHTML+"<br>");
      int abaStart=abaHTML.indexOf("11010");
      int abaEnd=(abaHTML.lastIndexOf("11111")+4);
      dataCONVERSION+=aba2str(abaHTML,abaStart,abaEnd,"\"Forward\" Swipe");
      
      dataCONVERSION+=" * Trying \"Reverse\" Swipe<br>";
      int abaBUFFlen=((abaHTML.length())+1);
      char abachar[abaBUFFlen];
      abaHTML.toCharArray(abachar,abaBUFFlen);
      abaHTML=String(strrev(abachar));
      dataCONVERSION+=("Reversed Binary:"+abaHTML+"<br>");
      abaStart=abaHTML.indexOf("11010");
      abaEnd=(abaHTML.lastIndexOf("11111")+4);
      dataCONVERSION+=aba2str(abaHTML,abaStart,abaEnd,"\"Reverse\" Swipe");
    
      //dataCONVERSION+=(String()+F(" * You can verify the data at the following URL:<br><a target=\"_blank\" href=\"https://www.legacysecuritygroup.com/aba-decode.php?binary=")+abaHTML+F("\">https://www.legacysecuritygroup.com/aba-decode.php?binary=")+abaHTML+F("</a>"));
      dataCONVERSION.replace("*", "<br><br>");
      dataCONVERSION.replace(":", ": ");

      abaHTML="";
      abaStart=0;
      abaEnd=0;
    }
    
    server.send(200, "text/html", String()+F(
      "<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - Conversion Tool")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br>")
      +dataCONVERSION+
      F(
      "<hr>"
      "<FORM action=\"/data-convert\" id=\"aba2ascii\" method=\"post\">"
      "<b>Convert ABA Binary Data to ASCII:</b><br>"
      "<INPUT form=\"aba2ascii\" type=\"text\" name=\"abaHTML\" value=\"\" pattern=\"[0-1]{1,}\" required title=\"Only 0's & 1's allowed, must not be empty\" minlength=\"1\" size=\"52\"><br>"
      "<INPUT form=\"aba2ascii\" type=\"submit\" value=\"Convert\"><br>"
      "</FORM>"
      "<br>"
      "<FORM action=\"/data-convert\" id=\"bin2hex\" method=\"post\">"
      "<b>Convert Binary Data to Hexadecimal:</b><br>"
      "<small>For use with card cloning, typically includes both the preamble and card data(binary before and after the space in log).</small><br>"
      "<INPUT form=\"bin2hex\" type=\"text\" name=\"bin2hexHTML\" value=\"\" pattern=\"[0-1]{1,}\" required title=\"Only 0's & 1's allowed, no spaces allowed, must not be empty\" minlength=\"1\" size=\"52\"><br>"
      "<INPUT form=\"bin2hex\" type=\"submit\" value=\"Convert\"><br>"
      "</FORM>"
      "<br>"
      "<FORM action=\"/data-convert\" id=\"hex2bin\" method=\"post\">"
      "<b>Convert Hexadecimal Data to Binary:</b><br>"
      "<small>In some situations you may want to add a leading zero to pad the output to come up with the correct number of bits.</small><br>"
      "<INPUT form=\"hex2bin\" type=\"text\" name=\"hex2binHTML\" value=\"\" pattern=\"[0-9a-fA-F]{1,}\" required title=\"Only characters 0-9 A-F a-f allowed, no spaces allowed, must not be empty\" minlength=\"1\" size=\"52\"><br>"
      "<INPUT form=\"hex2bin\" type=\"submit\" value=\"Convert\"><br>"
      "</FORM>"
      )
    );
      
    dataCONVERSION="";
  });

  //#include "api_server.h"

  server.on("/stoptx", [](){
    server.send(200, "text/html", String()+F("<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - Stop TX")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br><body>This will kill any ongoing transmissions.<br><br>Are you sure?<br><br><a href=\"/stoptx/yes\">YES</a> - <a href=\"/\">NO</a></body></html>"));
  });

  server.on("/stoptx/yes", [](){
    TXstatus=0;
    server.send(200, "text/html", String()+F("<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - Stop TX")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br><br><a href=\"/experimental\"><- BACK TO TX MODE</a><br><br>All transmissions have been stopped."));
  });

  server.on("/experimental", [](){
    String experimentalStatus="Awaiting Instructions";

    if (server.hasArg("pinHTML")||server.hasArg("bruteEND")) {
      pinHTML=server.arg("pinHTML");
      int pinBITS=server.arg("pinBITS").toInt();
      int pinHTMLDELAY=server.arg("pinHTMLDELAY").toInt();
      int bruteforcing;
      int brutePAD=(server.arg("bruteSTART").length());
      if (server.hasArg("bruteSTART")) {
        bruteforcing=1;
      }
      else {
        bruteforcing=0;
      }

      TXstatus=1;
      
      wg.pause();
      digitalWrite(DATA0, HIGH);
      digitalWrite(DATA0_MOS, LOW);
      pinMode(DATA0,OUTPUT);
      pinMode(DATA0_MOS,OUTPUT);
      digitalWrite(DATA1, HIGH);
      digitalWrite(DATA1_MOS, LOW);
      pinMode(DATA1,OUTPUT);
      pinMode(DATA1_MOS,OUTPUT);

      pinHTML.replace("F1","C");
      pinHTML.replace("F2","D");
      pinHTML.replace("F3","E");
      pinHTML.replace("F4","F");

      experimentalStatus=String()+"Transmitting "+pinBITS+"bit Wiegand Format PIN: "+pinHTML+" with a "+pinHTMLDELAY+"ms delay between \"keypresses\"";
      delay(50);
      
      int bruteSTART;
      int bruteEND;
      if (server.hasArg("bruteSTART")) {
        bruteSTART=server.arg("bruteSTART").toInt();
      }
      else {
        bruteSTART=0;
      }
      
      if (server.hasArg("bruteEND")) {
        bruteEND=server.arg("bruteEND").toInt();
      }
      else {
        bruteEND=0;
      }

      if (server.hasArg("bruteSTART")) {
        server.send(200, "text/html", String()+F("<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - TX Mode")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br><a href=\"/experimental\"><- BACK TO TX MODE</a><br><br>Brute forcing ")+pinBITS+F("bit Wiegand Format PIN from ")+(server.arg("bruteSTART"))+F(" to ")+(server.arg("bruteEND"))+F(" with a ")+pinHTMLDELAY+F("ms delay between \"keypresses\"<br>This may take a while, your device will be busy until the sequence has been completely transmitted!<br>Please \"STOP CURRENT TRANSMISSION\" before attempting to use your device or simply wait for the transmission to finish.<br>You can view if the brute force attempt has completed by returning to the TX page and checking the status located under \"Transmit Status\"<br><br><a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>"));
        delay(50);
      }

      String bruteSTARTchar="";
      String bruteENDchar="";
      if (server.hasArg("bruteSTARTchar")&&(server.arg("bruteSTARTchar")!="")) {
        bruteSTARTchar=(server.arg("bruteSTARTchar"));
        bruteSTARTchar.replace("F1","C");
        bruteSTARTchar.replace("F2","D");
        bruteSTARTchar.replace("F3","E");
        bruteSTARTchar.replace("F4","F");
      }
      if (server.hasArg("bruteENDchar")&&(server.arg("bruteENDchar")!="")) {
        bruteENDchar=(server.arg("bruteENDchar"));
        bruteENDchar=(server.arg("bruteENDchar"));
        bruteENDchar.replace("F1","C");
        bruteENDchar.replace("F2","D");
        bruteENDchar.replace("F3","E");
        bruteENDchar.replace("F4","F");
      }

      unsigned long bruteFAILdelay=0;
      unsigned long bruteFAILS=0;
      int bruteFAILmultiplier=0;
      int bruteFAILmultiplierCURRENT=0;
      int bruteFAILmultiplierAFTER=0;
      int delayAFTERpin=0;
      int bruteFAILSmax=0;
      bruteFAILSmax=(server.arg("bruteFAILSmax")).toInt();
      delayAFTERpin=(server.arg("delayAFTERpin")).toInt();
      bruteFAILdelay=(server.arg("bruteFAILdelay")).toInt();
      bruteFAILmultiplier=(server.arg("bruteFAILmultiplier")).toInt();
      bruteFAILmultiplierAFTER=(server.arg("bruteFAILmultiplierAFTER")).toInt();

      for (int brute=bruteSTART; brute<=bruteEND; brute++) {

        if (bruteforcing==1) {
          pinHTML=String(brute);
          while (pinHTML.length()<brutePAD) {
            pinHTML="0"+pinHTML;
          }
        }

        if (bruteSTARTchar!="") {
          pinHTML=bruteSTARTchar+pinHTML;
        }

        if (bruteENDchar!="") {
          pinHTML=pinHTML+bruteENDchar;
        }
          
        for (int i = 0; i < pinHTML.length(); i++) {
          char key = pinHTML.charAt(i);
          String bits = mapKeyToBits(key, pinBITS);
          if (bits != "") {
            pinSEND(pinHTMLDELAY, bits);
          }
        }

        server.handleClient();
        if (TXstatus!=1) {
          break;
        }

        bruteFAILS++;

        if (bruteFAILS>=4294967000) {
          bruteFAILS=(4294966000);
        }
        if (bruteFAILdelay>=4294967000) {
          bruteFAILdelay=(4294966000);
        }
        
        if (bruteFAILmultiplier!=0) {
          bruteFAILmultiplierCURRENT++;
          if (bruteFAILmultiplierCURRENT>=bruteFAILmultiplierAFTER) {
            bruteFAILmultiplierCURRENT=0;
            bruteFAILdelay=(bruteFAILdelay*bruteFAILmultiplier);
          }
        }
        
        if ((bruteFAILS>=bruteFAILSmax)&&(bruteFAILSmax!=0)) {
          delay(bruteFAILdelay*1000);
        }
        else {
          delay(delayAFTERpin);
        }
        
      }
      pinMode(DATA0, INPUT);
      pinMode(DATA0_MOS, INPUT);
      pinMode(DATA1, INPUT);
      pinMode(DATA1_MOS, INPUT);
      wg.clear();
      pinHTML="";
      pinHTMLDELAY=100;
      TXstatus=0;
      bruteforcing=0;
      brutePAD=0;
      bruteSTARTchar="";
      bruteENDchar="";
      bruteFAILdelay=0;
      bruteFAILS=0;
      bruteFAILmultiplier=0;
      bruteFAILmultiplierCURRENT=0;
      bruteFAILmultiplierAFTER=0;
      delayAFTERpin=0;
      bruteFAILSmax=0;
    }


    if (server.hasArg("binHTML")) {
      String binHTML=server.arg("binHTML");
      
      digitalWrite(DATA0, HIGH);
      digitalWrite(DATA0_MOS, LOW);
      pinMode(DATA0,OUTPUT);
      pinMode(DATA0_MOS,OUTPUT);
      digitalWrite(DATA1, HIGH);
      digitalWrite(DATA1_MOS, LOW);
      pinMode(DATA1,OUTPUT);
      pinMode(DATA1_MOS,OUTPUT);

      for (int i=0; i<=binHTML.length(); i++) {
        if (binHTML.charAt(i) == '0') {
          digitalWrite(DATA0, LOW);
          digitalWrite(DATA0_MOS, HIGH);
          delayMicroseconds(txdelayus);
          digitalWrite(DATA0, HIGH);
          digitalWrite(DATA0_MOS, LOW);
        }
        else if (binHTML.charAt(i) == '1') {
          digitalWrite(DATA1, LOW);
          digitalWrite(DATA1_MOS, HIGH);
          delayMicroseconds(txdelayus);
          digitalWrite(DATA1, HIGH);
          digitalWrite(DATA1_MOS, LOW);
        }
        delay(txdelayms);
      }

      pinMode(DATA0, INPUT);
      pinMode(DATA0_MOS, INPUT);
      pinMode(DATA1, INPUT);
      pinMode(DATA1_MOS, INPUT);
      wg.clear();

      experimentalStatus=String()+"Transmitting Binary: "+binHTML;
      binHTML="";
    }

    if (server.arg("fuzzType")=="simultaneous") {

      int fuzzTimes=0;
      dos=0;
      if ((server.arg("fuzzTimes"))=="dos") {
        dos=1;
        server.send(200, "text/html", String()+
        +F("<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - TX Mode")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br>"
       // "<a href=\"/\"><- BACK TO MAIN-MENU</a><br><br>"
        "<a href=\"/experimental\"><- BACK TO TX MODE</a><br><br>"
        "Denial of Service mode active.<br>Transmitting D0 and D1 bits simultaneously until stopped."
        "<br>This may take a while, your device will be busy until the sequence has been completely transmitted!"
        "<br>Please \"STOP CURRENT TRANSMISSION\" before attempting to use your device or simply wait for the transmission to finish.<br>"
        "You can view if the fuzzing attempt has completed by returning to the TX page and checking the status located under \"Transmit Status\"<br><br>"
        "<a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>"));
        delay(50);
      }
      else {
        fuzzTimes=server.arg("fuzzTimes").toInt();
        server.send(200, "text/html", String()+
        +F("<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - TX Mode")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br>"
        //"<a href=\"/\"><- BACK TO MAIN-MENU</a><br><br>"
        "<a href=\"/experimental\"><- BACK TO TX MODE</a><br><br>"
        "Transmitting D0 and D1 bits simultaneously ")+fuzzTimes+F(" times."
        "<br>This may take a while, your device will be busy until the sequence has been completely transmitted!"
        "<br>Please \"STOP CURRENT TRANSMISSION\" before attempting to use your device or simply wait for the transmission to finish.<br>"
        "You can view if the fuzzing attempt has completed by returning to the TX page and checking the status located under \"Transmit Status\"<br><br>"
        "<a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>"));
        delay(50);
      }
      
      wg.pause();
      digitalWrite(DATA0, HIGH);
      digitalWrite(DATA0_MOS, LOW);
      pinMode(DATA0,OUTPUT);
      pinMode(DATA0_MOS,OUTPUT);
      digitalWrite(DATA1, HIGH);
      digitalWrite(DATA1_MOS, LOW);
      pinMode(DATA1,OUTPUT);
      pinMode(DATA1_MOS,OUTPUT);

      TXstatus=1;

      for (int i=0; i<=fuzzTimes || dos==1; i++) {
        digitalWrite(DATA0, LOW);
        digitalWrite(DATA0_MOS, HIGH);
        digitalWrite(DATA1, LOW);
        digitalWrite(DATA1_MOS, HIGH);
        delayMicroseconds(txdelayus);
        digitalWrite(DATA0, HIGH);
        digitalWrite(DATA0_MOS, LOW);
        digitalWrite(DATA1, HIGH);
        digitalWrite(DATA1_MOS, LOW);
        delay(txdelayms);
        server.handleClient();
        if (TXstatus!=1) {
          break;
        }
      }

      pinMode(DATA0, INPUT);
      pinMode(DATA0_MOS, INPUT);
      pinMode(DATA1, INPUT);
      pinMode(DATA1_MOS, INPUT);
      wg.clear();
      TXstatus=0;
      dos=0;

      //experimentalStatus=String()+"Transmitting D0 and D1 bits simultaneously "+fuzzTimes+" times.";
    }

    if (server.arg("fuzzType")=="alternating") {

      int fuzzTimes=0;
      dos=0;
      if ((server.arg("fuzzTimes"))=="dos") {
        dos=1;
        server.send(200, "text/html", String()+
        +F("<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - TX Mode")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br>"
        //"<a href=\"/\"><- BACK TO MAIN-MENU</a><br><br>"
        "<a href=\"/experimental\"><- BACK TO TX MODE</a><br><br>"
        "Denial of Service mode active.<br>Transmitting bits alternating between D0 and D1 until stopped."
        "<br>This may take a while, your device will be busy until the sequence has been completely transmitted!"
        "<br>Please \"STOP CURRENT TRANSMISSION\" before attempting to use your device or simply wait for the transmission to finish.<br>"
        "You can view if the fuzzing attempt has completed by returning to the TX page and checking the status located under \"Transmit Status\"<br><br>"
        "<a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>"));
        delay(50);
      }
      else {
        fuzzTimes=server.arg("fuzzTimes").toInt();
        server.send(200, "text/html", String()+
        +F("<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - TX Mode")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br>"
        //"<a href=\"/\"><- BACK TO MAIN-MENU</a><br><br>"
        "<a href=\"/experimental\"><- BACK TO TX MODE</a><br><br>"
        "Transmitting ")+fuzzTimes+F(" bits alternating between D0 and D1."
        "<br>This may take a while, your device will be busy until the sequence has been completely transmitted!"
        "<br>Please \"STOP CURRENT TRANSMISSION\" before attempting to use your device or simply wait for the transmission to finish.<br>"
        "You can view if the fuzzing attempt has completed by returning to the TX page and checking the status located under \"Transmit Status\"<br><br>"
        "<a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>"));
        delay(50);
      }
      
      wg.pause();
      digitalWrite(DATA0, HIGH);
      digitalWrite(DATA0_MOS, LOW);
      pinMode(DATA0,OUTPUT);
      pinMode(DATA0_MOS,OUTPUT);
      digitalWrite(DATA1, HIGH);
      digitalWrite(DATA1_MOS, LOW);
      pinMode(DATA1,OUTPUT);
      pinMode(DATA1_MOS,OUTPUT);

      String binALT="";
      TXstatus=1;

      for (int i=0; i<fuzzTimes || dos==1; i++) {
        if (i%2==0) {
          digitalWrite(DATA0, LOW);
          digitalWrite(DATA0_MOS, HIGH);
          delayMicroseconds(txdelayus);
          digitalWrite(DATA0, HIGH);
          digitalWrite(DATA0_MOS, LOW);
          binALT=binALT+"0";
        }
        else {
           digitalWrite(DATA1, LOW);
           digitalWrite(DATA1_MOS, HIGH);
           delayMicroseconds(txdelayus);
           digitalWrite(DATA1, HIGH);
           digitalWrite(DATA1_MOS, LOW);
           binALT=binALT+"1";
        }
        delay(txdelayms);
        server.handleClient();
        if (TXstatus!=1) {
          break;
        }
      }

      pinMode(DATA0, INPUT);
      pinMode(DATA0_MOS, INPUT);
      pinMode(DATA1, INPUT);
      pinMode(DATA1_MOS, INPUT);
      wg.clear();
      TXstatus=0;
      dos=0;

      //experimentalStatus=String()+"Transmitting alternating bits: "+binALT;
      binALT="";
    }

    if (server.arg("pushType")=="Ground") {
      Serial.end();
      digitalWrite(3,LOW);
      pinMode(3,OUTPUT);
      delay(server.arg("pushTime").toInt());
      pinMode(3,INPUT);
      Serial.begin(9600);

      experimentalStatus=String()+"Grounding \"Push to Open\" wire for "+(server.arg("pushTime").toInt())+"ms.";
    }

    if (server.arg("pushType")=="High") {
      Serial.end();
      digitalWrite(3,HIGH);
      pinMode(3,OUTPUT);
      delay(server.arg("pushTime").toInt());
      pinMode(3,INPUT);
      Serial.begin(9600);

      experimentalStatus=String()+"Outputting 3.3V on \"Push to Open\" wire for "+(server.arg("pushTime").toInt())+"ms.";
    }

    String activeTX="";
    if (TXstatus==1) {
      
      if (pinHTML!="") {
        String currentPIN=pinHTML;
        activeTX="Brute forcing PIN: "+currentPIN+"<br><a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>";
        currentPIN="";
      }
      else if (dos==1) {
        activeTX="Denial of Service mode active...<br><a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>";
      }
      else {
        activeTX="Transmitting...<br><a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>";
      }
      
    }
    else {
      activeTX="INACTIVE<br><button>NOTHING TO STOP</button>";
    }

    server.send(200, "text/html", 
      String()+
      F(
      "<!DOCTYPE HTML><html>")+css("ESP-RFID-Tool-v2 - Manual Mode (Transmitting)")+F("<body><button onclick=\"window.location.href='/'\"><- BACK TO MAIN-MENU</button><br>"
      //"<!DOCTYPE HTML>"
      //"<html>"
      //"<head>"
      //"<title>TX Mode</title>"
      //"</head>"
      "<body>"
      )+experimentalStatus+"<br><br>"
      +F(
      "<b>Transmit Status:</b> ")+activeTX+F("<br><br>"
      //"<a href=\"/\"><- BACK TO MAIN-MENU</a><br>"
      "<P>"
      "<h1>TX Mode</h1>"
      "<hr>"
      "<small>"
      "<b>Warning:</b> Use this mode at your own risk!<br>"
      "Note: Timings for the Wiegand Data Pulse Width and Wiegand Data Interval may be changed on the settings page."
      "</small>"
      "<br>"
      "<hr>"
      "<br>"
      "<FORM action=\"/experimental\" id=\"transmitbinary\" method=\"post\">"
      "<b>Binary Data:</b><br>"
      "<small>Typically no need to include preamble</small><br>"
      "<INPUT form=\"transmitbinary\" type=\"text\" name=\"binHTML\" value=\"\" pattern=\"[0-1]{1,}\" required title=\"Only 0's & 1's allowed, must not be empty\" minlength=\"1\" size=\"52\"><br>"
      "<INPUT form=\"transmitbinary\" type=\"submit\" value=\"Transmit\"><br>"
      "</FORM>"
      "<br>"
      "<hr>"
      "<br>"
      "<FORM action=\"/experimental\" id=\"transmitpin\" method=\"post\">"
      "<b>Transmit PIN:</b><br>"
      "<small>Available keys 0-9, * or A, # or B, F1 or C, F2 or D, F3 or E, F4 or F</small><br>"
      "<small>PIN: </small><INPUT form=\"transmitpin\" type=\"text\" name=\"pinHTML\" value=\"\" pattern=\"[0-9*#A-F]{1,}\" required title=\"Available keys 0-9, * or A, # or B, F1 or C, F2 or D, F3 or E, F4 or F, must not be empty\" minlength=\"1\" size=\"52\"><br>"
      "<small>Delay between \"keypresses\": </small><INPUT form=\"transmitpin\" type=\"number\" name=\"pinHTMLDELAY\" value=\"100\" minlength=\"1\" min=\"0\" size=\"8\"><small>ms</small><br>"
      "<INPUT form=\"transmitpin\" type=\"radio\" name=\"pinBITS\" id=\"pinBITS\" value=\"4\" checked required> <small>4bit Wiegand PIN Format</small>   "
      "<INPUT form=\"transmitpin\" type=\"radio\" name=\"pinBITS\" id=\"pinBITS\" value=\"8\" required> <small>8bit Wiegand PIN Format</small><br>"
      "<INPUT form=\"transmitpin\" type=\"submit\" value=\"Transmit\"><br>"
      "</FORM>"
      "<br>"
      "<hr>"
      "<br>"
      "<FORM action=\"/experimental\" id=\"brutepin\" method=\"post\">"
      "<b>Bruteforce PIN:</b><br>"
      "<small>Delay between \"keypresses\": </small><INPUT form=\"brutepin\" type=\"number\" name=\"pinHTMLDELAY\" value=\"3\" minlength=\"1\" min=\"0\" size=\"8\"><small>ms</small><br>"
      "<small>Delay between entering complete PINs: </small><INPUT form=\"brutepin\" type=\"number\" name=\"delayAFTERpin\" value=\"0\" minlength=\"1\" min=\"0\" size=\"8\"><small>ms</small><br>"
      "<small>PIN begins with character(s): </small><INPUT form=\"brutepin\" type=\"text\" name=\"bruteSTARTchar\" value=\"\" pattern=\"[0-9*#A-F]{0,}\" title=\"Available keys 0-9, * or A, # or B, F1 or C, F2 or D, F3 or E, F4 or F, must not be empty\" size=\"8\"><br>"
      "<small>PIN start position: </small><INPUT form=\"brutepin\" type=\"number\" name=\"bruteSTART\" value=\"0000\" minlength=\"1\" min=\"0\" size=\"8\"><br>"
      "<small>PIN end position: </small><INPUT form=\"brutepin\" type=\"number\" name=\"bruteEND\" value=\"9999\" minlength=\"1\" min=\"0\" size=\"8\"><br>"
      "<small>PIN ends with character(s): </small><INPUT form=\"brutepin\" type=\"text\" name=\"bruteENDchar\" value=\"#\" pattern=\"[0-9*#A-F]{0,}\" title=\"Available keys 0-9, * or A, # or B, F1 or C, F2 or D, F3 or E, F4 or F, must not be empty\" size=\"8\"><br>"
      "<small>NOTE: The advanced timing settings listed below override the \"Delay between entering complete PINs\" setting(listed above) when the conditions listed below are met.</small><br>"
      "<small>Number of failed PIN attempts(X) before a delay: </small><INPUT form=\"brutepin\" type=\"number\" name=\"bruteFAILSmax\" value=\"0\" minlength=\"1\" min=\"0\" size=\"8\"><br>"
      "<small>Delay in seconds(Y) after [X] failed PINs: </small><INPUT form=\"brutepin\" type=\"number\" name=\"bruteFAILdelay\" value=\"0\" minlength=\"1\" min=\"0\" size=\"8\"><small>s</small><br>"
      "<small>Multiply delay [Y] by <INPUT form=\"brutepin\" type=\"number\" name=\"bruteFAILmultiplier\" value=\"0\" minlength=\"1\" min=\"0\" size=\"4\"> after every <INPUT form=\"brutepin\" type=\"number\" name=\"bruteFAILmultiplierAFTER\" value=\"0\" minlength=\"1\" min=\"0\" size=\"4\"> failed pin attempts</small><br>"
      "<INPUT form=\"brutepin\" type=\"radio\" name=\"pinBITS\" id=\"pinBITS\" value=\"4\" checked required> <small>4bit Wiegand PIN Format</small>   "
      "<INPUT form=\"brutepin\" type=\"radio\" name=\"pinBITS\" id=\"pinBITS\" value=\"8\" required> <small>8bit Wiegand PIN Format</small><br>"
      "<INPUT form=\"brutepin\" type=\"submit\" value=\"Transmit\"></FORM><br>"
      "<br>"
      "<hr>"
      "<br>"
      "<b>Fuzzing:</b><br><br>"
      "<FORM action=\"/experimental\" id=\"fuzz\" method=\"post\">"
      "<b>Number of bits:</b>"
      "<INPUT form=\"fuzz\" type=\"number\" name=\"fuzzTimes\" value=\"100\" minlength=\"1\" min=\"1\" max=\"2147483647\" size=\"32\"><br>"
      //"<INPUT form=\"fuzz\" type=\"text\" name=\"fuzzTimes\" value=\"\" pattern=\"^[1-9]+[0-9]*$\" required title=\"Must be a number > 0, must not be empty \" minlength=\"1\" size=\"32\"><br>"
      "<INPUT form=\"fuzz\" type=\"radio\" name=\"fuzzType\" id=\"simultaneous\" value=\"simultaneous\" required> <small>Transmit a bit simultaneously on D0 and D1 (X bits per each line)</small><br>"
      "<INPUT form=\"fuzz\" type=\"radio\" name=\"fuzzType\" id=\"alternating\" value=\"alternating\"> <small>Transmit X bits alternating between D0 and D1 each bit (01010101,etc)</small><br>"
      "<INPUT form=\"fuzz\" type=\"submit\" value=\"Fuzz\"><br>"
      "</FORM>"
      "<br>"
      "<hr>"
      "<br>"
      "<b>Denial Of Service Mode:</b><br><br>"
      "<FORM action=\"/experimental\" id=\"dos\" method=\"post\">"
      "<b>Type of Attack:</b>"
      "<INPUT hidden=\"1\" form=\"dos\" type=\"text\" name=\"fuzzTimes\" value=\"dos\"><br>"
      "<INPUT form=\"dos\" type=\"radio\" name=\"fuzzType\" id=\"simultaneous\" value=\"simultaneous\" required> <small>Transmit a bit simultaneously on D0 and D1 until stopped</small><br>"
      "<INPUT form=\"dos\" type=\"radio\" name=\"fuzzType\" id=\"alternating\" value=\"alternating\"> <small>Transmit bits alternating between D0 and D1 each bit (01010101,etc) until stopped</small><br>"
      "<INPUT form=\"dos\" type=\"submit\" value=\"Start DoS\"><br>"
      "</FORM>"
      "<br>"
      "<hr>"
      "<br>"
      "<b>Push Button for Door Open:</b><br>"
      "<small>Connect \"Push to Open\" wire from the reader to the RX pin(GPIO3) on the programming header on ESP-RFID-Tool.</small><br>"
      "<small>Warning! Selecting the wrong trigger signal type may cause damage to the connected hardware.</small><br><br>"
      "<FORM action=\"/experimental\" id=\"push\" method=\"post\">"
      "<b>Time in ms to push the door open button:</b>"
      "<INPUT form=\"push\" type=\"text\" name=\"pushTime\" value=\"50\" pattern=\"^[1-9]+[0-9]*$\" required title=\"Must be a number > 0, must not be empty\" minlength=\"1\" size=\"32\"><br>"
      "<b>Does the wire expect a High or Low signal to open the door:</b>"
      "<INPUT form=\"push\" type=\"radio\" name=\"pushType\" id=\"Ground\" value=\"Ground\" checked required> <small>Low Signal[Ground]</small>   "
      "<INPUT form=\"push\" type=\"radio\" name=\"pushType\" id=\"Ground\" value=\"High\" required> <small>High Signal[3.3V]</small><br>"
      "<INPUT form=\"push\" type=\"submit\" value=\"Push\"><br>"
      "</FORM>"
      "<br>"
      "<hr>"
      "<br>"
      "</P>"
      "</body>"
      "</html>"
      )
    );

    if (server.args()>=1) {
      if (safemode==1) {
        delay(50);
        ESP.restart();
      }
    }

  });
  initAPI();
  server.begin();
  WiFiClient client;
  client.setNoDelay(1);

//  Serial.println("Web Server Started");

  MDNS.begin("ESP");

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();

  MDNS.addService("http", "tcp", 1337);
  
  if (ftpenabled==1){
    ftpSrv.begin(String(ftp_username),String(ftp_password));
  }

//Start RFID Reader
  pinMode(LED_BUILTIN, OUTPUT);  // LED
  if (ledenabled==1){
    digitalWrite(LED_BUILTIN, LOW);
  }
  else{
    digitalWrite(LED_BUILTIN, HIGH);
  }

}
//

//Do It!

///////////////////////////////////////////////////////
// LOOP function
void loop()
{
  if (ftpenabled==1){
    ftpSrv.handleFTP();
  }
  server.handleClient();
  httpServer.handleClient();
  while (Serial.available()) {
    String cmd = Serial.readStringUntil(':');
    if(cmd == "ResetDefaultConfig"){
      loadDefaults();
      ESP.restart();
    }
  }

//Serial.print("Free heap-");
//Serial.println(ESP.getFreeHeap(),DEC);

  if(wg.available()) {
    wg.pause();             // pause Wiegand pin interrupts
    LogWiegand(wg);
    wg.clear();             // compulsory to call clear() to enable interrupts for subsequent data
    if (safemode==1) {
      ESP.restart();
    }
  }

}
