#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>  // Include the EEPROM library

#define TRIGER 5
#define MIRROR_HEATER 14
#define MIRROR_LAMP 16
#define SENSOR_WENT 2
#define SENSOR_LIGHT 4

const char* ssid = "Patsuk";  // Set your WiFi network name (SSID)
const char* password = "@@@@@@@@";  // Set your WiFi password

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
  
  pinMode(MIRROR_HEATER, OUTPUT);
  digitalWrite(MIRROR_HEATER, LOW);
  
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
    String html = "<html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";    
    html += "<style>";
    html += "    body {";
    html += "        font-family: Arial, sans-serif;";
    html += "        display: flex;";
    html += "        justify-content: center;";
    html += "        align-items: center;";
    html += "        height: 100vh;";
    html += "        margin: 0;";
    html += "        background-color: #f4f4f4;";
    html += "        overflow-x: hidden;";
    html += "    }";
    html += "    .container {";
    html += "        text-align: center;";
    html += "        background: white;";
    html += "        padding: 20px;";
    html += "        border-radius: 8px;";
    html += "        box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);";
    html += "        width: 100%;"; 
    html += "        max-width: 500px;"; 
    html += "        box-sizing: border-box;";
    html += "    }";
    html += "    .row {";
    html += "        display: flex;";
    html += "        justify-content: center;";
    html += "        align-items: center;";
    html += "        margin-bottom: 10px;";
    html += "    }";
    html += "    .card {";
    html += "        background-color: #f9f9f9;";
    html += "        padding: 20px;";
    html += "        border-radius: 8px;";
    html += "        box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);";
    html += "        margin-bottom: 20px;";
    html += "        width: 100%;";  
    html += "        box-sizing: border-box;";
    html += "    }";
    html += "    button {";
    html += "        margin: 10px;";
    html += "        padding: 15px 20px;"; 
    html += "        font-size: 1rem;";
    html += "        border: none;";
    html += "        border-radius: 5px;";
    html += "        cursor: pointer;";
    html += "        transition: background-color 0.3s;";
    html += "    }";
    html += "    .on {";
    html += "        background-color: #28a745;";
    html += "        color: white;";
    html += "    }";
    html += "    .off {";
    html += "        background-color: blue;";
    html += "        color: white;";
    html += "    }";
    html += "    input {";
    html += "        padding: 15px;"; 
    html += "        margin: 10px;";
    html += "        border: 1px solid #ccc;";
    html += "        border-radius: 5px;";
    html += "        font-size: 1rem;"; 
    html += "        width: calc(100% - 40px);"; 
    html += "    }";

    html += "    /* Media query for smaller devices */";
    html += "    @media screen and (max-width: 600px) {";
    html += "        body {";
    html += "            height: auto;";
    html += "            justify-content: flex-start;";
    html += "            padding: 10px;"; 
    html += "        }";
    html += "        .container {";
    html += "            width: 100vw;";  
    html += "            height: auto;";
    html += "            max-width: none;"; 
    html += "            border-radius: 0;";
    html += "            padding: 10px;";
    html += "        }";
    html += "        button {";
    html += "            width: 100%;";
    html += "            font-size: 1.2rem;";
    html += "        }";
    html += "        input {";
    html += "            font-size: 1.2rem;";
    html += "            width: calc(100% - 20px);";
    html += "        }";
    html += "    }";
    html += "</style>";
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<p>Went sensor reading: <span id='wentSensorReading'>null</span></p>";
    html += "<p>Light sensor reading: <span id='lightSensorReading'>null</span></p>";
    html += "<p>Toggle went in: <span id='delayToToggle'>N/A</span></p>";

    html += "<div class='row'>";
    html += "<button onclick='changeState()'>Toggle went</button>";
    html += "</div>";

    html += "<div class='row'>";
    html += "<button id='lampButton' class='off' onclick='toggleMirrorLight()'>Mirror Light: Off</button>";
    html += "<button id='heaterButton' class='off' onclick='toggleMirrorHeater()'>Mirror Heater: Off</button>";
    html += "</div>";

    html += "<div class='card'>";
    html += "<div class='row'>";
    html += "<label for='delayByLightsInput'>Enter delay after turning lights off (1-60 minutes):</label>";
    html += "</div>";
    html += "<div class='row'>";
    html += "<input type='text' id='delayByLightsInput' pattern='[0-9]*' placeholder='1-60' value='null'>";
    html += "<button onclick='sendDelayByLights()'>Accept</button>";
    html += "</div>";
    html += "</div>";

    html += "<div class='card'>";
    html += "<div class='row'>";
    html += "<label for='delayInput'>Enter delay (1-60 minutes):</label>";
    html += "</div>";
    html += "<div class='row'>";
    html += "<input type='text' id='delayInput' pattern='[0-9]*' placeholder='1-60' value='null'>";
    html += "<button onclick='sendDelay()'>Accept</button>";
    html += "</div>";
    html += "</div>";

    html += "</div>";
    html += "<script>";
    html += "    function getMemoryData(callback) {";
    html += "        var xhr = new XMLHttpRequest();";
    html += "        xhr.open('GET', '/getMemoryData', true);";
    html += "        xhr.onreadystatechange = function() {";
    html += "            if (xhr.readyState === 4 && xhr.status === 200) {";
    html += "                var data = JSON.parse(xhr.responseText);";
    html += "                callback(data);";
    html += "            }";
    html += "        };";
    html += "        xhr.send();";
    html += "    }";

    html += "    function updatePageWithData(data) {";
    html += "        document.getElementById('wentSensorReading').textContent = data.wentSensorReading;";
    html += "        document.getElementById('lightSensorReading').textContent = data.lightSensorReading;";
    html += "        document.getElementById('delayToToggle').textContent = data.delayToToggle !== null ? parseInt(data.delayToToggle / 1000) : 'N/A';";

    html += "        document.getElementById('heaterButton').textContent = 'Mirror Heater: ' + (data.mirrorHeaterState ? 'On' : 'Off');";
    html += "        document.getElementById('lampButton').textContent = 'Mirror Light: ' + (data.mirrorLampState ? 'On' : 'Off');";

    html += "        var heaterButton = document.getElementById('heaterButton');";
    html += "        if (data.mirrorHeaterState) {";
    html += "            heaterButton.classList.remove('off');";
    html += "            heaterButton.classList.add('on');";
    html += "        } else {";
    html += "            heaterButton.classList.remove('on');";
    html += "            heaterButton.classList.add('off');";
    html += "        }";

    html += "        var lampButton = document.getElementById('lampButton');";
    html += "        if (data.mirrorLampState) {";
    html += "            lampButton.classList.remove('off');";
    html += "            lampButton.classList.add('on');";
    html += "        } else {";
    html += "            lampButton.classList.remove('on');";
    html += "            lampButton.classList.add('off');";
    html += "        }";
    html += "    }";

    html += "    function update() {";
    html += "        getMemoryData(updatePageWithData);";
    html += "    }";
    
    html += "    function initPage() {";
    html += "        getMemoryData(function(data) {";
    html += "          document.getElementById('delayByLightsInput').value = data.delayByLightsMinutes;";
    html += "          document.getElementById('delayInput').value = data.delayMinutes;";
    html += "          updatePageWithData(data);";
    html += "        });";
    html += "    }";

    html += "    function changeState() {";
    html += "        var xhr = new XMLHttpRequest();";
    html += "        xhr.open('GET', '/changeState', true);";
    html += "        xhr.send();";
    html += "        update();";
    html += "    }";

    html += "    function toggleMirrorLight() {";
    html += "        var xhr = new XMLHttpRequest();";
    html += "        xhr.open('GET', '/toggleMirrorLight', true);";
    html += "        xhr.send();";
    html += "        update();";
    html += "    }";
    html += "    function toggleMirrorHeater() {";
    html += "        var xhr = new XMLHttpRequest();";
    html += "        xhr.open('GET', '/toggleMirrorHeater', true);";
    html += "        xhr.send();";
    html += "        update();";
    html += "    }";
    html += "    function sendDelay() {";
    html += "        var delayInput = document.getElementById('delayInput');";
    html += "        var delayValue = delayInput.value;";
    html += "        if (/^\\d+$/.test(delayValue) && delayValue >= 1 && delayValue <= 60) {";
    html += "            var xhr = new XMLHttpRequest();";
    html += "            xhr.open('GET', '/setDelay?value=' + delayValue, true);";
    html += "            xhr.send();";
    html += "        } else {";
    html += "            alert('Please enter a valid number between 1 and 60.');";
    html += "        }";
    html += "    }";
    html += "    function sendDelayByLights() {";
    html += "        var delayInput = document.getElementById('delayByLightsInput');";
    html += "        var delayValue = delayInput.value;";
    html += "        if (/^\\d+$/.test(delayValue) && delayValue >= 1 && delayValue <= 60) {";
    html += "            var xhr = new XMLHttpRequest();";
    html += "            xhr.open('GET', '/setDelayByLights?value=' + delayValue, true);";
    html += "            xhr.send();";
    html += "        } else {";
    html += "            alert('Please enter a valid number between 1 and 60.');";
    html += "        }";
    html += "    }";
    html += "    setInterval(update, 1000);";
    html += "    initPage();";
    html += "</script>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

server.on("/getMemoryData", HTTP_GET, []() {
    String json = "{";
    json += "\"wentSensorReading\": " + String(wentSensorReading);
    json += ", \"lightSensorReading\": " + String(lightSensorReading);
    if (wentSensorReading && delayedToggleMillis > 0) {
        json += ", \"delayToToggle\": " + String(delayedToggleMillis - millis());
    }
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
    digitalWrite(MIRROR_HEATER, mirrorHeaterState ? HIGH : LOW); // Set pin HIGH or LOW
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
    digitalWrite(MIRROR_LAMP, mirrorLampState ? HIGH : LOW); // Set pin HIGH or LOW
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
