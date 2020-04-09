/************************************** OTA *****************************************/
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#ifndef STASSID
#define STASSID "SmartRGE1BB"
#define STAPSK  "s9003502"
#endif

// WiFi network info.
const char* ssid = STASSID;
const char* wifiPassword = STAPSK;
/************************************* CAYENNE *****************************************/

//#define CAYENNE_DEBUG
#define CAYENNE_PRINT Serial                 //wifi
#include <CayenneMQTTESP8266.h>

// If too many errors when connecting, deep sleep (which will restart the esp)
// PIN D0 OF THE NODEMCU MUST BE CONNECTed TO THE RST PIN
int failedConnections = 0;
#define SLEEP_TIME 20e6

#include <DHTesp.h> // DHTesp library

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "6e7efa60-b6a3-11e8-a5e0-433900986fca";
char password[] = "70a3b8c6b91c0e52f4341fa54b8b3f87bfc0d0f5";
char clientID[] = "d301fc90-fe7d-11e8-809d-0f8fe4c30267";

#define VIRTUAL_CHANNEL 1 // Feed the fishies
#define VIRTUAL_CHANNEL 2 // Flotteur C reservoir a poissons manque d'eau
#define VIRTUAL_CHANNEL 3 // Etat de la lampe
#define VIRTUAL_CHANNEL 4 // Temps entre cycles
#define VIRTUAL_CHANNEL 5 // 
#define VIRTUAL_CHANNEL 6
#define VIRTUAL_CHANNEL 7
#define VIRTUAL_CHANNEL 8 // float A
#define VIRTUAL_CHANNEL 9 // float b
#define VIRTUAL_CHANNEL 10
#define VIRTUAL_CHANNEL 11
#define VIRTUAL_CHANNEL 12 // Qte de bouffe a donner
#define VIRTUAL_CHANNEL 13 // Feedback bouffe a donner
#define VIRTUAL_CHANNEL 14 // pH sensor
#define VIRTUAL_CHANNEL 15 // pH sensor
#define VIRTUAL_CHANNEL 16 // pH calibration
#define VIRTUAL_CHANNEL 17 // get pH
#define VIRTUAL_CHANNEL 18 // Flotteur réservoir pompe

#define PRISE_LAMPE D2     // Prise controllable 1
#define PRISE_POMPE D1    // Prise controllable 2
#define PRISE_B D3  // Prise controllable 3

#define PIN_FLOAT_A D7  // Flotteur bas du circuit des plantes
#define PIN_FLOAT_B D5  // Flotteur haut du circuit des plantes
#define PIN_FLOAT_C D6  // Flotteur sécurité du réservoir à poissons
#define PIN_FLOAT_D A0 // GPIO 9 = SD2 Flotteur réservoir d'eau pompe


float calibration = 22.68; //change this value to calibrate the ph sensor
float pHsensorValue = 0;
char receive[10];
char send[10];
void updatepH();


#define DHTPIN D4
DHTesp dht;

#define DELAIS_ACTIONNEURS 2000
#define DUREE_REMPLISSAGE_MAX 180000
#define TEMPS_ECLAIRAGE_JOUR 72000000 // 20h not used if cayenne schedule works
#define TEMPS_ECLAIRAGE_NUIT 72000000 // 20h
#define TEMPS_PRISE_PH 450000 //15min // 2min30

// Valeurs par defaut
int duree_pompage = 10; // Secondes de durée de pompe
int nourrir_state = 0;
int qte_bouffe = 8;
int qte_bouffe_confirm = qte_bouffe;
int temps_entre_remplissages = 900000; //15min par defaut

float hum, temp;
float hum_pres, temp_pres;

unsigned long timerDernierPompage = 0;
unsigned long timerPompage = 0; //
unsigned long timerLampe = 0; //
unsigned long timerDernierpH = 0;

bool flotteur_A = 0;
bool flotteur_B = 0;
bool flotteur_C = 0;
bool flotteur_D = 0;
bool etat_pompe = 0;
bool etat_lampe = 0;
bool etat_priseb = 0;

