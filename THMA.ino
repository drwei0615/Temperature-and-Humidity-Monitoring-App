#include "arduino_secrets.h"
#include "arduino_secrets.h"
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include "thingProperties.h"

#define DHTPIN 4         // DHT11 connected to D2
#define DHTTYPE DHT11
#define GREEN_LED 14     // D5
#define RED_LED 12       // D6

DHT dht(DHTPIN, DHTTYPE);

// Web server for MIT App (runs in parallel with Arduino Cloud)
WiFiServer appServer(80);

unsigned long lastPrint = 0; // for IP logging

void setup() {
  Serial.begin(9600);
  delay(1500); 

  dht.begin(); // Initialize DHT11

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  // Start Arduino Cloud
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  delay(2000);
 
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());

  appServer.begin(); // Start HTTP server for MIT App
}

void loop() {
  ArduinoCloud.update();  // Sync cloud values

  // Read temperature and humidity
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // Validate readings
  if (!isnan(temp) && !isnan(hum)) {
    temperature = temp;
    humidity = hum;

    Serial.print("Temp: ");
    Serial.print(temperature);
    Serial.print("°C | Humidity: ");
    Serial.println(humidity);
  }

  // Auto LED control
  if (temperature > 30) {
    redLED = true;
    greenLED = false;
  } else {
    redLED = false;
    greenLED = true;
  }

  digitalWrite(GREEN_LED, greenLED);
  digitalWrite(RED_LED, redLED);

  // MIT App Inventor Server Logic
  handleMITAppRequest();

  // 🔁 Print IP every 5 seconds
  if (millis() - lastPrint > 5000) {
    Serial.print("Current IP: ");
    Serial.println(WiFi.localIP());
    lastPrint = millis();
  }

  delay(1000); // Wait 1 sec
}

void onGreenLEDChange() {
  digitalWrite(GREEN_LED, greenLED);
}

void onRedLEDChange() {
  digitalWrite(RED_LED, redLED);
}

// ========== Function: Handle MIT App Web Requests ==========
void handleMITAppRequest() {
  WiFiClient client = appServer.available();
  if (!client) return;

  String req = client.readStringUntil('\r');
  client.flush();

  req.trim();
  Serial.print("MIT App Request: "); Serial.println(req);

  // Handle LED toggle commands
  if (req.indexOf("/green/on") != -1) greenLED = true;
  if (req.indexOf("/green/off") != -1) greenLED = false;
  if (req.indexOf("/red/on") != -1) redLED = true;
  if (req.indexOf("/red/off") != -1 && temperature <= 30) redLED = false;

  digitalWrite(GREEN_LED, greenLED);
  digitalWrite(RED_LED, redLED);

  delay(500);

  // Respond with current sensor + LED state as JSON
  String response = "{";
  response += "\"temperature\":" + String(temperature) + ",";
  response += "\"humidity\":" + String(humidity) + ",";
  response += "\"greenLED\":" + String(greenLED ? 1 : 0) + ",";
  response += "\"redLED\":" + String(redLED ? 1 : 0);
  response += "}";

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println();
  client.println(response);
} 