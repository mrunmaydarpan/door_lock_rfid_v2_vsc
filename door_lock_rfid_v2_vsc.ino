#include <Adafruit_NeoPixel.h>
#include <Button.h>
#include <EEPROM.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Timer.h>
#include <Wire.h>

byte locked[] = {B01110, B10001, B10001, B10001,
                 B11111, B11111, B11011, B11111};
byte unlocked[] = {B01110, B10001, B10000, B10000,
                   B11111, B11111, B11011, B11111};
byte phone[] = {B00001, B11111, B10001, B10001, B11111, B11111, B11111, B11111};
byte cross[] = {B00000, B00000, B00000, B00000, B00000, B00101, B00010, B00101};

Button inLockB(23);
#define relay 24 // Set Relay Pin
#define wipeB 25 // Button pin for WipeMode
#define inLED 22
#define buzz 28
#define SS_PIN 53
#define RST_PIN 48
#define PIN 49 // led Pin
#define r1 45
#define r2 43
#define r3 41
#define r4 39
#define c1 38
#define c2 40
#define c3 42
#define c4 44

#define NUMPIXELS 8

LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
MFRC522 mfrc522(SS_PIN, RST_PIN);
Timer t;

boolean match = false;       // initialize card match to false
boolean programMode = false; // initialize programming mode to false
boolean replaceMaster = false;

uint8_t successRead; // Variable integer to keep if we have Successful Read from
                     // Reader
byte storedCard[4];  // Stores an ID read from EEPROM
byte readCard[4];    // Stores scanned ID read from RFID Module
byte masterCard[4];  // Stores master card's ID read from EEPROM

char password[4];
char initial_password[4], new_password[4];

int i = 0;
char key = 0;
int lockState = 0;
const byte rows = 4;
const byte columns = 4;

char hexaKeys[rows][columns] = {{'1', '2', '3', 'A'},
                                {'4', '5', '6', 'B'},
                                {'7', '8', '9', 'C'},
                                {'*', '0', '#', 'D'}};

byte row_pins[rows] = {r1, r2, r3, r4};
byte column_pins[columns] = {c1, c2, c3, c4};

Keypad keypad =
    Keypad(makeKeymap(hexaKeys), row_pins, column_pins, rows, columns);

unsigned long previousMillis = 0;
const long interval = 500;
int state;

////////////////////////////// Setup ////////////////////////////////

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  Serial.println("SYSTEM RESTART");
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.createChar(1, phone);
  lcd.createChar(2, locked);
  lcd.createChar(3, unlocked);
  lcd.createChar(4, cross);

  pinMode(inLED, OUTPUT);
  char md[] = "MDtronics";
  for (int l = 0; l < sizeof(md) - 1; l++) {
    lcd.setCursor(l, 0);
    lcd.print(md[l]);
    pixels.setPixelColor(l, pixels.Color(0, 255, 0));
    pixels.show();
    digitalWrite(inLED, !digitalRead(inLED));
    delay(300);
  }
  pixels.clear();
  lcd.setCursor(0, 1);
  lcd.print("DOOR LOCK SYSTEM");
  delay(1000);
  initial_Password();
  lcd.clear();
  pinMode(wipeB, INPUT_PULLUP);
  pinMode(relay, OUTPUT);
  digitalWrite(inLED, LOW);
  inLockB.begin();
  SPI.begin();
  mfrc522.PCD_Init();
  ShowReaderDetails(); // Show details of PCD - MFRC522 Card Reader details
  WipeAll();           // Pressed while (powered on) it wipes EEPROM
  checkMasterID();     // Check if master card defined, if not let user choose a
                       // master card
  Serial.println(F("-------------------"));
  Serial.println(F("Master Card's UID"));
  for (uint8_t i = 0; i < 4; i++) {     // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2 + i); // Write it to masterCard
    Serial.print(masterCard[i], HEX);
  }
  Serial.println("");
  Serial.println(F("-------------------"));
  Serial.println(F("Everything is ready"));
  Serial.println(F("Waiting PICCs to be scanned"));
  noTone(buzz);
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 255, 255));
    pixels.show();
  }
  t.every(500, blink);
}

/////////////////////////// Main Loop ///////////////////////////////

