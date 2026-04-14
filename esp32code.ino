#include <WiFi.h>
#include <HTTPClient.h>


const char* ssid = "Catherine's Galaxy A13 5G";
const char* password = "ganda ganda ni cathy";


const char* apiURL = "https://dashboard.philsms.com/api/v3/sms/send";
const char* apiToken = "2198|6c95ErZiJavekXXi7AkVJvh0OhBhEM04v3KFqnqqcdf72119";


HardwareSerial arduinoSerial(2); 


String lastCommand = "";
unsigned long lastSMSTime = 0;
const unsigned long smsCooldown = 10000; 

unsigned long lastWiFiCheck = 0;
const unsigned long wifiInterval = 5000;

void setup() {
  Serial.begin(115200);


  arduinoSerial.begin(9600, SERIAL_8N1, 16, 17);

  connectToWiFi();
}

void loop() {
  maintainWiFi();

  if (arduinoSerial.available()) {
    String command = arduinoSerial.readStringUntil('\n');
    command.trim();

    if (command.length() > 0) {
      Serial.println("Received: " + command);

      // Prevent duplicate spam
      if (command != lastCommand || millis() - lastSMSTime > smsCooldown) {

        if (command == "FIRE") {
          sendSMS("FIRE DETECTED! Check area immediately.");
        } 
        else if (command.startsWith("GAS:")) {
          String gasValue = command.substring(4);
          sendSMS("GAS LEAK! Level: " + gasValue);
        }

        lastCommand = command;
        lastSMSTime = millis();

      } else {
        Serial.println("Duplicate ignored");
      }
    }
  }
}


void maintainWiFi() {
  if (millis() - lastWiFiCheck > wifiInterval) {
    lastWiFiCheck = millis();

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Reconnecting WiFi...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
  }
}


void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect!");
  }
}


void sendSMS(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(apiURL);
    http.addHeader("Authorization", String("Bearer ") + apiToken);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");


    String payload = "{";
    payload += "\"recipient\":\"639953317582\",";
    payload += "\"sender_id\":\"PhilSMS\",";
    payload += "\"type\":\"plain\",";
    payload += "\"message\":\"" + message + "\"";
    payload += "}";

    Serial.println("Sending SMS...");
    int code = http.POST(payload);

    if (code > 0) {
      Serial.print("HTTP Code: ");
      Serial.println(code);

      String response = http.getString();
      Serial.println(response);

      if (code == 200 || code == 201) {
        Serial.println("SMS SENT");
      } else {
        Serial.println("SMS FAILED");
      }

    } else {
      Serial.print("HTTP Error: ");
      Serial.println(code);
    }

    http.end();
  } else {
    Serial.println("WiFi lost, reconnecting...");
    connectToWiFi();
  }
}
