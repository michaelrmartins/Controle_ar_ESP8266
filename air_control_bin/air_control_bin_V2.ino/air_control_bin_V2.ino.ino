/*
  ESP8266 AIR CONTROL - DEC/2022 by
  Michael Martins / M1k3o.0
  Github: https://github.com/michaelrmartins/Esp8266_air_control.git

      /*
       ============================================================= 
       =         - This Project is still in development. -         =
       =============================================================
      /*  

/*

-- - Changelog - --
24/11/2022 - Create
29/11/2022 - First Functions created
01/12/2022 - Wifi Connection Success
04/12/2022 - JSON Parse Success
04/12/2022 - Zabbix API Connection Success
19/12/2022 - Create Debug mensages in Serial Monitor
17/01/2023 - Include Temperature Sensor
27/01/2023 - Include Trimpot 
01/02/2023 - Include Serial Debug infos
03/02/2023 - Version 1.0 finished!

---------- Hardware Mapping --------
Trimpot 1 - Analog 0

Relay 1 - Digital 1  //GPIO 5
Relay 2 - Digital 2  //GPIO 4
 
DHT Sensor - Digital 6 //GPIO 12

*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <DHT.h>

// Constants DHT Sensor config 
// ALERT !!!!!  THIS SENSOR HAVE A JOKE!!  !!!! USE ESP8266 GPIO TABLE 
#define DHTPIN 12  // Digital 6
#define DHTTYPE DHT22

// Constants Module configs
#define ESP_ID 01
#define ESP_LOCATION "Datacenter FMC"

// Wifi Settings
#ifndef STASSID
#define STASSID "GTI_2Ghz"
#define STAPSK  "eduardoemonica-legiao"
#endif

// Constants States  // ** Relay module uses inverted logic ** 
#define ON 0
#define OFF 1

const char* ssid = STASSID;
const char* password = STAPSK;

//Variables
int i;
int time_to_inverter = 86400;  // Time in seconds to inverter relays / 86400 = 24 horas
int temperature_limit = 26;
unsigned int contador = 1;
unsigned int invert_relay_counter = 1;
const int analog_pin_1 = A0;

// Relay Pins
const int relay_1_pin = 5; // Relay 1 Digital pin
const int relay_2_pin = 4; // Relay 2 Digital pin

// Sensor
float sensor_1_trim = 0;
float sensor_1_temp = 0;
float sensor_1_humidity = 0;

// Relay Status / 0 = OFF / 1 = ON
int relay_1_status = ON;
int relay_2_status = OFF;
int relay_3_status = OFF;
int relay_4_status = OFF;

// Relay Erros Status / 0 = OK / 1 = Failure | // Only back to 0 after system reset
int relay_1_error = 0;
int relay_2_error = 0;
int relay_3_error = 0;
int relay_4_error = 0;

/////////////////////////// Functions //////////////////////////////

// Air inverter Function
void air_inverter()
{
  // Relay inversion check
  if ( invert_relay_counter >= time_to_inverter && relay_1_error == 0 && relay_2_error == 0){

    digitalWrite(relay_1_pin, !digitalRead(relay_1_pin));
    delay(1300);
    digitalWrite(relay_2_pin, !digitalRead(relay_2_pin));
    invert_relay_counter = 1; // Reset Counter

    // Update Status values
    relay_1_status = digitalRead(relay_1_pin);
    relay_2_status = digitalRead(relay_2_pin);
    }
}

// Temperature check and Status update Function
void temperature_check()
{
  if (sensor_1_temp >= temperature_limit && relay_1_error == 0 && relay_2_error == 0)
    {
      // Verify which relay is activated, and change error status to 1
      if (relay_1_status == ON)
      {
        relay_1_error = 1;
      } else { relay_2_error = 1; }

      digitalWrite(relay_1_pin, ON);
      digitalWrite(relay_2_pin, ON);

      relay_1_status = digitalRead(relay_1_pin);
      relay_2_status = digitalRead(relay_2_pin);
    }
}

// Temperature Trim Read Function / 
float sensor1_TrimValue_Read()
{
  float sensor_1_rawData = analogRead(analog_pin_1);
  float sensor_1_trimValue=map(sensor_1_rawData, 0, 1023, -8, 8);

  return sensor_1_trimValue;
}

// Wifi Led Blink
void wifi_led_blink()
{
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
}

// DHT Sensor Initialize
DHT dhtSensor(DHTPIN, DHTTYPE);

// Create instance to server and specify the port to listen on as an argument
WiFiServer server(80);

// Setup / Configurations
void setup() {
  // Pins mode Initialize
  pinMode(relay_1_pin, OUTPUT);
  pinMode(relay_2_pin, OUTPUT);

  // Pins State Initialize
  digitalWrite(relay_1_pin, ON);
  digitalWrite(relay_2_pin, OFF);

  dhtSensor.begin();
  Serial.begin(115200);
  delay(3000);

  // ----- OTA Configuration ---- //
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("esp-dht01");

  // No authentication by default
  ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  //MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  //ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  
  // Debug Header Infos | First startup, is necessary read sensor temp and humidity before loop
  sensor_1_temp = dhtSensor.readTemperature() + sensor_1_trim;
  sensor_1_humidity = dhtSensor.readHumidity();
  Serial.println("Device Initialize...");
  Serial.print("Device Location: ");
  Serial.println(ESP_LOCATION);
  Serial.print("Device ID: ");
  Serial.println(ESP_ID);
  Serial.print("Wifi SSID: ");
  Serial.println(STASSID);
  Serial.print("Wifi Password: ");
  Serial.println(STAPSK);
  Serial.print("Sensor 1 Temp: ");
  Serial.println(sensor_1_temp);
  Serial.print("Sensor 1 Humidity: ");
  Serial.println(sensor_1_humidity);
  Serial.print("Sensor 1 Trim value: ");
  Serial.println(sensor_1_trim);
  Serial.println("Device Ready!");

  // WIFI Debug infos
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  //  Wifi begin and Connect
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  /*
  while (WiFi.status() != WL_CONNECTED) {
    wifi_led_blink();
    Serial.print(F("."));
  }
  Serial.println();
  Serial.println(F("WiFi connected"));
  digitalWrite(LED_BUILTIN, HIGH);  
   */

  // Start server
  server.begin();
  Serial.println(F("Server started"));

  // Print the IP address
  Serial.println(WiFi.localIP());
}

