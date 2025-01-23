#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include "webpage.h"

#define TRIGER 5
#define MIRROR_LAMP 16
#define SENSOR_WENT 2
#define SENSOR_LIGHT 4

const char* ssid = "Patsuk";  // Set your WiFi network name (SSID)
const char* password = "@@@@@@@@";  // Set your WiFi password

const String MIRROR_DOMAIN = "http://192.168.0.132/";

const int eepromAddrDelay = 0;  // Address in EEPROM to store the delay
int delayMinutes = 5;
const int eepromAddrDelayByLights = sizeof(int);
int delayByLightsMinutes = 3;
const int epromLength = sizeof(int) * 2;

unsigned long getDelayMillis() {
  return delayMinutes * 60 * 1000;
}
unsigned long getDelayByLightsMillis() {
  return delayByLightsMinutes * 60 * 1000;
}

ESP8266WebServer server(80);  // Create an instance of the web server on port 80

bool wentSensorReading = false;
bool lightSensorReading = false;
bool previousWentSensorReading = false;
bool previousLightSensorReading = false;

bool mirrorHeaterState = false;
bool mirrorLampState = false;

unsigned long delayedToggleMillis = 0;

void setup() {
  Serial.begin(9600);  // Set the serial speed to 9600
  pinMode(SENSOR_WENT, INPUT_PULLUP);
  pinMode(SENSOR_LIGHT, INPUT_PULLUP);
  pinMode(TRIGER, OUTPUT);
  
  digitalWrite(TRIGER, LOW);
  
  pinMode(MIRROR_LAMP, OUTPUT);
  digitalWrite(MIRROR_LAMP, LOW);
  
  delay(10);

  // Connect to Wi-Fi
  Serial.println();

  setupServer();

  timer1_attachInterrupt(clockUpdate);
  timer1_write(90000);  // ~ 0.5 second interval
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_LOOP);

  // Initialize delay from EEPROM
  readFromEEPROM();
}

void loop() {
  server.handleClient();

  if (previousWentSensorReading != wentSensorReading) {
    previousWentSensorReading = wentSensorReading;
    mirrorHeaterState = wentSensorReading;
    onWentSensorChange();
    updateMirrorHeater();
  }  

  if (previousLightSensorReading != lightSensorReading) {
    previousLightSensorReading = lightSensorReading;
    mirrorLampState = lightSensorReading;
    onLightsSensorChange();
    updateMirrorLight();
  }  

  if (delayedToggleMillis > 0 && millis() > delayedToggleMillis) {
    delayedToggleMillis = 0;
    if (wentSensorReading) {
      toggleWent();
    }
  }

    static unsigned long lastFetchMillis = 0; 
    unsigned long currentMillis = millis();
    if (currentMillis - lastFetchMillis >= 5000) {
        lastFetchMillis = currentMillis;
        fetchMirrorStates();
    }
}

void onWentSensorChange() {
  if(lightSensorReading) {
    delayedToggleMillis = 0;
  } else {
      if (wentSensorReading) {
        delayedToggleMillis = millis() + getDelayMillis();
      } else {
        delayedToggleMillis = 0;
      }
  }
}

void onLightsSensorChange() {
  if(lightSensorReading) {  
    delayedToggleMillis = 0;
  } else {
    if (wentSensorReading) {
      delayedToggleMillis = millis() + getDelayByLightsMillis();
    } else {
      delayedToggleMillis = 0;
    }
  }
}

void setupServer() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

  Serial.print("Server IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.println("HTTP server started");

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", webpage);
  });

server.on("/getMemoryData", HTTP_GET, []() {
    String delayToToggle = "null";
    if (wentSensorReading && delayedToggleMillis > 0) {
        delayToToggle = String(delayedToggleMillis - millis());
    }
    String json = "{";
    json += "\"wentSensorReading\": " + String(wentSensorReading);
    json += ", \"lightSensorReading\": " + String(lightSensorReading);
    json += ", \"delayToToggle\": " + delayToToggle;
    json += ", \"mirrorHeaterState\":" + String(mirrorHeaterState);
    json += ", \"mirrorLampState\":" + String(mirrorLampState);
    json += ", \"delayMinutes\":" + String(delayMinutes);
    json += ", \"delayByLightsMinutes\":" + String(delayByLightsMinutes);
    json += "}";
    
    server.send(200, "application/json", json);
});

  server.on("/changeState", HTTP_GET, []() {
    toggleWent();
  });

  server.on("/setDelay", HTTP_GET, []() {
    if (server.hasArg("value")) {
      int newDelay = server.arg("value").toInt();
      Serial.println(newDelay);
      if (newDelay >= 1 && newDelay <= 60) {
        delayMinutes = newDelay;
        saveToEEPROM();
      }
    }
    server.send(200, "plain/text", "Ok");
  });

  server.on("/setDelayByLights", HTTP_GET, []() {
    if (server.hasArg("value")) {
      int newDelay = server.arg("value").toInt();
      Serial.println(newDelay);
      if (newDelay >= 1 && newDelay <= 60) {
        delayByLightsMinutes = newDelay;
        saveToEEPROM();
      }
    }
    server.send(200, "plain/text", "Ok");
  });

  server.on("/toggleMirrorHeater", HTTP_GET, []() {
      toggleMirrorHeater();
      server.send(200, "text/plain", "Toggled mirror heater");
  });

  server.on("/toggleMirrorLight", HTTP_GET, []() {
      toggleMirrorLight();
      server.send(200, "text/plain", "Toggled mirror light");
  });
}

