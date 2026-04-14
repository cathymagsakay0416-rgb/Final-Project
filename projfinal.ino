#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>


#define flamePin 7
#define ledPin 2
#define buzzerPin 3
#define relayPin 8
#define mq135Pin A0


LiquidCrystal_I2C lcd(0x27, 16, 2); 
SoftwareSerial espSerial(10, 11); 


enum State {
  SAFE,
  FIRE,
  GAS
};

State currentState = SAFE;
State lastState = SAFE;


unsigned long lastReadTime = 0;
const int readInterval = 300;


int gasBaseline = 0;       
int gasThreshold = 0;      
#define GAS_OFFSET 15       
#define GAS_CONSECUTIVE 4   
int gasCounter = 0;


int readGasAverage(int samples = 20) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(mq135Pin);
    delay(10); // slight delay for stability
  }
  return sum / samples;
}

void setup() {
  pinMode(flamePin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); 
  delay(50); 

  Serial.begin(9600);
  espSerial.begin(9600);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("Fire & Smoke");
  lcd.setCursor(0,1);
  lcd.print("Detector Ready");
  delay(2000);
  lcd.clear();

  digitalWrite(relayPin, HIGH); 


  lcd.setCursor(0,0);
  lcd.print("Warming up sensor");
  lcd.setCursor(0,1);
  lcd.print("Please wait...");
  long sum = 0;
  unsigned long warmUpStart = millis();
  const unsigned long warmUpTime = 30000; // 10 min warm-up
  while (millis() - warmUpStart < warmUpTime) {
    digitalWrite(relayPin, HIGH);
    int val = readGasAverage();
    sum += val;
    Serial.print("Warm-up Gas: ");
    Serial.println(val);
    delay(500);
  }

  gasBaseline = sum / (warmUpTime / 500); 
  gasThreshold = gasBaseline + GAS_OFFSET;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Sensor Ready");
  lcd.setCursor(0,1);
  lcd.print("Threshold: ");
  lcd.print(gasThreshold);
  delay(2000);
  lcd.clear();

  Serial.print("Gas Baseline: ");
  Serial.println(gasBaseline);
  Serial.print("Gas Threshold: ");
  Serial.println(gasThreshold);
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastReadTime >= readInterval) {
    lastReadTime = currentMillis;

    int gasValue = readGasAverage();
    int flame = digitalRead(flamePin);

    Serial.print("Flame: ");
    Serial.print(flame);
    Serial.print(" | Gas: ");
    Serial.println(gasValue);

    // Determine state
    if (flame == LOW) {
      currentState = FIRE;
      gasCounter = 0; // reset counter
    } 
    else if (gasValue > gasThreshold) {
      gasCounter++;
      if (gasCounter >= GAS_CONSECUTIVE) {
        currentState = GAS;
      } else {
        currentState = SAFE;
      }
    } 
    else {
      currentState = SAFE;
      gasCounter = 0; 
    }

    if (currentState != lastState || currentState == SAFE) {
      lcd.clear();

      switch (currentState) {
        case FIRE:
          digitalWrite(ledPin, HIGH);
          digitalWrite(buzzerPin, HIGH);
          digitalWrite(relayPin, HIGH); // Fan OFF

          lcd.setCursor(0,0);
          lcd.print("FIRE DETECTED!");
          lcd.setCursor(0,1);
          lcd.print("Check Area");

          Serial.println("Sending FIRE");
          espSerial.println("FIRE");
          break;

        case GAS:
          digitalWrite(ledPin, HIGH);
          digitalWrite(buzzerPin, HIGH);
          digitalWrite(relayPin, LOW);  

          lcd.setCursor(0,0);
          lcd.print("GAS DETECTED!");
          lcd.setCursor(0,1);
          lcd.print("Fan ON");

          Serial.print("Sending GAS: ");
          Serial.println(gasValue);
          espSerial.print("GAS:");
          espSerial.println(gasValue);
          break;

        case SAFE:
          digitalWrite(ledPin, LOW);
          digitalWrite(buzzerPin, LOW);
          digitalWrite(relayPin, HIGH); 

          lcd.setCursor(0,0);
          lcd.print("System Safe");
          lcd.setCursor(0,1);
          lcd.print("No Danger");

          Serial.println("System Safe");
          break;
      }

      lastState = currentState; 
    }
  }
}