/************************************************************************************/
void setup() {
  Serial.begin(9600);
  //Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, wifiPassword);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    //Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  // Port defaults to 8266
  ArduinoOTA.setPort(8466);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("Esp_aquap");

  // No authentication by default
  ArduinoOTA.setPassword("Carlphilippe1");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    //Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    //Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    //Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      //Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      //Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      //Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      //Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      //Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();

  //Serial.println("Ready");
  //Serial.print("IP address: ");
  //Serial.println(WiFi.localIP());

  pinMode(PRISE_LAMPE, OUTPUT);
  pinMode(PRISE_POMPE, OUTPUT);
  pinMode(PRISE_B, OUTPUT);
  pinMode(PIN_FLOAT_D, INPUT);
  digitalWrite(PRISE_LAMPE, HIGH);
  digitalWrite(PRISE_POMPE, HIGH);
  digitalWrite(PRISE_B, HIGH);
  Cayenne.begin(username, password, clientID, ssid, wifiPassword);
  //dht.begin();

  CAYENNE_IN(12);
  CAYENNE_IN(4);
  CAYENNE_OUT(13);
  CAYENNE_OUT(10);
  CAYENNE_OUT(14);
  CAYENNE_IN(15);
  CAYENNE_OUT(16);
  CAYENNE_IN(17);
  CAYENNE_OUT(18);
  dht.setup(DHTPIN, DHTesp::DHT22);
}
/************************************************************************************/
void loop() {
  /********************* Acquisition de data ****************************/
  if (WiFi.status() == WL_CONNECTED) {                                           // If connected to internet, proceed normally
    Cayenne.loop();                                                              // Proceed with cayenne.loop()
    ArduinoOTA.handle();
    delay(1000);  // For testing this file.
  } else {
    failedConnections++;
    // If we failed the connection more than ten times, deep sleep and retry another time
    if (failedConnections > 10) {
      ESP.deepSleep(SLEEP_TIME);                                                // PIN D0 OF THE NODEMCU MUST BE CONNECT TO THE RST PIN
    } else {
      // Sinon, delay 1000
      delay(1000);
    }
  }

  flotteur_A = digitalRead(PIN_FLOAT_A);
  flotteur_B = digitalRead(PIN_FLOAT_B);
  flotteur_C = digitalRead(PIN_FLOAT_C);
  int analD = analogRead(PIN_FLOAT_D);
  if (analD <= 500)
    flotteur_D = 0;
  else
    flotteur_D = 1;
  //hum = dht.readHumidity();
  //temp = dht.readTemperature();
  hum = dht.getHumidity();
  temp = dht.getTemperature();


  /* Serial.println();
    Serial.print("temps entre remplissages "); Serial.println(temps_entre_remplissages);
    Serial.print("flotteur_A "); Serial.println(flotteur_A);
    Serial.print("flotteur_B "); Serial.println(flotteur_B);
    Serial.print("flotteur_C "); Serial.println(flotteur_C);
    Serial.print("humidite "); Serial.println(hum);
    Serial.print("temperature "); Serial.println(temp);
    Serial.print("nourrir state "); Serial.println(nourrir_state);


    /********************* Selection de la situation selon data**********************/

  /*if (flotteur_C == LOW)
    {
    Cayenne.virtualWrite(2, LOW); // Ne pompe pas et envoie à cayenne une alerte (remplir aquarium)
    }
    else */
  if (millis() - timerDernierpH >= TEMPS_PRISE_PH) {
    CAYENNE_IN(17);
    timerDernierpH = millis();
  }

  if (millis() - timerDernierPompage >= temps_entre_remplissages)
  {
    if ( !flotteur_B && !flotteur_A && !flotteur_D && !etat_pompe)
    {
      digitalWrite(PRISE_POMPE, LOW); // Demarre la pompe
      etat_pompe = true;
      timerPompage = millis();
    }
    else if (millis() - timerPompage >= DUREE_REMPLISSAGE_MAX || flotteur_B || flotteur_A || flotteur_D) // Protection bris du flotteur
    {
      digitalWrite(PRISE_POMPE, HIGH); // Arrete la pompe
      etat_pompe = false;
      timerDernierPompage = millis();
    }
  }

  if ( etat_lampe == 1 && millis() - timerLampe >= TEMPS_ECLAIRAGE_NUIT)
  {
    timerLampe = millis();
    etat_lampe = 0;
    digitalWrite(PRISE_LAMPE, etat_lampe);
    Cayenne.virtualWrite(3, etat_lampe);
  }
  else if (etat_lampe == 0 && millis() - timerLampe >= TEMPS_ECLAIRAGE_JOUR)
  {
    timerLampe = millis();
    etat_lampe = 1;
    digitalWrite(PRISE_LAMPE, etat_lampe);
    Cayenne.virtualWrite(3, etat_lampe);
  }
  Cayenne.loop();
}

