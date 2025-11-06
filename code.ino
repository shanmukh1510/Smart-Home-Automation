#include "thingProperties.h"
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#define LED_PIN 6
#define FAN_PIN 8
#define SOIL_SENSOR_PIN A2
#define GAS_SENSOR_PIN A0
#define RAIN_SENSOR_ANALOG A1
#define RAIN_SENSOR_DIGITAL 7

// RFID Pins
#define SS_PIN 10
#define RST_PIN 9

// Servo Pin
#define SERVO_PIN 5

MFRC522 rfid(SS_PIN, RST_PIN);
Servo doorLock;

// Authorized UID 
byte authorizedUID[4] = {0x79, 0x1C, 0x5B, 0x61};

// Door angles
const int LOCKED_POS = 0;
const int UNLOCKED_POS = 90;
bool doorUnlocked = false;
unsigned long doorTimer = 0;
const unsigned long DOOR_OPEN_DURATION = 5000; // 5 seconds

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(LED_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(GAS_SENSOR_PIN, INPUT);
  pinMode(SOIL_SENSOR_PIN, INPUT);
  pinMode(RAIN_SENSOR_ANALOG, INPUT);
  pinMode(RAIN_SENSOR_DIGITAL, INPUT);

  doorLock.attach(SERVO_PIN);
  doorLock.write(LOCKED_POS);

  // Initialize Cloud Connection
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  Serial.println("SMART HOME SYSTEM INITIALIZED");
  Serial.println("RFID + SENSOR + IoT SYSTEM ACTIVE");
}

void loop() {
  ArduinoCloud.update(); // Keep IoT Cloud connected

  // --- RFID Access Control ---
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.print("Card UID: ");
    for (byte i = 0; i < rfid.uid.size; i++) {
      Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(rfid.uid.uidByte[i], HEX);
    }
    Serial.println();

    // Compare UID
    if (isAuthorized(rfid.uid.uidByte)) {
      Serial.println(" Access Granted – Door Unlocking...");
      doorLock.write(UNLOCKED_POS);
      doorUnlocked = true;
      doorTimer = millis();
    } else {
      Serial.println(" Access Denied");
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  // --- Auto Lock Timer ---
  if (doorUnlocked && (millis() - doorTimer >= DOOR_OPEN_DURATION)) {
    doorLock.write(LOCKED_POS);
    doorUnlocked = false;
    Serial.println("🔒 Door Locked Automatically");
  }

  // --- Sensor Readings ---
  int soilValue = analogRead(SOIL_SENSOR_PIN);
  int gasValue = analogRead(GAS_SENSOR_PIN);
  int rainAnalog = analogRead(RAIN_SENSOR_ANALOG);
  int rainDigital = digitalRead(RAIN_SENSOR_DIGITAL);

  int soilPercent = map(soilValue, 1023, 0, 0, 100);

  // --- Update IoT Cloud Variables ---
  soillevel = soilPercent;
  gaslevel = gasValue;
  rainlevel = rainAnalog;

  // --- Display Values in Serial Monitor ---
  Serial.print("Soil Moisture: ");
  Serial.print(soilPercent);
  Serial.println("%");

  Serial.print("Gas Sensor Value: ");
  Serial.println(gasValue);

  Serial.print("Rain Analog: ");
  Serial.print(rainAnalog);
  Serial.print(" | Digital: ");
  Serial.println(rainDigital);

  // --- Rain Condition ---
  if (rainDigital == LOW) {
    Serial.println("🌧 Rain Detected");
  } else {
    Serial.println("☀ No Rain");
  }

  // --- Gas Alert ---
  if (gasValue > 1000) {
    Serial.println("⚠ GAS LEAKAGE DETECTED ⚠");
    digitalWrite(LED_PIN, HIGH);
  } else {
    if (!light) digitalWrite(LED_PIN, LOW);
  }

  Serial.println("-----------------------------");
  delay(2000);
}

bool isAuthorized(byte *uid) {
  for (byte i = 0; i < 4; i++) {
    if (uid[i] != authorizedUID[i]) return false;
  }
  return true;
}

// ------------------ IoT CLOUD HANDLERS ------------------

void onLightChange() {
  if (light) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("💡 LED turned ON (from Cloud)");
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.println("💡 LED turned OFF (from Cloud)");
  }
}

void onFanChange() {
  if (fan) {
    digitalWrite(FAN_PIN, HIGH);
    Serial.println("🌀 Fan turned ON (from Cloud)");
  } else {
    digitalWrite(FAN_PIN, LOW);
    Serial.println("🌀 Fan turned OFF (from Cloud)");
  }
}

void onSoillevelChange() {
  // No manual input required, value updates automatically
}

void onGaslevelChange() {
  // No manual input required, value updates automatically
}

void onRainlevelChange() {
  // No manual input required, value updates automatically
}