void loop() {
  lcdDefault();
  t.update();
  do {
    successRead = getID(); // sets successRead to 1 when we get read from reader
    changeMaster();
    passLoop();
    t.update();
    if (programMode) {
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 255, 255));
        pixels.show();
      }
      lcd.setCursor(0, 0);
      lcd.print("  PROGRAM MODE  ");
      lcd.setCursor(0, 1);
      lcd.print("-SCAN YOUR CARD-");
    } else {
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 255, 0));
        pixels.show();
      }
    }
  } while (!successRead); // the program will not go further while you are not
                          // getting a successful read
  if (programMode) {
    if (isMaster(readCard)) // When in program mode check First If master card
                            // scanned again to exit program mode
    {
      Serial.println(F("Master Card Scanned"));
      Serial.println(F("Exiting Program Mode"));
      Serial.println(F("-----------------------------"));
      programMode = false;
      lcd.clear();
      return;
    } else {
      if (findID(readCard)) // If scanned card is known delete it
      {
        Serial.println(F("I know this PICC, removing..."));
        deleteID(readCard);
        Serial.println("-----------------------------");
        Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
      } else { // If scanned card is not known add it
        Serial.println(F("I do not know this PICC, adding..."));
        writeID(readCard);
        Serial.println(F("-----------------------------"));
        Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
      }
    }
  } else {
    if (isMaster(readCard)) // If scanned card's ID matches Master Card's ID -
    {                       // enter program mode
      programMode = true;
      Serial.println(F("Hello Master - Entered Program Mode"));
      uint8_t count = EEPROM.read(0); // Read the first Byte of EEPROM that
      Serial.print(F("I have "));     // stores the number of ID's in EEPROM
      Serial.print(count);
      Serial.print(F(" record(s) on EEPROM"));
      Serial.println("");
      Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
      Serial.println(F("Scan Master Card again to Exit Program Mode"));
      Serial.println(F("-----------------------------"));
    } else {
      if (findID(readCard)) // If not, see if the card is in the EEPROM
      {
        for (int i = 0; i < NUMPIXELS; i++) {
          pixels.setPixelColor(i, pixels.Color(0, 255, 0));
          pixels.show();
        }
        Serial.println(F("Welcome, You shall pass"));
        granted(); // Open the door lock for 300 ms
      } else       // If not, show that the ID was not valid
      {
        for (int i = 0; i < NUMPIXELS; i++) {
          pixels.setPixelColor(i, pixels.Color(255, 0, 0));
          pixels.show();
        }
        Serial.println(F("You shall not pass"));
        denied();
      }
    }
  }
}

//////////////////////// Get PICC's UID /////////////////////////////

uint8_t getID() {
  // Getting ready for Reading PICCs
  if (!mfrc522.PICC_IsNewCardPresent()) // If a new PICC placed to RFID reader
                                        // continue
  {
    return 0;
  }
  if (!mfrc522.PICC_ReadCardSerial()) // Since a PICC placed get Serial and
                                      // continue
  {
    return 0;
  }
  Serial.println(F("Scanned PICC's UID:"));

  for (uint8_t i = 0; i < 4; i++) {
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

void ShowReaderDetails() // Get the MFRC522 software version
{
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if ((v == 0x00) || (v == 0xFF)) {
    lcd.clear();
    lcd.print("RFID FAILED");
    lcd.setCursor(0, 1);
    lcd.print("CHECK CONNECTION");
    Serial.println(F(
        "WARNING: Communication failure, is the MFRC522 properly connected?"));
    Serial.println(F("SYSTEM HALTED: Check connections."));
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(255, 0, 0));
      pixels.show();
    }
    while (true)
      ; // do not go further
  }
}

//////////////////////// Read an ID from EEPROM /////////////////////

void readID(uint8_t number) {
  uint8_t start = (number * 4) + 2; // Figure out starting position
  for (uint8_t i = 0; i < 4; i++)   // Loop 4 times to get the 4 Bytes
  {
    storedCard[i] =
        EEPROM.read(start + i); // Assign values read from EEPROM to array
  }
}

//////////////////////// Check Bytes   //////////////////////////////

boolean checkTwo(byte a[], byte b[]) {
  if (a[0] != 0)  // Make sure there is something in the array first
    match = true; // Assume they match at first
  for (uint8_t k = 0; k < 4; k++) {
    if (a[k] != b[k]) // IF a != b then set match = false, one fails, all fail
      match = false;
  }
  if (match) // Check to see if if match is still true
  {
    return true; // Return true
  } else {
    return false; // Return false
  }
}

////////////////////////  Find Slot //////////////////////////////////////

uint8_t findIDSLOT(byte find[]) {
  uint8_t count = EEPROM.read(0);      // Read the first Byte of EEPROM that
  for (uint8_t i = 1; i <= count; i++) // Loop once for each EEPROM entry
  {
    readID(i); // Read an ID from EEPROM, it is stored in storedCard[4]
    if (checkTwo(find,
                 storedCard)) // Check to see if the storedCard read from EEPROM
    {
      // is the same as the find[] ID card passed
      return i; // The slot number of the card
      break;    // Stop looking we found it
    }
  }
}

