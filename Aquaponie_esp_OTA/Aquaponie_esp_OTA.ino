/************************************** OTA *****************************************/
const int FW_VERSION = 2;                                                        // Version number, don't forget to update this on changes
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
// Note the raw.githubuserconent, this allows us to access the contents at the url, not the webpage itself
const char* fwURLBase = "https://raw.githubusercontent.com/BIUS-USherbrooke/BIUS_Domotique/master/Modules/ESPOTAexample/ESPOTAexample"; // IP adress to the subfolder containing the binary and version number for this specific device

/************************************* CAYENNE *****************************************/

#define CAYENNE_DEBUG
#define CAYENNE_PRINT Serial                 //wifi
#include <CayenneMQTTESP8266.h>
//#include <DHT.h>  // DHTsensor
#include <DHTesp.h> // DHTesp library, a verifier laquelle fonctionne le mieux avec cayenne.

// WiFi network info.
char ssid[] = "SmartRGE1BB";
char wifiPassword[] = "s9003502";

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "6e7efa60-b6a3-11e8-a5e0-433900986fca";
char password[] = "70a3b8c6b91c0e52f4341fa54b8b3f87bfc0d0f5";
char clientID[] = "d301fc90-fe7d-11e8-809d-0f8fe4c30267";

#define VIRTUAL_CHANNEL 1 // Feed the fishies
#define VIRTUAL_CHANNEL 2 // Flotteur C manque d'eau
#define VIRTUAL_CHANNEL 3 // Etat de la lampe
#define VIRTUAL_CHANNEL 4 // Temps entre cycles
#define VIRTUAL_CHANNEL 5 // 
#define VIRTUAL_CHANNEL 6
#define VIRTUAL_CHANNEL 7
#define VIRTUAL_CHANNEL 8
#define VIRTUAL_CHANNEL 9
#define VIRTUAL_CHANNEL 10
#define VIRTUAL_CHANNEL 11

#define PRISE_LAMPE D2     // Prise controllable 1
#define PRISE_POMPE D1    // Prise controllable 2
#define PRISE_B D3  // Prise controllable 3

#define PIN_FLOAT_A D4  // Flotteur bas du circuit des plantes
#define PIN_FLOAT_B D5  // Flotteur haut du circuit des plantes
#define PIN_FLOAT_C D6  // Flotteur sécurité du réservoir à poissons

#define DHTPIN D7
//#define DHTTYPE DHT11
//DHT dht(DHTPIN, DHTTYPE);
DHTesp dht;

#define DELAIS_ACTIONNEURS 2000
#define DUREE_REMPLISSAGE_MAX 180000
#define TEMPS_ECLAIRAGE_JOUR 43200000 // 12h
#define TEMPS_ECLAIRAGE_NUIT 43200000 // 12h

int duree_pompage = 10; // Secondes de durée de pompe
int nourrir_state = 0;
int temps_entre_remplissages = 1800000; //2h par defaut

float hum, temp;
float hum_pres, temp_pres;

unsigned long timerDernierPompage = 0;
unsigned long timerPompage = 0; // 
unsigned long timerLampe = 0; //

bool flotteur_A = 0;
bool flotteur_B = 0;
bool flotteur_C = 0;
bool etat_pompe = 0;
bool etat_lampe = 0;
bool etat_priseb = 0;

/************************************************************************************/
void setup() {
  Serial.begin(9600);
  pinMode(PRISE_LAMPE, OUTPUT);
  pinMode(PRISE_POMPE, OUTPUT);
  pinMode(PRISE_B,OUTPUT);
  digitalWrite(PRISE_LAMPE, HIGH);
  digitalWrite(PRISE_POMPE, HIGH);
  digitalWrite(PRISE_B,HIGH);
  Cayenne.begin(username, password, clientID, ssid, wifiPassword);
  //dht.begin();
  dht.setup(DHTPIN, DHTesp::DHT11);
}
/************************************************************************************/
void loop() {
  /********************* Acquisition de data ****************************/
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 50) {
    delay(500);
    i++;
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Connection failed, restart");// echoué = reset
    WiFi.disconnect();
    delay(500);
    //ESP.restart();
    Cayenne.begin(username, password, clientID, ssid, wifiPassword);
  }
  flotteur_A = digitalRead(PIN_FLOAT_A);
  flotteur_B = digitalRead(PIN_FLOAT_B);
  flotteur_C = digitalRead(PIN_FLOAT_C);
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
  if (millis() - timerDernierPompage >= temps_entre_remplissages)
  {
    if ( flotteur_B == 0 )
    {
      digitalWrite(PRISE_POMPE, LOW); // Demarre la pompe
      timerPompage = millis();
    }
    if (millis() - timerPompage >= DUREE_REMPLISSAGE_MAX || flotteur_B == 1) // Protection bris du flotteur
    {
        digitalWrite(PRISE_POMPE, HIGH); // Arrete la pompe
        timerDernierPompage = millis();
    }
  }

  if ( etat_lampe == 1 && millis() - timerLampe >= TEMPS_ECLAIRAGE_JOUR)
  {
    timerLampe = millis();
    etat_lampe = 0;
    digitalWrite(PRISE_LAMPE, etat_lampe);
    Cayenne.virtualWrite(3, etat_lampe);
  }
  else if (etat_lampe == 0 && millis() - timerLampe >= TEMPS_ECLAIRAGE_NUIT)
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
int currentValue = getValue.asInt();
  if (currentValue == 1)
  {
    nourrir_state = 1;
    Serial.write(nourrir_state / 256);
    Serial.write(nourrir_state % 256);
    delay(75);  //allows all serial sent to be received together
    nourrir_state = 0;
  }
  else
  { 
    nourrir_state = 0;
  }
}
// Alerte manque d'eau
CAYENNE_OUT(2)
{
  Cayenne.virtualWrite(2, flotteur_C,"digital_sensor","d");
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
      Cayenne.virtualWrite(5,hum);
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
  Cayenne.virtualWrite(7, flotteur_B,"digital_sensor","d");
}
CAYENNE_OUT(8)
{
  Cayenne.virtualWrite(8, flotteur_A,"digital_sensor","d");
}
CAYENNE_IN(9)
{
  int etat_pompe = (bool) getValue.asInt();
  digitalWrite(PRISE_POMPE, etat_pompe);
}
CAYENNE_OUT(10)
{
Cayenne.virtualWrite(10,temps_entre_remplissages/(60*1000));
}
CAYENNE_IN(11)
{
  int etat_priseb = (bool) getValue.asInt();
  digitalWrite(PRISE_B, etat_priseb);
}
