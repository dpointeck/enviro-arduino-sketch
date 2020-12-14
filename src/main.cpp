#include <stdio.h>
#include <Arduino.h>
#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#include "arduino_secrets.h"
#include "arduino_config.h"

char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password (use for WPA, or use as key for WEP)
char serverAddress[] = "10.0.0.50"; // server address
int port = 3000;

int status = WL_IDLE_STATUS; // the Wifi radio's status
DynamicJsonDocument doc(2048);
void printData();
float temperature;
float humidity;
float pressure;
float altitude;
float gas;

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme; // I2C
WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);

void setup() {
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED)
  {
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
    // wait 5 seconds for connection:
    delay(5000);
  }

  // Setup BME680 Sensor
  if (!bme.begin(0x76, true))
  {
    while (1);
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
}

void loop() {
  // check the network connection once every 60 seconds:
  delay(SEND_DELAY);
  if (status != WL_CONNECTED)
  {
    status = WL_IDLE_STATUS;
    setup();
  }
  else 
  {
    if(DEBUG){
      printData();
    }

    if (!bme.performReading())
    {
      return;
    }
    temperature = bme.temperature;
    doc["temperature"] = (int)(temperature * 100 + 0.5) / 100.0;
    
    pressure = bme.pressure / 100.0;
    doc["pressure"] = pressure;

    humidity = bme.humidity;
    doc["humidity"] = (int)(humidity * 100 + 0.5) / 100.0;

    gas = (int)((bme.gas_resistance / 1000.0) * 100 + 0.5 ) / 100.0;
    doc["gas"] = gas;

    altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
    doc["altitude"] = (int)(altitude * 100 + 0.5) / 100.0;
    doc["locationId"] = LOCATION;

    String json;
    serializeJson(doc, json);
    client.beginRequest();
    client.post("/api/v1/entries", "application/json", json);
    client.endRequest();
    // note: the above line can also be achieved with the simpler line below:
    //client.print(postData);
  }
}

void printData()
{
  Serial.println("Board Information:");
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  
  Serial.println();
  Serial.println("Network Information:");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);
  Serial.print("Connection status: ");
  status = WiFi.status();
  Serial.println(status);
}