////////////////////// Find ID From EEPROM //////////////////////////

boolean findID(byte find[]) {
  uint8_t count = EEPROM.read(0);      // Read the first Byte of EEPROM that
  for (uint8_t i = 1; i <= count; i++) // Loop once for each EEPROM entry
  {
    readID(i); // Read an ID from EEPROM, it is stored in storedCard[4]
    if (checkTwo(
            find,
            storedCard)) { // Check to see if the storedCard read from EEPROM
      return true;
      break; // Stop looking we found it
    } else {
      // If not, return false
    }
  }
  return false;
}

/////////////////// Write Success to EEPROM /////////////////////////
// Flashes the green LED 3 times to indicate a successful write to EEPROM

void successWrite() {
  lcd.clear();
  lcd.print("CARD ADDED");
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 255, 0));
    pixels.show();
  }
  delay(1000);
  pixels.clear();
}

////////////////////// Write Failed to EEPROM ///////////////////////
// Flashes the red LED 3 times to indicate a failed write to EEPROM

void failedWrite() {
  lcd.clear();
  lcd.print("FAILED TO WRITE");
  pixels.clear();
  delay(200);
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    pixels.show();
  }
  delay(200);
  pixels.clear();
  delay(200);
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    pixels.show();
  }
  delay(200);
  pixels.clear();
  delay(200);
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    pixels.show();
  }
  delay(200);
}

//////////////////// Success Remove UID From EEPROM  ////////////////
// Flashes the blue LED 3 times to indicate a success delete to EEPROM

void successDelete() {
  lcd.clear();
  lcd.print("CARD DELETED");

  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    pixels.show();
  }

  delay(1000);
  pixels.clear();
}

////////////////////// Check readCard IF is masterCard
//////////////////////////////////////
// Check to see if the ID passed is the master programing card

boolean isMaster(byte test[]) {
  if (checkTwo(test, masterCard))
    return true;
  else
    return false;
}

bool monitorWipeButton(uint32_t interval) {
  uint32_t now = (uint32_t)millis();
  while ((uint32_t)millis() - now < interval) {
    if (((uint32_t)millis() % 500) == 0) // check on every half a second
    {
      if (digitalRead(wipeB) != LOW)
        return false;
    }
  }
  return true;
}

void beep() {
  tone(buzz, 3000, 100);
  delay(100);
  noTone(buzz);
  delay(100);
  tone(buzz, 3000, 100);
  delay(100);
  noTone(buzz);
}

void WipeAll() {
  if (digitalRead(wipeB) == LOW) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(255, 0, 0));
      pixels.show();
    }

    lcd.clear();
    lcd.print("WIPE ALL DATA");

    Serial.println(F("Wipe Button Pressed"));
    Serial.println(F("You have 10 seconds to Cancel"));
    Serial.println(F("This will be remove all records and cannot be undone"));

    bool buttonState =
        monitorWipeButton(10000); // Give user enough time to cancel operation
    if (buttonState == true &&
        digitalRead(wipeB) == LOW) // If button still be pressed, wipe EEPROM
    {
      lcd.clear();
      lcd.home();
      lcd.println("WIPING DATA     ");
      Serial.println(F("Starting Wiping EEPROM"));

      for (uint16_t x = 0; x < EEPROM.length();
           x++) // Loop end of EEPROM address
      {
        if (EEPROM.read(x) == 0) // If EEPROM address 0
        {
          // do nothing, already clear, go to the next address in order to save
          // time and reduce writes to EEPROM
        } else {
          EEPROM.write(x, 0); // if not write 0 to clear, it takes 3.3mS
        }
      }

      Serial.println(F("EEPROM Successfully Wiped"));
      pixels.clear();
      delay(200);
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 0, 0));
        pixels.show();
      }
      delay(200);
      pixels.clear();
      delay(200);
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 0, 0));
        pixels.show();
      }
      delay(200);
      pixels.clear();
    } else {
      Serial.println(
          F("Wiping Cancelled")); // Show some feedback that the wipe button did
                                  // not pressed for 15 seconds
      pixels.clear();
    }
  }
}