// LOOP - Handler Functions ---------
void loop() 
{
  // Check if client has connected
  WiFiClient client = server.available();
  if (!client) 
  {
    // Wifi Check
    if (WiFi.status() == WL_CONNECTED)
    {
       digitalWrite(LED_BUILTIN, HIGH);  
    } 
    else
    {
       wifi_led_blink();
    }
    // OTA Handle
    ArduinoOTA.handle();

    // Call functions 
    air_inverter();
    temperature_check();
    
    // sensor2
    sensor_1_trim = sensor1_TrimValue_Read();
    sensor_1_temp = dhtSensor.readTemperature() + sensor_1_trim;
    sensor_1_humidity = dhtSensor.readHumidity();
    invert_relay_counter +=1;
    delay(1000);
    //Serial.println(sensor_1_temp);
    //Serial.println(sensor_1_trim);
    return;
  }

  // New Client Connection handler
  Serial.println(F("new client"));
  client.setTimeout(5000); // default is 1000

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(F("request: "));
  Serial.println(req);
    /*
      // Match the request / Post Commands
      int val;
      if (req.indexOf(F("/gpio/0")) != -1) {
        val = 0;
      } else if (req.indexOf(F("/gpio/1")) != -1) {
        val = 1;
      } else {
        Serial.println(F("invalid request"));
        val = digitalRead(LED_BUILTIN);
      }

      // Set LED according to the request
      digitalWrite(LED_BUILTIN, val);
    */

  // read/ignore the rest of the request
  // do not client.flush(): it is for output only, see below
  while (client.available()) {
    // byte by byte is not very efficient
    client.read();
  }

  // Send the response to the client
  // it is OK for multiple small client.print/write,
  // because nagle algorithm will group them into one single packet
  // client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nGPIO is now ")); // Original line
  client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"));
  
  // Create JSON File
  client.print("{"); // Open Json
  client.println("\"ID\":\"" + String(ESP_ID) + "\",");
  client.println("\"LOCATION\":\"" + String(ESP_LOCATION) + "\",");
  client.println("\"SENSOR_TEMP_1_TRIM_VALUE\":\"" + String(sensor_1_trim) + "\",");
  client.println("\"SENSOR_TEMP_1\":\"" + String(sensor_1_temp) + "\",");
  client.println("\"SENSOR_HUMIDITY_1\":\"" + String(sensor_1_humidity) + "\",");
  client.println("\"RELAY_1_STATUS\":\"" + String(relay_1_status) + "\",");
  client.println("\"RELAY_2_STATUS\":\"" + String(relay_2_status) + "\",");
  client.println("\"RELAY_3_STATUS\":\"" + String(relay_3_status) + "\",");
  client.println("\"RELAY_4_STATUS\":\"" + String(relay_4_status) + "\",");
  client.println("\"RELAY_1_ERROR\":\"" + String(relay_1_error) + "\",");
  client.println("\"RELAY_2_ERROR\":\"" + String(relay_2_error) + "\",");
  client.println("\"RELAY_3_ERROR\":\"" + String(relay_3_error) + "\",");
  client.println("\"RELAY_4_ERROR\":\"" + String(relay_4_error) + "\",");
  client.println("\"SSID_CONNECTED\":\"" + String(ssid) + "\",");
  client.println("\"WIFI_SIGNAL_RSSI\":\"" + String(WiFi.RSSI()) + "\",");
  client.println("\"TIME_COUNTER\":\"" + String(invert_relay_counter) + "\"");
  //client.println("\"IP_ADDRESS\":\"" + String(WiFi.localIP()) + "\"")
  client.print("}"); // End Json
  
  // The client will actually be *flushed* then disconnected
  // when the function returns and 'client' object is destroyed (out-of-scope)
  // flush = ensure written data are received by the other side
  Serial.println(F("Disconnecting from client"));
}