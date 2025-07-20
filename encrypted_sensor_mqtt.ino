#include <SimpleDHT.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <AESLib.h>  // AES Encryption Library

// WiFi parameters
#define WLAN_SSID       "Wifi"
#define WLAN_PASS       "Password"

// Adafruit IO
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME "YOUR_ADAFRUIT_USERNAME"
#define AIO_KEY "YOUR_ADAFRUIT_IO_KEY"


// Encryption Key (16 bytes for AES-128)
byte aes_key[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };  // Example key
byte aes_iv[] = { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF };  // Example IV

AESLib aesLib;

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish Temperature1 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Temperature1");
Adafruit_MQTT_Publish Humidity1 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Humidity1");

int pinDHT11 = 0;
SimpleDHT11 dht11;

// Function to encrypt data using AES and return as Base64-encoded string
String encrypt(String data) {
  char encrypted_output[512]; // Buffer for encrypted output
  int dataSize = data.length();

  // Encrypt the data (AES-128) and encode it to Base64
  int bits = 128; // Key size in bits
  aesLib.encrypt64((byte*)data.c_str(), dataSize, encrypted_output, aes_key, bits, aes_iv);

  return String(encrypted_output);  // Return encrypted data as a String
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  connect();
}

// Connect to Adafruit IO via MQTT
void connect() {
  while (mqtt.connect() != 0) {
    delay(10000);
  }
  Serial.println("Adafruit IO Connected!");
}

void loop() {
  if (!mqtt.connected()) {
    connect();
  }
  mqtt.ping();

  byte temp = 0, hum = 0;
  int err = dht11.read(pinDHT11, &temp, &hum, NULL);
  if (err != SimpleDHTErrSuccess) {
    Serial.println("Failed to read from DHT11");
    delay(2000);
    return;
  }

  // Print the plain sensor data
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(" C, Humidity: ");
  Serial.print(hum);
  Serial.println(" %");

  // Encrypt the sensor data
  String tempData = String(temp);
  String humData = String(hum);
  String encryptedTemp = encrypt(tempData);
  String encryptedHum = encrypt(humData);

  // Print encrypted data
  Serial.print("Encrypted Temperature: ");
  Serial.println(encryptedTemp);
  Serial.print("Encrypted Humidity: ");
  Serial.println(encryptedHum);

  // Measure time for MQTT publish
  unsigned long startTime = millis();
  if (!Temperature1.publish(encryptedTemp.c_str())) {
    Serial.println("Failed to publish encrypted temperature");
  }
  if (!Humidity1.publish(encryptedHum.c_str())) {
    Serial.println("Failed to publish encrypted humidity");
  }
  unsigned long elapsedTime = millis() - startTime;
  Serial.print("Time taken for MQTT publish: ");
  Serial.println(elapsedTime); // Display time taken

  delay(5000);
}