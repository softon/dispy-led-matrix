#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include <time.h>
#include <ArduinoJson.h>
#include "config.h"

//OTA
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char *ssid     = WIFI_SSID;
const char *password = WIFI_PASSWORD;

int pinCS = D4; // Attach CS to this pin, DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI )
int numberOfHorizontalDisplays = DISPLAY_LENGTH;
int numberOfVerticalDisplays   = DISPLAY_HEIGHT;
char time_value[20];
String message, webpage;

//################# DISPLAY CONNECTIONS ################
// LED Matrix Pin -> ESP8266 Pin
// Vcc            -> 3v  (3V on NodeMCU 3V3 on WEMOS)
// Gnd            -> Gnd (G on NodeMCU)
// DIN            -> D7  (Same Pin for WEMOS)
// CS             -> D4  (Same Pin for WEMOS)
// CLK            -> D5  (Same Pin for WEMOS)


Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
int wait   = LED_SCROLL_DELAY; // In milliseconds between scroll movements
int spacer = LED_SPACER;
int width  = 5 + spacer; // The font width is 5 pixels
String SITE_WIDTH =  "1000";

int device_id = DEVICE_ID;

int wd_count = 0;
int sync_count = 0;

void setup() {
  Serial.begin(115200); // initialize serial communications
  
  WiFi.begin(ssid,password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  configTime(NTP_ADJUSTMENT_FACTOR, 0, NTP_SERVER, "time.nist.gov");
  //setenv("TZ", "GMT+05:30",1);

  
  matrix.setIntensity(LED_INTENSITY);    // Use a value between 0 and 15 for brightness
  matrix.setRotation(0, 1);  // The first display is position upside down
  matrix.setRotation(1, 1);  // The first display is position upside down
  matrix.setRotation(2, 1);  // The first display is position upside down
  matrix.setRotation(3, 1);  // The first display is position upside down

  matrix.fillScreen(LOW);
  matrix.write();

  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(DEVICE_HOSTNAME);

  // No authentication by default
  ArduinoOTA.setPassword(DEVICE_OTA_PASSWORD);

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  
  //ArduinoOTA.setPasswordHash("65975a74ecc2cb99263ae4a447726392");

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
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  display_message(DEVICE_HOSTNAME);
  
  delay(500);
}

void loop() {
  ArduinoOTA.handle();
  time_t now = time(nullptr);
  String time = String(ctime(&now));
  time.trim();
  //Serial.println(time);
  time.substring(11,19).toCharArray(time_value, 10); 
  matrix.drawChar(2,0, time_value[0], HIGH,LOW,1); // H
  matrix.drawChar(8,0, time_value[1], HIGH,LOW,1); // HH  
  matrix.drawChar(14,0,time_value[2], HIGH,LOW,1); // HH:
  matrix.drawChar(20,0,time_value[3], HIGH,LOW,1); // HH:M
  matrix.drawChar(26,0,time_value[4], HIGH,LOW,1); // HH:MM
  matrix.write(); // Send bitmap to display

  delay(2000);
  if(wd_count>1000){
    ESP.restart();
  }
  wd_count++;
  sync_count++;
  if(sync_count>4){
    sync_count =0;
    get_message(); // Display the message
  }
  
}

void get_message(){
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
 
      HTTPClient http;  //Declare an object of class HTTPClient
       
      http.begin("http://dispy.in/api/notices/"+String(device_id));  //Specify request destination
      int httpCode = http.GET();                                                                  //Send the request
       
      if (httpCode > 0) { //Check the returning code
       
        String payload = http.getString();   //Get the request response payload
        //Serial.println(payload);                     //Print the response payload
        const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 600;
        DynamicJsonDocument doc(capacity);

        // Parse JSON object
        //JsonObject& root = jsonBuffer.parseObject(payload);
        DeserializationError err = deserializeJson(doc, payload);

        if (err) {
          Serial.println(F("Parsing failed!"));
          return;
        }
        int node_length = doc.size();
        for(int i=0; i<node_length;i++){
          display_message(doc[i].as<char*>());
        }
     
      } 
     
      http.end();   
   
  }
}

void display_message(String message) {
  for ( int i = 0 ; i < width * message.length() + matrix.width() - spacer; i++ ) {
    matrix.fillScreen(LOW);
    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2; // center the text vertically
    while ( x + width - spacer >= 0 && letter >= 0 ) {
      if ( letter < message.length() ) {
        matrix.drawChar(x, y, message[letter], HIGH, LOW, 1); // HIGH LOW means foreground ON, background OFF, reverse these to invert the display!
      }
      letter--;
      x -= width;
    }
    matrix.write(); // Send bitmap to display
    delay(wait / 2);
  }
}
