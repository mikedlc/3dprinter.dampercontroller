/*
 Publishing in the callback 
 
  - connects to an MQTT server
  - subscribes to the topic "inTopic"
  - when a message is received, republishes it to "outTopic"
  
  This example shows how to publish messages within the
  callback function. The callback function header needs to
  be declared before the PubSubClient constructor and the 
  actual callback defined afterwards.
  This ensures the client reference in the callback function
  is valid.
  
*/

extern "C" {
#include "user_interface.h"
}
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//#include <Stepper.h>
#include <ArduinoOTA.h>
#include "config.h"



const char *ssid =	"WiFiFoFum";		// cannot be longer than 32 characters!
const char *pass =	"6316EarlyGlow";		//

IPAddress server(192, 168, 1, 213);
WiFiClient wclient;

PubSubClient client(wclient, server);
#define BUFFER_SIZE 100

// change this to fit the number of steps per revolution
const int stepsPerRevolution = 200;

// initialize the stepper library on connected pins:
Stepper myStepper(stepsPerRevolution, 15, 12, 13, 14);


long lastMsg = 0;
long lastLedCheck = 0;
char msg[50];
int value = 0;
int led_red_pin = 16;
int led_green_pin = 5;
int led_blue_pin = 2;
int toggle_btn_pin = 0;
int toggle_btn_state = 0;


#define IN1  15
#define IN2  13
#define IN3  12
#define IN4  14
#define LIMITPIN 4
int Steps = 0;
boolean Direction = true;// Clockwise=True
unsigned long last_marker;
unsigned long currentMillis;
int steps_left=4096;
long int marker;

const int ledPin = 13;
const int openDamperSteps = 1100; 

int damperState = 0; //0-stop, 1-closed, 2-open, 3-closing, 4-opening

// Callback function
void callback(const MQTT::Publish& pub) {
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.

  // Copy the payload to a new message
  MQTT::Publish newpub("outTopic", pub.payload(), pub.payload_len());
  client.publish(newpub);

    if (pub.payload_string() == "OPEN_DAMPER"){
      Serial.println("OPEN_DAMPER Requested");
     if (damperState<2) //not in motion  or already open
      {
        damperState = 4;
      }
      else
      {
        client.publish("/damper/status", "error: damper status=" + damperState);
      }
    }
    else if (pub.payload_string() == "CLOSE_DAMPER"){
      Serial.println("CLOSE_DAMPER Requested");
      //closeDamper
      if (damperState<3) //not in motion
      {
        damperState = 3;
      }
      else
      {
        client.publish("/damper/status", "error: damper status=" + damperState);
      }
    } else {
      Serial.println(pub.payload_string());
      client.publish("/damper/status", "error: unrecognized action");
    }

  Serial.print(pub.topic());
  Serial.print(" => ");
  if (pub.has_stream()) {
    uint8_t buf[BUFFER_SIZE];
    int read;
    while (read = pub.payload_stream()->read(buf, BUFFER_SIZE)) {
      Serial.write(buf, read);
    }
    pub.payload_stream()->stop();
    Serial.println("");
  } else{
    Serial.println(pub.payload_string());
  }
}

