#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "Helper.h"
#define RXD2 16
#define TXD2 17

HardwareSerial *gsmSerial;

const char* ssid     = "SSID";
const char* password = "password";
const char* websockets_server = "ws://192.168.1.14:8080";//"192.168.198.215:8080"; //server adress and port
const byte led_gpio = 19;

using namespace websockets;
StaticJsonDocument<200> doc;
WebsocketsClient client;

void onMessageCallback(WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.println(message.data());
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, message.data());

    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    const char* number = doc["number"];
    const char* uuid = doc["uuid"];
    const char* sms_message = doc["message"];
    Serial.println(number);
    Serial.println(uuid);
    Serial.println(sms_message);
    int8_t result = sendSms(number, sms_message);

    doc.remove("number");
    doc.remove("message");
    doc["result"] = result;
    
    char json_string[60];
    serializeJson(doc, json_string);
    client.send(json_string);
}

int8_t sendSms(const char* number, const char* text) {
  delay(1500);
  int8_t atCommandResponse, counter = 0;
  do {
    atCommandResponse = sendSMSATcommands(text, number, "+CMGS:", 2000, gsmSerial);
    delay(500);
    counter++;
  } while (atCommandResponse == 0 && counter < 10);
  if (atCommandResponse) {
    Serial.print("SMS text ");
    Serial.print(text);
    Serial.print(" sent to ");
    Serial.println(number);
  } else {
    Serial.println("SMS error");
  }
  
  return atCommandResponse;
}

void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Connnection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Got a Ping!");
        client.pong();
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
    }
}

bool connectToWifi() {
   // Connect to the WiFi network
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    } 
    // Print the IP address
    Serial.println("");
    Serial.println("Connected to WiFi");
    Serial.println("IP address: " + WiFi.localIP().toString());

    // Setup Callbacks
    client.onMessage(onMessageCallback);
    client.onEvent(onEventsCallback);
    
    // Connect to server
    bool connected = client.connect(websockets_server);
    if(connected) {
        Serial.println("Connected!");
    } else {
        Serial.println("Not Connected!");
    }
    return connected;
}

void setup() {
  pinMode(led_gpio, OUTPUT);
  // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);
  Serial.begin(115200);
  Serial2.begin(115200);
  Serial.println("Serial Txd is on pin: "+String(TX));
  Serial.println("Serial Rxd is on pin: "+String(RX));
  
  gsmSerial = &Serial2;
  
  delay(5000);  
  int8_t registered = registerGSMToNetwork();
  if (registered) {
    delay(1000);  
    int8_t turnedOn = turnOnGSM();
    if (registered && turnedOn) {
      delay(1000);
      Serial.println("GSM READY");
      bool wifiConnected = connectToWifi();
      if (wifiConnected) {
        digitalWrite(led_gpio, HIGH); 
      }
    }  
  }
}

void loop() { 
  if (Serial2.available() > 0)
    Serial.write(Serial2.read());
  if (Serial.available() > 0)
    Serial2.write(Serial.read());

  client.poll();
}

int8_t turnOnGSM() {
  int8_t atCommandResponse, counter = 0;
  do {
    atCommandResponse = sendATcommandUNI("AT+CMGF=1", "OK", NULL, 2000, gsmSerial);
    delay(500);
    counter++;
  } while (atCommandResponse == 0 && counter < 10);
  return atCommandResponse;
}

int8_t registerGSMToNetwork() {
  int8_t atCommandResponse, counter = 0;
  do {
    atCommandResponse = sendATcommandUNI("AT+CREG?", "+CREG: 0,1", "+CREG: 0,5", 2000, gsmSerial);
    delay(2000);
    counter++;
  } while (atCommandResponse == 0 && counter < 10);
  return atCommandResponse;
}

void sendSMS() {
  int8_t atCommandResponse, counter = 0;
  do {
    atCommandResponse = sendSMSATcommands("Test","00385989080355","+CMGS:",2000,gsmSerial);
    delay(500);
    counter++;
  } while (atCommandResponse == 0 && counter < 10);
   
}