/***************************************Communication avec cayenne****************************************/
// Read the time to pump
CAYENNE_IN(1)
{
  nourrir_state = 1;
  dtostrf(nourrir_state, 4, 3, send);
  Serial.write(send, 10); // send 10 bytes through serial
  delay(50); //allows all // Serial sent to be received together
  nourrir_state = 0;
}
// Alerte manque d'eau
CAYENNE_OUT(2)
{
  Cayenne.virtualWrite(2, flotteur_C, "digital_sensor", "d");
}
// Lecture de l'ajustement cayenne du temps entre les remplissages
CAYENNE_IN(3)
{
  etat_lampe = (bool) getValue.asInt();
  digitalWrite(PRISE_LAMPE, etat_lampe);
  timerLampe = millis();
}
CAYENNE_IN(4)
{
  int new_value = getValue.asInt();
  temps_entre_remplissages = new_value * 1000 * 60;
}
CAYENNE_OUT(5)
{
  if (hum != hum_pres && !isnan(hum))
  {
    Cayenne.virtualWrite(5, hum);
  }
  hum_pres = hum;
}
CAYENNE_OUT(6)
{
  if (temp != temp_pres && !isnan(temp))
  {
    Cayenne.virtualWrite(6, temp);
  }
  temp_pres = temp;
}
CAYENNE_OUT(7)
{
  Cayenne.virtualWrite(7, flotteur_B, "digital_sensor", "d");
}
CAYENNE_OUT(8)
{
  Cayenne.virtualWrite(8, flotteur_A, "digital_sensor", "d");
}
CAYENNE_IN(9)
{
  int etat_pompe = (bool) getValue.asInt();
  digitalWrite(PRISE_POMPE, etat_pompe);
}
CAYENNE_OUT(10)
{
  Cayenne.virtualWrite(10, temps_entre_remplissages / (60 * 1000));
}
CAYENNE_IN(11)
{
  int etat_priseb = (bool) getValue.asInt();
  digitalWrite(PRISE_B, etat_priseb);
}
CAYENNE_IN(12)
{
  int new_value = getValue.asInt();
  qte_bouffe = new_value;
  dtostrf(qte_bouffe, 4, 3, send);
  Serial.write(send, 10); // send 10 bytes through serial
  delay(50);
  Serial.readBytes(receive, 10);
  qte_bouffe_confirm = atof(receive);
  Cayenne.virtualWrite(13, qte_bouffe_confirm);

}
CAYENNE_OUT(13)
{
  Cayenne.virtualWrite(13, qte_bouffe_confirm);
}

CAYENNE_OUT(14)
{
  Cayenne.virtualWrite(14, pHsensorValue);
}

CAYENNE_IN(15)
{
  String value0;
  value0 = (String)getValue.asString();
  float new_value = value0.toFloat();
  calibration = new_value;
  Cayenne.virtualWrite(16, calibration);
}

CAYENNE_OUT(16) {
  Cayenne.virtualWrite(16, calibration);
}

CAYENNE_IN(17) {
  updatepH();
  Cayenne.virtualWrite(14, pHsensorValue);
}

CAYENNE_OUT(18) {
  Cayenne.virtualWrite(18, flotteur_D, "digital_sensor", "d"); // Alerte manque d'eau
}

void updatepH() {
  float validate = 666;
  while (validate < 0 || validate > 14) {
    dtostrf(calibration, 4, 3, send);
    Serial.write(send, 10); // send 10 bytes through serial
    delay(50);
    Serial.readBytes(receive, 10);
    validate = atof(receive);
  }
  pHsensorValue = validate;
}