void releaseEngine() {
  digitalWrite(15, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  digitalWrite(14, LOW);
}


void setup() {
  // Setup console
  Serial.begin(115200);
  delay(10);

  pinMode(IN1, OUTPUT); 
  pinMode(IN2, OUTPUT); 
  pinMode(IN3, OUTPUT); 
  pinMode(IN4, OUTPUT);
  pinMode(LIMITPIN, INPUT_PULLUP);

  // set the speed at 60 rpm:
  myStepper.setSpeed(60);

  pinMode(LED_BUILTIN, OUTPUT); // Initialize the BUILTIN_LED pin as an output
  pinMode(led_red_pin, OUTPUT); 
  pinMode(led_green_pin, OUTPUT);
  pinMode(led_blue_pin, OUTPUT);
  pinMode(toggle_btn_pin,INPUT_PULLUP);

    // OTA begin
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
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

}

void loop() {

  ArduinoOTA.handle();

  //reconnect
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");

    WiFi.setHostname("ArduinoMini001");
    WiFi.begin(ssid, pass);

    if (WiFi.waitForConnectResult() != WL_CONNECTED)
      return;
    Serial.println("WiFi connected: ");
    Serial.println(WiFi.localIP());
      Serial.println(WiFi.getHostname());
    Serial.println("\n");
  }
  //Wifi Connected, so proceed.
  if (WiFi.status() == WL_CONNECTED) {

    //MQTT Connect
    if (!client.connected()) {
      Serial.println("Connecting to MQTT server");
      if (client.connect(MQTT::Connect("DamperDevBoard")
			 .set_auth("mike", "Quik5ilver7"))) {
        Serial.println("Connected to MQTT server");
	      client.set_callback(callback);
	      client.publish("/print/status","Arduino Here...");
        Serial.println("Checking in with: Arduino Here...");
        delay(5000);   
	      client.subscribe("/print/status");
      } else {
        Serial.println("Could not connect to MQTT server, waiting 5 sec...");
          delay(5000);   
      }
    }
//    Serial.println("Debug1");
    client.loop();
//    Serial.println("Debug2");
    //MQTT Connected, proceed.    
    if (client.connected()){
//      Serial.print("Client Connected... Damper State: ");
//      Serial.println(damperState);
      //Main Logic
      //0-stop, 1-closed, 2-open, 3-closing, 4-opening
      if (damperState == 0) //close Damper on init
      {
        Serial.println("Closing Damper on Init...");
        damperState = 3;
        Serial.print("Damper State now: ");
        Serial.println(damperState);
      }

      if (damperState == 3 ) //close Damper
      {
        Serial.println("Damper State is CLOSING...");
        int status;
        status = digitalRead(LIMITPIN);
        Serial.print("LIMITPIN Status: ");
        Serial.println(status);
        if (status == HIGH)
        {
          digitalWrite(led_blue_pin, LOW);
          digitalWrite(led_green_pin, HIGH);
          Direction = true;
          move_stepper(5);
        }
        else
        {
            damperState = 1; //closed
            releaseEngine();
            digitalWrite(led_green_pin, LOW);
            digitalWrite(led_red_pin, HIGH);
            Serial.println("Damper Closed!");
        }
      }
  
      if (damperState == 4) //opening Damper
      {
        digitalWrite(led_red_pin, LOW);
        digitalWrite(led_green_pin, HIGH);
        Serial.println("Opening Damper!");
        snprintf (msg, 50, "open_damper", value);
        Serial.print("Publish message: ");
        Serial.println(msg);
        
        client.publish("/print/status", msg);
        Direction = false;
        move_stepper(openDamperSteps);
        damperState = 2;
        releaseEngine();
        digitalWrite(led_green_pin, LOW);
        digitalWrite(led_blue_pin, HIGH);
      }

      //Watchdog
      long now = millis();
      if (now - lastMsg > 10000) {
        lastMsg = now;
        ++value;
        int status;
        status = digitalRead(LIMITPIN);
              //0-stop, 1-closed, 2-open, 3-closing, 4-opening
        Serial.print("LIMITPIN Status: ");
        Serial.println(status);
        Serial.print("Damper State: ");
        Serial.println(damperState);

        snprintf (msg, 50, "whatchdog is alive #%ld", value);
        Serial.print("Publish message: ");
        Serial.println(msg);
        client.publish("/print/status:", msg);
      }

      if (now - lastLedCheck > 2000) {
        lastLedCheck = now;
      
        int status;
        status = digitalRead(LIMITPIN);

        if (status == 0)
        {
          digitalWrite(led_blue_pin, LOW);
          digitalWrite(led_red_pin, HIGH);
        }
        else if (status == 1)
        {
          digitalWrite(led_red_pin, LOW);
          digitalWrite(led_blue_pin, HIGH);
        }
      }

      // check toggle_btn
      int status;
      status = digitalRead(toggle_btn_pin);
      if (status == LOW)
      {
        if (toggle_btn_state == 0) // change
        {
          Serial.println("Button pressed!");
          toggle_btn_state = 1;
          //0-stop, 1-closed, 2-open, 3-closing, 4-opening
            if (damperState == 0 || damperState == 2) 
            {
              damperState = 3;
            }
            else if (damperState == 1) 
            {
              damperState = 4;
            }
        }  
      }
      else
      {
        if (toggle_btn_state == 1) // change
        {
          Serial.println("Button depressed!");
          toggle_btn_state = 0;

        }
      }
    }


  }
}//end loop

void move_stepper(int steps_left)
{
        while(steps_left>0){
        currentMillis = micros();
        if(currentMillis-last_marker>=1000){
          stepper(1); 
          marker=marker+micros()-last_marker;
          last_marker=micros();
          steps_left--;
          yield();
        }
      }
}

void stepper(int xw){
  for (int x=0;x<xw;x++){
    switch(Steps){
      case 0:
        digitalWrite(IN1, LOW); 
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
      break; 
      case 1:
        digitalWrite(IN1, LOW); 
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, HIGH);
      break; 
      case 2:
        digitalWrite(IN1, LOW); 
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
      break; 
      case 3:
        digitalWrite(IN1, LOW); 
        digitalWrite(IN2, HIGH);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
      break; 
      case 4:
        digitalWrite(IN1, LOW); 
        digitalWrite(IN2, HIGH);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
      break; 
      case 5:
        digitalWrite(IN1, HIGH); 
        digitalWrite(IN2, HIGH);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
      break; 
        case 6:
        digitalWrite(IN1, HIGH); 
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
      break; 
      case 7:
        digitalWrite(IN1, HIGH); 
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
      break; 
      default:
        digitalWrite(IN1, LOW); 
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
      break; 
    }
    SetDirection();
  }
} 

void SetDirection(){
  if(Direction==1){ Steps++;}
  if(Direction==0){ Steps--; }
  if(Steps>7){Steps=0;}
  if(Steps<0){Steps=7; }
}

