#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>
#include "Adafruit_CCS811.h"
#include <Adafruit_NeoPixel.h>


//flag for saving data
bool shouldSaveConfig = false;

//define your default values here, if there are different values in config.json, they are overwritten.
//char mqtt_server[40];
char mqtt_server[40] = "192.168.1.249";
char mqtt_port[6] = "1883";
char mqtt_user[20] = "user";
char mqtt_pass[20] = "password";
char mqtt_topic[50] = "stat/air-quality-monitor";

#define ADDR      0x5A
//#define ADDR      0x5B

#define LED_PIN       13 // D7 where WS2812b LEDS are connected
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      4

// D5 Reset

const int sclPin = 5; // D1
const int sdaPin = 4; //  D0

unsigned long  lastMeasurement;
const int delayval = 25; // delay for neopixel

long eCO2 = 0;
long TVOC = 0;
float temp;
int avgeCO2, avgTVOC, i, j = 0, k;
int sumeCO2 = 0;
int sumTVOC = 0;
enum { N = 6 }; // Number of readings to make smooting(average).
//             Default reading every 10 seconds and sends every minute

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_CCS811 ccs;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void setup() {

  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hold your hats");

  pixels.begin(); // This initializes the NeoPixel library.
  pixels.setBrightness(255);
  pixels.show();

  //clean FS for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_pass, json["mqtt_pass"]);
          strcpy(mqtt_topic, json["mqtt_topic"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read



  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 20);
  WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", mqtt_pass, 20);
  WiFiManagerParameter custom_mqtt_topic("topic", "mqtt topic", mqtt_topic, 50);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

// Reset Wifi settings for testing
//  wifiManager.resetSettings();

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //  wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_mqtt_topic);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimum quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("Air-quality-monitor", "config123")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_pass"] = mqtt_pass;
    json["mqtt_topic"] = mqtt_topic;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("Local IP");
  Serial.println(WiFi.localIP());
  const uint16_t mqtt_port_x = atol( mqtt_port );
  client.setServer(mqtt_server, mqtt_port_x);

  if(!ccs.begin()){
    Serial.println("Failed to start sensor! Please check your wiring.");
  } else {
  }



  //calibrate temperature sensor
  while(!ccs.available()) {
    float temp = ccs.calculateTemperature();
    ccs.setTempOffset(temp - 25.0);
  }
  Wire.setClockStretchLimit( 460L );
  ccs.setDriveMode(CCS811_DRIVE_MODE_10SEC);
} // End Setup

// NEOPIXEL
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<pixels.numPixels(); i++) {
      pixels.setPixelColor(i, c);
      pixels.show();
      delay(wait);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect("AirQualitySensor", mqtt_user, mqtt_pass)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      for (int b=1; b<=5; b++) {     // Start our for loop
        colorWipe(pixels.Color(0, 0, 255), 10);
        delay(500);
        colorWipe(pixels.Color(0, 0, 0), 10);
        delay(500);
      }

    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();  // check for MQTT messages

  long now = millis();
///////////////////////////////////////////////////////////////////////////////////////////////////////
  if( now - lastMeasurement > 10000) {
    lastMeasurement = now;
    eCO2 = 0;
    TVOC = 0;
    if (j < N) {
      temp = ccs.calculateTemperature();
      if(!ccs.readData() ){
        eCO2 = ccs.geteCO2(); //returns eCO2 reading
        TVOC = ccs.getTVOC(); //return TVOC reading
        if ( eCO2 > 65000 ) {
          Serial.println("ERROR reading from sensor!");
          delay(3000);
          eCO2 = 0;
        }
        Serial.print("CO2: ");
        Serial.print(eCO2);
        Serial.print("ppm, TVOC: ");
        Serial.print(TVOC);
        Serial.print("ppb   Temp:");
        Serial.println(temp);
        }
      else{
        Serial.println("ERROR reading from sensor!");
        delay(3000);
      }


      // Add numbers to average
       sumeCO2 += eCO2;
       sumTVOC += TVOC;

      j++;
    } else { // We have N averaged measurements, print it and reset counters
      avgeCO2 = sumeCO2 / N;
      avgTVOC = sumTVOC / N;
      Serial.print("Average eCO2: ");
      Serial.println(avgeCO2);
      Serial.print("Average TVOC: ");
      Serial.println(avgTVOC);

      if( avgeCO2 > 0 ) {
        // Prepare a JSON payload string
        String payload = "{";
        payload += "\"eCO2\":"; payload += avgeCO2; payload += ",";
        payload += "\"TVOC\":"; payload += avgTVOC; payload += ",";
        payload += "\"temperature\":"; payload += temp;
        payload += "}";

        // Convert payload
        char attributes[100];
        payload.toCharArray( attributes, 100 );
        // Send MQTT message
        Serial.print("Sending MQTT message: ");
        Serial.println(payload);
        client.publish(mqtt_topic, attributes , false);

      }

      // Set color of neopixel
      if (avgeCO2 >= 1500) {
        colorWipe(pixels.Color(255, 0, 0), 50);
      } else if (avgeCO2 < 1500 && avgeCO2 >= 1000 ) {
        colorWipe(pixels.Color(255, 119, 0), 50);
      } else if (avgeCO2 < 1000  && avgeCO2 > 0 ) {
        colorWipe(pixels.Color(0, 255, 0), 50);
      } else {
        colorWipe(pixels.Color(0, 0, 0), 50);
      }

      j=0;
      sumeCO2 = 0;
      sumTVOC = 0;
    }
  } // End if(now - lastMeasurement > 1000)
} // End LOOP