bool mirrorRequest(String endpoint) {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient wifiClient;
        HTTPClient http;
        http.begin(wifiClient, MIRROR_DOMAIN + endpoint);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            Serial.println("changeState called successfully");
            http.end();
            return true;
        } else {
            Serial.printf("changeState request failed with code: %d\n", httpCode);
        }
        http.end();
    } else {
        Serial.println("Wi-Fi not connected");
    }
    return false;
}

void fetchMirrorStates() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient wifiClient;
        HTTPClient http;
        http.begin(wifiClient, MIRROR_DOMAIN + "getState");
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.println("Response from getState: " + payload);

            // Parse JSON to extract the states
            DynamicJsonDocument doc(256);  // Adjust size if needed
            DeserializationError error = deserializeJson(doc, payload);
            if (error) {
                Serial.print("JSON deserialization failed: ");
                Serial.println(error.c_str());
                return;
            }

            // Assign values from JSON to the booleans
            if (doc.containsKey("mirrorHeaterState")) {
                mirrorHeaterState = doc["mirrorHeaterState"].as<int>() != 0;  // Convert int to boolean
            }

            if (doc.containsKey("mirrorLightState")) {
                mirrorLampState = doc["mirrorLightState"].as<int>() != 0;  // Convert int to boolean
            }

            Serial.print("Mirror Heater State: ");
            Serial.println(mirrorHeaterState ? "ON" : "OFF");
            Serial.print("Mirror Lamp State: ");
            Serial.println(mirrorLampState ? "ON" : "OFF");
        } else {
            Serial.printf("getState request failed with code: %d\n", httpCode);
        }
        http.end();
    } else {
        Serial.println("Wi-Fi not connected");
    }
}


void clockUpdate() {
  wentSensorReading = !digitalRead(SENSOR_WENT);
  lightSensorReading = !digitalRead(SENSOR_LIGHT);
  Serial.print("Went sensor = ");
  Serial.print(wentSensorReading);
  Serial.print(" Light sensor = ");
  Serial.print(lightSensorReading);
  Serial.print(" delay = ");
  Serial.print(delayMinutes);
  Serial.print(" delayByLights = ");
  Serial.print(delayByLightsMinutes);
  Serial.println();
}

void toggleWent() {
    digitalWrite(TRIGER, HIGH);
    delay(250);
    digitalWrite(TRIGER, LOW);
    server.send(200, "plain/text", "Ok");
    Serial.println("changeState");
}

void toggleMirrorHeater() {
    mirrorHeaterState = !mirrorHeaterState; // Toggle the state
    updateMirrorHeater();
}

void updateMirrorHeater() {
    mirrorRequest(mirrorHeaterState ? "turnOnMirrorHeater" : "turnOffMirrorHeater");
    Serial.print("Mirror Heater is now: ");
    Serial.println(mirrorHeaterState ? "ON" : "OFF");
}

// Function to toggle the mirror light
void toggleMirrorLight() {
    mirrorLampState = !mirrorLampState; // Toggle the state
    updateMirrorLight();
}

// Function to toggle the mirror light
void updateMirrorLight() {
    mirrorRequest(mirrorLampState ? "turnOnMirrorLight" : "turnOffMirrorLight");
    
    Serial.print("Mirror Lamp is now: ");
    Serial.println(mirrorLampState ? "ON" : "OFF");
}

void saveToEEPROM() {
  EEPROM.begin(epromLength);  // Initialize the EEPROM
  EEPROM.put(eepromAddrDelay, delayMinutes);
  EEPROM.put(eepromAddrDelayByLights, delayByLightsMinutes);
  EEPROM.commit();  // Commit the changes to EEPROM
}

void readFromEEPROM() {
  EEPROM.begin(epromLength);  // Initialize the EEPROM
  EEPROM.get(eepromAddrDelay, delayMinutes);
  EEPROM.get(eepromAddrDelayByLights, delayByLightsMinutes);
  EEPROM.end();  // Release the EEPROM
}
