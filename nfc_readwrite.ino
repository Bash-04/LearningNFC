#include <Wire.h>
#include <Adafruit_PN532.h>

// Define I2C pins for ESP32
#define SDA_PIN 21
#define SCL_PIN 22
#define RST_PIN 32
#define IRQ_PIN 33

// Define page total
#define NTAG213 36
#define NTAG215 129
#define NTAG216 222

// Create an instance of the PN532 NFC library
// Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);
Adafruit_PN532 nfc(RST_PIN, IRQ_PIN);

// Define the Key structure
struct Key {
  uint8_t pwd[4];   // Assuming PACK is 4 bytes
  uint8_t pack[4];  // Assuming PACK is 4 bytes
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
    // uint8_t success = nfc.mifareclassic_ReadDataBlock(page, data);
    uint8_t success = nfc.ntag2xx_ReadPage(page, data);

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

void readNFC(uint8_t startPage, uint8_t numpages, uint8_t* uid, uint8_t* uidLength) {
  Serial.println("Reading...");
  if (readNFCUID(uid, uidLength)) {
    readNFCData(startPage, numpages);  // Read page to verify written data
  } else {
    Serial.println("Failed to read UID.");
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

void setKey(Key key) {
  byte pwdpage = 0x85;
  byte packpage = 0x86;

  // set password
  writeNFCData(pwdpage, key.pwd, 4);

  // set pack
  writeNFCData(packpage, key.pack, 4);
}

bool Auth(Key key) {
  uint8_t response[4];  // Response buffer (PACK + result)
  uint8_t responseLength = sizeof(response);

  // Send PWD_AUTH command with the current password
  uint8_t command[5] = { 0x1B, key.pwd[0], key.pwd[1], key.pwd[2], key.pwd[3] };

  Serial.println(nfc.inDataExchange(command, sizeof(command), response, &responseLength));

  if (!nfc.inDataExchange(command, sizeof(command), response, &responseLength)) {
    Serial.println("PWD_AUTH command failed.");
    return false;
  }
  Serial.println("Authentication successful.");
  return true;
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
  uint8_t writeconflock[4] = { 0x4, 0x0, 0x0, 0xFE };    // Data to write
  // uint8_t writeconflock[4] = { 0x4, 0x0, 0x0, 0x00 };    // Data to write   doesn't unlock
  // uint8_t writeconf3[4] = { 0x0, 0x5, 0x0, 0x0 };        // Data to write
  uint8_t pwd[4] = { 0x24, 0x4A, 0x7E, 0x64 };           // Data to write
  uint8_t pack[4] = { 0x00, 0x05, 0x00, 0x00 };          // Data to write
  // uint8_t pack[4] = { 0xaa, 0xaa, 0x00, 0x00 };          // Data to write
  uint8_t rst[4] = { 0x00, 0x00, 0x00, 0x00 };           // Data to write
  Key key;                                               // Correctly initialize the Key struct

  memcpy(key.pwd, pwd, sizeof(pwd));
  memcpy(key.pack, pack, sizeof(pack));

  // Read the UID before writing
  uint8_t uid[7];
  uint8_t uidLength;

  // Read NFC
  // readNFC(1, 134, uid, &uidLength);
  // readNFC(129, 6, uid, &uidLength);
  readNFC(131, 1, uid, &uidLength);

  // Write NFC - set password (key)
  Serial.println("Attempting to write data: ...");
  // if (readNFCUID(uid, &uidLength)) {
  //   // write commands
  //   setKey(key);
  // } else {
  //   Serial.println("Failed to read UID.");
  // }
  
  // Write NFC - set auth0 byte
  if (readNFCUID(uid, &uidLength)) {
    if (Auth(key)) {
      writeNFCData(0x83, writeconfunlock, 4);  // unlock
      writeNFCData(129, writeData, 4);  // unlock
      Serial.println("YES");
    } else {
      Serial.println("NO");
    }
  } else {
    Serial.println("failed to read UID.");
  }

  // Read NFC
  // readNFC(1, 134, uid, &uidLength);
  // readNFC(128, 7, uid, &uidLength);
  readNFC(131, 1, uid, &uidLength);

  delay(2000);  // Wait before the next loop
}
