/*
Design Synthesis.net -r.young 5/28/2019
Skylight Sub-Controller [LSCTRL1 / LSCTRL2]
Control two motors with potentiometer limits
UP and DOWN limit switches for damage control

*/
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Automaton.h>

#define motorPin1   2
#define motorPin2   3
#define dirPin      4
//  RED WIRE COMMON HIGH NO
#define upLimitPin1    6 //BLUE
#define downLimitPin1  5 //YELLO
#define upLimitPin2    7 //BLUE
#define downLimitPin2  8 //YELLOW

#define potPin1   A0
#define potPin2   A1
#define ledPin 9

const int UpperLimit = 475;
const int LowerLimit = 33;
enum position {down, up, moving};
// Automaton Objects ----------------------------------------
Atm_button upSwitch1;
Atm_button downSwitch1;
Atm_button upSwitch2;
Atm_button downSwitch2;

// State Machine functions for Potentiometer--
Atm_analog pot1;
Atm_analog pot2;
// Controller machine for monitoring the Potenciometers
Atm_controller MotorController;
Atm_led statusLED;

const char* mqtt_mqttServer = "192.198.10.22";
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 10, 169);
IPAddress myDns(192, 168, 10, 199);
IPAddress gateway(192, 168, 10, 199);
IPAddress subnet(255, 255, 255, 0);
IPAddress mqttServer (192, 168, 10, 22);

EthernetClient ethclient;
PubSubClient client(ethclient);

unsigned long previousMillis;
unsigned long polling_interval = 100;
int position = 0;

Atm_led motor1;
Atm_led motor2;

uint16_t avgPot1Buffer[24];
uint16_t avgPot2Buffer[24];


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  
  Serial.println();

   if ((char)payload[0] == '0') {
   
    Serial.println("MQTT_STOP");
    motor1.trigger(motor1.EVT_OFF);
    motor2.trigger(motor2.EVT_OFF);

    digitalWrite(dirPin,LOW);
      
  } 

  if ((char)payload[0] == '1') {
   
    Serial.println("MQTT_CLOSING");
    client.publish("STATUS", "1-CLOSING");
    motor1.trigger(motor1.EVT_ON);
    motor2.trigger(motor2.EVT_ON);
    digitalWrite(dirPin,LOW);
     
  } 

  if ((char)payload[0] == '2') {

    Serial.println("MQTT_OPEN");
    client.publish("STATUS", "1-OPENING");
    motor1.trigger(motor1.EVT_ON);
    motor2.trigger(motor2.EVT_ON);
    digitalWrite(dirPin,HIGH);
    
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("LF2")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("LF2", "ONLINE");
      

      // ... and resubscribe
      client.subscribe("SIGNAL");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void Stop(int motorNum)
{
    char s[24];  
    sprintf(s,"Stop motor %d",motorNum);
    Serial.println(s); 
    motor1.trigger(motor1.EVT_OFF);
    //digitalWrite(motorNum,LOW);
    digitalWrite(dirPin,LOW);

}

void pot1_callback( int idx, int v, int up ) {
  //Serial.println(v);
  if (v < LowerLimit  || v > UpperLimit)
  { 
   //Stop(motorPin1);
   motor1.trigger(motor1.EVT_OFF);
   motor1.trace( Serial);
  }
}
void pot2_callback( int idx, int v, int up ) {
   //Serial.println(v);
  if (v < LowerLimit  || v > UpperLimit)
  {  
   //Stop(motorPin2);
   motor2.trigger(motor2.EVT_OFF);
   motor2.trace( Serial );
  }
}
// Limits Switches Callback -------------------------
void btn_callback(int idx, int v, int up)
{
  if(idx==1){    
    motor1.trigger(motor1.EVT_OFF);
    Serial.println("MOTOR-1->STOP"); 
  }
  if(idx==2){    
    motor1.trigger(motor2.EVT_OFF);
    Serial.println("MOTOR-2->STOP");
  } 
}

void setup() {
  // Directional PIN ------------------------------------------
  pinMode(dirPin,OUTPUT);
  // Limit Switches Init ----------------------------------------
  downSwitch1.begin(downLimitPin1).onPress(btn_callback,1);
  upSwitch1.begin(upLimitPin1).onPress(btn_callback,1);
  
  upSwitch2.begin(upLimitPin2).onPress(btn_callback,2);
  downSwitch2.begin(downLimitPin2).onPress(btn_callback,2);
  //--------------------------------------------------------------
  pot1.begin(potPin1,100)
      .average(avgPot1Buffer, sizeof(avgPot1Buffer))
        .onChange(pot1_callback);
  pot2.begin(potPin2,100)
      .average(avgPot2Buffer, sizeof(avgPot2Buffer))
          .onChange(pot2_callback);
  // -------------------------------------------------------------
  // Motors Controls
  motor1.begin(motorPin1);
  motor2.begin(motorPin2);

  Serial.begin(9600);
  // print your local IP address: 
  // Allow the hardware to sort itself out
  delay(1500);

  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
  Ethernet.begin(mac, ip);
  Serial.println(Ethernet.localIP());
}

void loop() {
  
  if (!client.connected()) {
    reconnect();
  }
 
  client.loop();
  automaton.run();

  unsigned long currentMillis = millis();
  // Main Utility Task Loop
  if(currentMillis - previousMillis > polling_interval) {  
    previousMillis = currentMillis;  
  }


}


