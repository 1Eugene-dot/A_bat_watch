#include <SPI.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ArduinoJson.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <Dns.h>
#include <Dhcp.h>
#include "EmonLib.h"

/************************* Ethernet Client Setup *****************************/
byte mac[] = {0xDA, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};


#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "...your AIO username (see https://accounts.adafruit.com)..."
#define AIO_KEY         "...your AIO password..."

//Set up the ethernet client
EthernetClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// You don't need to change anything below this line!
#define halt(s) { Serial.println(F( s )); while(1);  }


/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish BatData = Adafruit_MQTT_Publish(&mqtt,  AIO_USERNAME "/Data");

/*****************************Pin Setup **************************************/

int PowerLed = 2; // Power LED
int Connected = 3; // Connected to MQTT server
int Run = 4; // Hearbeat
int DC_in = 0; // voltage div analog in
int CT1 = 1; //Current clamp 1 analog in
int CT2 = 2; //Curent clamp 2 analog in

EnergyMonitor emon1;

float CT_correction = 111.1;
char json_data[200];
float CT1_old = 0.0;
float CT2_old = 0.0;
float DC_old = 0.0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(PowerLed, OUTPUT);
  digitalWrite(PowerLed, HIGH);
  pinMode(Connected, OUTPUT);
  pinMode(Run, OUTPUT);

  Ethernet.begin(mac);
  delay(1000); //give the ethernet a second to initialize

}

void loop() {
  StaticJsonDocument<200> doc;
  delay(1000);
  digitalWrite(Run, HIGH);
  doc["CT1"] = getCT(CT1);
  doc["CT2"] = getCT(CT2);
  doc["A1"] = getAnalog(DC_in);
  // put your main code here, to run repeatedly:
  MQTT_connect();

  if (getCT(CT1) != CT1_old || getCT(CT2) != CT2_old || getAnalog(DC_in) != DC_old) {
    // Now we can publish stuff!
    serializeJson(doc, json_data);
    if (! BatData.publish(json_data)) {
      Serial.println(F("Failed"));
    } else {
      Serial.println(F("OK!"));
      CT1_old = getCT(CT1);
      CT2_old = getCT(CT2);
      DC_old = getAnalog(DC_in);
    }
  }
  // ping the server to keep the mqtt connection alive
  if (! mqtt.ping()) {
    digitalWrite(Connected, LOW);
    mqtt.disconnect();
  }


  delay(1000);
  digitalWrite(Run, LOW);
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    digitalWrite(Connected, HIGH);
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    digitalWrite(Connected, LOW);
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");
}

float getCT(int pin) {
  emon1.current(pin, CT_correction);
  delay(500);
  double Irms = emon1.calcIrms(1480);  // Calculate Irms only
  Serial.print(Irms * 230.0);         // Apparent power
  Serial.print(" ");
  Serial.println(Irms);
  return (Irms * 230.0);
}

float getAnalog(int pin) {
  float pin_data = analogRead(pin);

  float return_data = (pin_data / 5.00) * 100;

  return return_data;
}
