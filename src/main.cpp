/**************************************************************
*  Design Synthesis.net -r.young 7/10/2019 v1.3
*  Skylight Sub-Controller [LSCTRL1 / LSCTRL2]
*  Control two motors with potentiometer limits
*  UP and DOWN limit switches for failsafe 
*  damage control
*  ------------------------------------------------------------
*  UnitName =       LFCTRL-2
*  Controls Motor   3-4
*  ------------------------------------------------------------
***************************************************************/

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Automaton.h>


#define motorPin1   9
#define motorPin2   3
#define dirPin      4
//  RED WIRE COMMON HIGH NO
#define upLimitPin1    6 //BLUE
#define downLimitPin1  5 //YELLO
#define upLimitPin2    7 //BLUE
#define downLimitPin2  8 //YELLOW

#define potPin1   A0
#define potPin2   A1
#define ledPin    A3 // Status All In One LED
// Debugging Flags
#define debug     0
#define mqtt      1
#define switches  1


// M3 ------------------------------
const int UpperLimit = 405;
const int LowerLimit = 65;
// M4 -----------------------------
const int UpperLimit2 = 405;
const int LowerLimit2 = 65;
//  --------------------------------
int action = 0;
// Automaton Objects ------------------------------------------------------------------------
Atm_button upSwitch1;
Atm_button downSwitch1;
Atm_button upSwitch2;
Atm_button downSwitch2;
// State Flags ------------------------------------------------------------------------------
bool LF3_UP{false};
bool LF3_DOWN{false};
bool LF4_UP{false};
bool LF4_DOWN{false};
// State Machine functions for Potentiometer--
Atm_analog pot1;
Atm_analog pot2;
// Controller machine for monitoring the Potenciometers
Atm_controller MotorController;
Atm_led statusLED;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE0
};
IPAddress ip(192, 168, 10, 162);
IPAddress myDns(192, 168, 10, 199);
IPAddress gateway(192, 168, 10, 199);
IPAddress subnet(255, 255, 255, 0);
IPAddress mqttServer (192, 168, 10, 22);

EthernetClient ethclient;
PubSubClient client(ethclient);

unsigned long previousMillis;
unsigned long polling_interval = 500;
int position = 0;

Atm_led motor3;
Atm_led motor4;

uint16_t avgPot1Buffer[16];
uint16_t avgPot2Buffer[16];

long PUSHCORRECTION = 2.8;
long PULLCORRECTION = 0.14;