void changeMaster() {
  if (digitalRead(wipeB) == LOW) // Check if button is pressed
  {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(255, 0, 0));
      pixels.show();
    }

    Serial.println(F("Wipe Button Pressed"));
    Serial.println(F("Master Card will be Erased! in 10 seconds"));

    lcd.clear();
    lcd.home();
    lcd.print("MASTER ERASE");
    lcd.setCursor(0, 1);
    lcd.print("STARTED");

    bool buttonState =
        monitorWipeButton(10000); // Give user enough time to cancel operation
    if (buttonState == true &&
        digitalRead(wipeB) == LOW) // If button still be pressed, wipe EEPROM
    {
      EEPROM.write(1, 0); // Reset Magic Number.
      lcd.clear();
      lcd.print("MASTER ERASED");
      lcd.setCursor(0, 1);
      lcd.print("PLEASE RESET");
      Serial.println(F("Master Card Erased from device"));
      Serial.println(F("Please reset to re-program Master Card"));
      while (1)
        ;
    }
    Serial.println(F("Master Card Erase Cancelled"));
    lcd.clear();
    lcd.print("MASTER ERASE");
    lcd.setCursor(0, 1);
    lcd.print("CANCELED");
    delay(1000);
    lcd.clear();
    lcdDefault();
  }
}

void checkMasterID() {
  if (EEPROM.read(1) != 143) {
    lcd.clear();
    lcd.print("NO MASTER CARD");
    lcd.setCursor(0, 1);
    lcd.print("SCAN TO ADD ONE");
    Serial.println(F("No Master Card Defined"));
    Serial.println(F("Scan A PICC to Define as Master Card"));
    do {
      successRead = getID(); // sets successRead to 1 when we get read from
                             // reader otherwise 0
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 255));
        pixels.show();
      }
      delay(200);
      pixels.clear();
      delay(200);
    } while (!successRead); // Program will not go further while you not get a
                            // successful read
    for (uint8_t j = 0; j < 4; j++) // Loop 4 times
    {
      EEPROM.write(2 + j, readCard[j]); // Write scanned PICC's UID to EEPROM,
                                        // start from address 3
    }
    EEPROM.write(1, 143);
    lcd.clear();
    lcd.print("FOUND A CARD");
    lcd.setCursor(0, 1);
    lcd.print("ADDING AS MASTER");
    Serial.println(F("Master Card Defined"));
    delay(2000);
    lcd.clear();
  }
}

///////////////////////// Remove ID from EEPROM //////////////////////

void deleteID(byte a[]) {
  if (!findID(a)) // Before we delete from the EEPROM, check to see if we have
                  // this card!
  {
    failedWrite(); // If not
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  } else {
    uint8_t num = EEPROM.read(0); // Get the numer of used spaces, position 0
                                  // stores the number of ID cards
    uint8_t slot;                 // Figure out the slot number of the card
    uint8_t
        start; // = ( num * 4 ) + 6; // Figure out where the next slot starts
    uint8_t looping; // The number of times the loop repeats
    uint8_t j;
    uint8_t count = EEPROM.read(
        0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT(a); // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;                        // Decrement the counter by one
    EEPROM.write(0, num);         // Write the new count to the counter
    for (j = 0; j < looping; j++) // Loop the card shift times
    {
      EEPROM.write(
          start + j,
          EEPROM.read(
              start + 4 +
              j)); // Shift the array values to 4 places earlier in the EEPROM
    }
    for (uint8_t k = 0; k < 4; k++) // Shifting loop
    {
      EEPROM.write(start + j + k, 0);
    }
    successDelete();
    Serial.println(F("Succesfully removed ID record from EEPROM"));
  }
}

///////////////////////// Add ID to EEPROM ///////////////////////////

void writeID(byte a[]) {
  if (!findID(a)) // Before we write to the EEPROM, check to see if we have seen
                  // this card before!
  {
    uint8_t num = EEPROM.read(0);  // Get the numer of used spaces, position 0
                                   // stores the number of ID cards
    uint8_t start = (num * 4) + 6; // Figure out where the next slot starts
    num++;                         // Increment the counter by one
    EEPROM.write(0, num);          // Write the new count to the counter
    for (uint8_t j = 0; j < 4; j++) {
      EEPROM.write(
          start + j,
          a[j]); // Write the array values to EEPROM in the right position
    }
    successWrite();
    Serial.println(F("Succesfully added ID record to EEPROM"));
  } else {
    failedWrite();
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
}

void lcdDefault() {
  lcd.home();
  lcd.print("ENTER PASSWORD: ");
  lcd.setCursor(15, 1);
  lcd.write(2);
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 255, 0));
    pixels.show();
  }
}

void blink() { digitalWrite(inLED, !digitalRead(inLED)); }