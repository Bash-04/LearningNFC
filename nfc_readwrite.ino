#include <Wire.h>
#include <Adafruit_PN532.h>

// Define I2C pins for ESP32
#define SDA_PIN 21
#define SCL_PIN 22
#define RST_PIN 33
#define IRQ_PIN 32

// Define page total
#define NTAG213 36
#define NTAG215 129
#define NTAG216 222

// Create an instance of the PN532 NFC library
Adafruit_PN532 nfc(RST_PIN, IRQ_PIN);

// Define the Key structure
struct Key {
    uint8_t pass[4]; // Assuming PACK is 4 bytes
    uint8_t pack[4]; // Assuming PACK is 4 bytes
};

// Function to read the UID of the NFC tag
bool readNFCUID(uint8_t* uid, uint8_t* uidLength) {
  // Look for an NFC tag (ISO14443A)
  uint8_t success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, uidLength);

  if (success) {
    Serial.println("Found an NFC tag!");

    // Print the UID
    Serial.print("UID Length: ");
    Serial.print(*uidLength, DEC);
    Serial.println(" bytes");

    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < *uidLength; i++) {
      Serial.print(" 0x");
      Serial.print(uid[i], HEX);
    }
    Serial.println("");
    return true;
  }

  Serial.println("No NFC tag found");
  return false;
}

// Function to convert byte array to char array
void byteToCharArray(const uint8_t* byteArray, int byteArraySize, char* charArray) {
  for (int i = 0; i < byteArraySize; i++) {
    charArray[i] = (char)byteArray[i];  // Cast each byte to a char
  }
  charArray[byteArraySize] = '\0';  // Null-terminate the char array (important for strings)
}

// Function to read data starting from a specific page
void readNFCData(uint8_t startPage, uint8_t numPages) {
  uint8_t data[32];  // Buffer to hold the read data

  for (uint8_t page = startPage; page < (startPage + numPages); page++) {
    // Read 4 bytes from the NFC tag starting at the page
    uint8_t success = nfc.mifareclassic_ReadDataBlock(page, data);

    if (success) {
      Serial.print("Page ");
      Serial.print(page);
      Serial.print(": ");

      // Convert the read bytes to a char array
      char charArray[5];                    // 4 bytes + 1 for null terminator
      byteToCharArray(data, 4, charArray);  // Convert 4 bytes per page to char array

      // Print the char array (interpreted as a string)
      Serial.print("Data as chars: ");
      Serial.println(charArray);

      // Print the data in Hex
      for (int j = 0; j < 4; j++) {  // Each page has 4 bytes
        Serial.print(" 0x");
        Serial.print(data[j], HEX);
      }
      Serial.println();
    } else {
      Serial.print("Failed to read page ");
      Serial.println(page);
    }
  }
}

// Function to write data to a specific page
bool writeNFCData(uint8_t page, uint8_t* data, uint8_t dataSize) {
  // Ensure dataSize is 4 bytes for a single NTAG21xx page
  if (dataSize != 4) {
    Serial.println("Error: Data size must be 4 bytes for a single NTAG21xx page.");
    return false;
  }


  // Attempt to write the data to the specified page
  uint8_t success = nfc.ntag2xx_WritePage(page, data);

  // Check for success or failure
  if (success) {
    Serial.println("Succes!");
    return true;  // Return true if write was successful
  } else {
    Serial.print("Failed to write data, error code: ");
    Serial.println(success, HEX);
    return false;
  }
}

// Function to authenticate using PACK
int authenticate(uint8_t* res, Key* key) {
    bool flag_auth = true;
    for (int i = 0; i < 2; i++) {
        if (res[i] != key->pack[i]) {
            flag_auth = false;
            Serial.println("flag = false");
            break;
        }
    }
    return flag_auth ? 0 : -1;
}

void setup(void) {
  // Initialize I2C communication
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("NFC Reader starting...");

  // Initialize the NFC reader
  nfc.begin();

  // Check if the PN532 is detected
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN532");
    while (1)
      ;  // halt
  }

  // Print the version info
  Serial.print("Found PN532 with firmware version: ");
  Serial.println((versiondata >> 16) & 0xFF, HEX);

  // Configure the NFC reader
  nfc.SAMConfig();
  Serial.println("Waiting for an NFC card...");
}

void loop(void) {
  uint8_t writeData[4] = { 0x1, 0x2, 0x1, 0x1 };         // Data to write
  uint8_t writeconf[4] = { 0x0, 0x0, 0x0, 0xBD };        // Data to write
  uint8_t writeconfunlock[4] = { 0x4, 0x0, 0x0, 0xFF };  // Data to write
  uint8_t writeconflock[4] = { 0x4, 0x0, 0x0, 0x0 };     // Data to write
  uint8_t writeconf3[4] = { 0x0, 0x5, 0x0, 0x0 };        // Data to write
  uint8_t pwd[4] = { 0x24, 0x4A, 0x7E, 0x64 };           // Data to write
  uint8_t rst[4] = { 0x00, 0x00, 0x00, 0x00 };           // Data to write
  uint8_t pack[4] = { 0x00, 0x05, 0x00, 0x00 };          // Data to write
  Key key = {{0x24, 0x4A, 0x7E, 0x64}, {0x00, 0x05, 0x00, 0x00}};  // Correctly initialize the Key struct
  
  // Read the UID before writing
  uint8_t uid[7];
  uint8_t uidLength;

  // write NFC config slots
  // if (readNFCUID(uid, &uidLength)) {
  //   Serial.println("writing");
  //   writeNFCData(130, writeconf, 4);
  //   writeNFCData(131, writeconf2, 4);
  //   writeNFCData(132, writeconf3, 4);

  //   nfc.ntag2xx_WritePage(133, pass);

  // } else {
  //   Serial.println("Failed to read UID.");
  // }

  // Read NFC
  if (readNFCUID(uid, &uidLength)) {
    readNFCData(1, 134);  // Read page to verify written data
  } else {
    Serial.println("Failed to read UID.");
  }

  // Write NFC
  Serial.println("Attempting to write data: ...");
  if (readNFCUID(uid, &uidLength)) {
    // for (int i = 4; i <= NTAG215; i++) {
    //   writeNFCData(i, writeData, 4);
    // }

    // remove password
    authenticate(pack, &key);

    // set conf pages
    writeNFCData(131, writeconfunlock, 4);
    
    // set pwd
    writeNFCData(134, rst, 4);  // Set PACK
    writeNFCData(133, rst, 4);  // Set password (locks the tag)


  } else {
    Serial.println("Failed to read UID.");
  }

  // Read NFC
  if (readNFCUID(uid, &uidLength)) {
    readNFCData(130, 5);  // Read page to verify written data
  } else {
    Serial.println("Failed to read UID.");
  }

  delay(1000);  // Wait before the next loop
}