void callback(char* topic, byte* payload, unsigned int length) {
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  
  Serial.println();

   if ((char)payload[0] == '0') {
    #if debug == 1
    Serial.println("MQTT_STOP");
    #endif

    motor3.trigger(motor3.EVT_OFF);
    motor4.trigger(motor4.EVT_OFF);
    action=0;
    statusLED.blink(2000,500).trigger(statusLED.EVT_BLINK);
    digitalWrite(dirPin,LOW);
      
  } 

  if ((char)payload[0] == '1') {
   
    #if debug ==1
    Serial.println("MQTT_CLOSING");
    #endif

    action = 1;
    motor3.trigger(motor3.EVT_ON);
    motor4.trigger(motor4.EVT_ON);
    statusLED.blink(1000,1000).trigger(statusLED.EVT_BLINK);
    digitalWrite(dirPin,LOW);
     
  } 

  if ((char)payload[0] == '2') {

    #if debug ==1
    Serial.println("MQTT_OPEN");
    #endif

    action = 2;
    motor3.trigger(motor3.EVT_ON);
    motor4.trigger(motor4.EVT_ON);
    statusLED.blink(5000,500).trigger(statusLED.EVT_BLINK);
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
      client.publish("STATUS", "35");
      client.publish("STATUS", "45");
      statusLED.blink(500,500).trigger(statusLED.EVT_BLINK);
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
 

void pot1_callback( int idx, int v, int up ) {
 
  

  if (v < LowerLimit  && action==1)
  { 
    
   motor3.trigger(motor3.EVT_OFF);
   statusLED.blink(2000,500).trigger(statusLED.EVT_BLINK); 
   if(!LF3_DOWN){
      client.publish("STATUS", "31");
      LF3_DOWN=true;
      LF3_UP=false;
   }
  
   //action=0;

   }else if(v > UpperLimit && action==2){
     motor3.trigger(motor3.EVT_OFF);
     statusLED.blink(2000,500).trigger(statusLED.EVT_BLINK); 
     if(!LF3_UP){
       client.publish("STATUS", "32");
       LF3_UP=true;
       LF3_DOWN=false;
     }
     
     //action=0;
   }
  
}
void pot2_callback( int idx, int v, int up ) {
  
  if (v < LowerLimit2  && action==1)
  {  
    motor4.trigger(motor4.EVT_OFF);
    statusLED.blink(2000,500).trigger(statusLED.EVT_BLINK); 
    if (!LF4_DOWN)
    {
      client.publish("STATUS", "41");
      LF4_DOWN=true;
      LF4_UP=false;
    }
      
   }else if(v > UpperLimit2 && action==2){
     motor4.trigger(motor4.EVT_OFF);
     statusLED.blink(2000,500).trigger(statusLED.EVT_BLINK); 
     if(!LF4_UP){
       client.publish("STATUS", "42");
       LF4_UP=true;
       LF4_DOWN=false;
     }
     
     
   }
  
}

void button_change( int idx, int v, int up ) {
  
  if (pot1.state()< (LowerLimit -2) || pot1.state()> (UpperLimit + 2))
  {
    motor3.trigger( motor3.EVT_OFF );  
    motor4.trigger( motor4.EVT_OFF);  

  }
  if (pot2.state()< (LowerLimit - 2) || pot2.state() > (UpperLimit + 2))
  {
    motor3.trigger( motor3.EVT_OFF );  
    motor4.trigger( motor4.EVT_OFF);  

  }
  
 
  
}

void setup() {
  // Directional PIN ------------------------------------------
  pinMode(dirPin,OUTPUT);
  // Limit Switches Init ------------------------------------------
  downSwitch1.begin(downLimitPin1).debounce(500).onRelease(button_change);
  //downSwitch1.trace(Serial);
  upSwitch1.begin(upLimitPin1).debounce(500).onRelease(button_change);
  //upSwitch1.trace(Serial);
  //---------------------------------------------------------------
  upSwitch2.begin(upLimitPin2).debounce(500).onRelease(button_change);
  //upSwitch2.trace(Serial);
  downSwitch2.begin(downLimitPin2).debounce(500).onRelease( button_change);
  //downSwitch2.trace(Serial);
  //--------------------------------------------------------------
  pot1.begin(potPin1,50)
      .average(avgPot1Buffer, sizeof(avgPot1Buffer))
        .onChange(pot1_callback);
  pot2.begin(potPin2,50)
      .average(avgPot2Buffer, sizeof(avgPot2Buffer))
          .onChange(pot2_callback);
  // -------------------------------------------------------------
  // Motors Controls
  motor3.begin(motorPin1);
  motor4.begin(motorPin2);

  Serial.begin(9600);
  // print your local IP address: 
  // Allow the hardware to sort itself out
  delay(1500);
  statusLED.begin(ledPin);
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
  Ethernet.begin(mac, ip);
  Serial.println(Ethernet.localIP());

 
  
}

void loop() {
  
  #if mqtt == 1
  if (!client.connected()) {
    reconnect();
  }
 
  client.loop();
  #endif

 
  automaton.run();

  unsigned long currentMillis = millis();
  // Main Utility Task Loop
  if(currentMillis - previousMillis > polling_interval) {  
    previousMillis = currentMillis; 

    long pos1 = pot1.state() ;
    long pos2 = pot2.state(); 
    long err = pos1 - pos2; 

    #if debug ==1
    //(---------------------------------------------------)
    Serial.print("M3 -");
    Serial.println(pot1.state());
    Serial.println("---------");
    Serial.print("M4 -");
    Serial.println(pot2.state());
    //(----------------------------------------------------)

    Serial.print("Error ");
    Serial.println(err);
    Serial.print("Action ");
    Serial.println(action);
    #endif


    
    if (err > 0)   // Error is positive number--
    {
      if (action==2)
      {
        #if debug ==1
        Serial.print("Up > POS : ");
        Serial.println("PUSH 2");
        #endif
      // need to make function that reduces output the larger the error
       motor4.brightness(255 - (abs(err) * PULLCORRECTION) ) ;
       motor3.brightness(255 - (abs(err) * PUSHCORRECTION) );
       
      }
      if (action==1)
      {
        #if debug ==1
        Serial.print("Down>POS : ");
        Serial.println("PUSH 1");
        #endif
        
        motor4.brightness(255 - (abs(err) * PUSHCORRECTION));
        motor3.brightness(255 - (abs(err) * PULLCORRECTION) );
      }
    }

    if (err < 0)   // Error is negative number--
    {
      if (action==2)
      {
        
        #if debug==1
        Serial.print("Up >NEG: ");
        Serial.println("PUSH 2");
        #endif       
        motor4.brightness(255 - (abs(err) * PUSHCORRECTION));
        motor3.brightness(255 - (abs(err) * PULLCORRECTION) );
      }
      if (action==1)
      {
        #if debug ==1
        Serial.print("Down >NEG : ");
        Serial.println("PUSH 1");
        #endif
        motor4.brightness(255 - (abs(err) * PULLCORRECTION) );
        motor3.brightness(255 - (abs(err) * PUSHCORRECTION));
      }
    }
    
  
  }


}


