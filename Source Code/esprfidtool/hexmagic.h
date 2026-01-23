/* Copyright (c) 2024  Raik Schneider @Einstein2150
   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
 */
 
 String hexToBinary(String hexString) {
  String binaryString = "";
  
  // Durchlaufe den Hex-String und konvertiere jeden Hex-Wert in Binär
  for (int i = 0; i < hexString.length(); i++) {
    char hexChar = hexString.charAt(i);
    int hexValue;
    
    // Konvertiere Hex-Zeichen in Dezimalzahl
    if (hexChar >= '0' && hexChar <= '9') {
      hexValue = hexChar - '0';
    } else if (hexChar >= 'A' && hexChar <= 'F') {
      hexValue = hexChar - 'A' + 10;
    } else if (hexChar >= 'a' && hexChar <= 'f') {
      hexValue = hexChar - 'a' + 10;
    } else {
      // Ungültiges Hex-Zeichen
      continue;
    }
    
    // Konvertiere Dezimalzahl in 4-Bit-Binärzahl
    for (int j = 3; j >= 0; j--) {
      binaryString += (hexValue & (1 << j)) ? '1' : '0';
    }
  }
  
  return binaryString;
}


String binaryToHex(String binaryString) {
  String hexString = "";
  
  // Stelle sicher, dass die Länge des Binär-Strings durch 4 teilbar ist
  int padding = binaryString.length() % 4;
  if (padding > 0) {
    binaryString = String("0000").substring(0, 4 - padding) + binaryString;
  }

  // Durchlaufe den Binär-String und konvertiere jeden 4-Bit-Binärwert in Hex
  for (int i = 0; i < binaryString.length(); i += 4) {
    String binarySubstring = binaryString.substring(i, i + 4);
    int hexValue = 0;

    // Konvertiere 4-Bit-Binärzahl in Dezimalzahl
    for (int j = 0; j < 4; j++) {
      hexValue = (hexValue << 1) | (binarySubstring.charAt(j) - '0');
    }

    // Konvertiere Dezimalzahl in Hex-Zeichen
    char hexChar = (hexValue < 10) ? ('0' + hexValue) : ('A' + hexValue - 10);

    hexString += hexChar;
  }
  
  return hexString;
}


String reverseHex(String hexString) {
  // Überprüfe, ob der Hex-String gerade ist
  if (hexString.length() % 2 != 0) {
    // Wenn der Hex-String ungerade ist, füge eine führende Null hinzu
    hexString = "0" + hexString;
  }

  String reversedHex = "";

  // Durchlaufe den Hex-String rückwärts in 2er-Schritten
  for (int i = hexString.length() - 2; i >= 0; i -= 2) {
    // Extrahiere ein 2-Zeichen-Paar und füge es zum umgekehrten Hex-Wert hinzu
    reversedHex += hexString.substring(i, i + 2);
  }

  return reversedHex;
}


String sanitizeString(char* buffer, int length) {
  String result = "";
  for (int i = 0; i < length; i++) {
    // Überprüfe, ob das Zeichen gültig ist (z.B. im ASCII-Bereich)
    if (buffer[i] >= 10 && buffer[i] <= 126) {
      result += buffer[i];
    } else {
      // Andernfalls ersetze es durch ein Platzhalterzeichen oder entferne es
      result += "";
    }
  }
  return result;
}



String hexToBin(String hex) {
    String out = "";
    for (int i = 0; i < hex.length(); i++) {
        char c = hex.charAt(i);
        uint8_t v = strtoul(String(c).c_str(), NULL, 16);
        for (int b = 3; b >= 0; b--) {
            out += ((v >> b) & 1) ? '1' : '0';
        }
    }
    return out;
}

int parityEven(String bits) {
    int c = 0;
    for (int i = 0; i < bits.length(); i++)
        if (bits[i] == '1') c++;
    return (c % 2 == 0) ? 0 : 1;
}

int parityOdd(String bits) {
    return parityEven(bits) ? 0 : 1;
}


String reverseBits(String in) {
    String out = "";
    for (int i = in.length() - 1; i >= 0; i--) out += in[i];
    return out;
}


String makeWiegand32(String uidHex) {

    // 1. UID als 32-Bit Zahl
    uint32_t uid = strtoul(uidHex.c_str(), NULL, 16);

    // 2. Byte-Swap rückwärts (damit Decoder wieder zurückswappt)
    uint32_t swapped =
        ((uid & 0x000000FF) << 24) |
        ((uid & 0x0000FF00) << 8)  |
        ((uid & 0x00FF0000) >> 8)  |
        ((uid & 0xFF000000) >> 24);

    // 3. wieder in HEX
    char buf[20];
    sprintf(buf, "%08X", swapped);

    // 4. in BIN (MSB-first)
    String bin = hexToBin(String(buf));

    return bin; // 32 Bits
}




String makeWiegand34(String uidHex) {

    // 1. UID → BIN (MSB-first)
    String uidBin = hexToBin(uidHex);

    while (uidBin.length() < 32)
        uidBin = "0" + uidBin;

    // 2. Byte-Swap rückwärts (damit Decoder wieder zurückswappt)
    uint32_t uid = strtoul(uidHex.c_str(), NULL, 16);

    uint32_t swapped =
        ((uid & 0x000000FF) << 24) |
        ((uid & 0x0000FF00) << 8)  |
        ((uid & 0x00FF0000) >> 8)  |
        ((uid & 0xFF000000) >> 24);

    // 3. swapped UID wieder in BIN
    char buf[20];
    sprintf(buf, "%08X", swapped);
    String swappedHex = String(buf);

    String swappedBin = hexToBin(swappedHex);

    // 4. Parity über ORIGINAL (swapped) Bits
    String left  = swappedBin.substring(0, 16);
    String right = swappedBin.substring(16, 32);

    int p1 = parityEven(left);
    int p2 = parityOdd(right);

    // 5. Wiegand-34 zusammenbauen
    return String(p1) + swappedBin + String(p2);
}


String makeWiegand35(String uidHex) {

    // 1. UID als 32-Bit Zahl
    uint32_t uid = strtoul(uidHex.c_str(), NULL, 16);

    // 2. Byte-Swap rückwärts
    uint32_t swapped =
        ((uid & 0x000000FF) << 24) |
        ((uid & 0x0000FF00) << 8)  |
        ((uid & 0x00FF0000) >> 8)  |
        ((uid & 0xFF000000) >> 24);

    // 3. swapped UID → HEX
    char buf[20];
    sprintf(buf, "%08X", swapped);

    // 4. → BIN (MSB-first)
    String swappedBin = hexToBin(String(buf));

    // 5. Facility/Card nach HID Corporate 1000-35
    String facility = swappedBin.substring(0, 19);
    String card     = swappedBin.substring(19, 32);

    // 6. Paritybits
    int p1 = parityEven(facility);
    int p2 = parityOdd(card);

    // 7. Wiegand-35 zusammenbauen
    return String(p1) + facility + card + String(p2);
}
