#include <AccelStepper.h>
#define HALFSTEP 8

// Motor pin definitions
int motorPin1 = 8;     // IN1 on the ULN2003 driver 1
int motorPin2 = 9;     // IN2 on the ULN2003 driver 1
int motorPin3 = 10;     // IN3 on the ULN2003 driver 1
int motorPin4 = 11;     // IN4 on the ULN2003 driver 1

int qte_bouffe = 5;
int poissons_nourris = 0;
int nourrir_state = 0;

// https://raspberrypi.stackexchange.com/questions/96653/ph-4502c-ph-sensor-calibration-and-adc-using-mcp3008-pcf8591-and-ads1115
#define PIN_PH A0 // Pin analogue pour lire le pHmetre
bool readpH = true;
float getpH();
float lastpH = 0;
float calibration = 0;
unsigned long int avgValue;
float b;
int buf[10], tempH;
char temp[10];
char buffer[10];

// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
AccelStepper stepper1(HALFSTEP, motorPin1, motorPin3, motorPin2, motorPin4);

void setup() {
  Serial.begin(9600);
  delay(2000); // So that it will not receive unwanted stuff while the esp is starting up)
  Serial.flush();
}//--(end setup )---

void loop() {
  if (nourrir_state == 1) {
    feedFishies();
    nourrir_state = 0;
  }
  if (readpH == true) {
    float pH = getpH();
    dtostrf(pH, 4, 2, buffer);
    Serial.write(buffer);
    delay(50);
    lastpH = pH;
    readpH = false;
  }

  while (!Serial.available()) {}
  Serial.readBytes(temp,10); //Read the serial data and store in var;
  float data = atof(temp);
  //Serial.print("temp ");Serial.println(data);
  
  if (data == 1)
    nourrir_state = data;
    
  else if (data > 10 && data < 100) {
    readpH = true;
    calibration = data;
    //Serial.print("calibration ");Serial.println(calibration);
  }
  
  else if (data != 0 && data < 10){
    qte_bouffe = data;
    dtostrf(qte_bouffe, 4, 2, buffer);
    Serial.write(buffer);
    //Serial.print("Qte bouffe ");Serial.println(qte_bouffe);
   }
  else
    Serial.flush();
  stepper1.setCurrentPosition(0);
  stepper1.moveTo(qte_bouffe * 1000);

}

void feedFishies() {
  stepper1.setMaxSpeed(1000.0);
  stepper1.setAcceleration(150.0);
  stepper1.setSpeed(15);
  while (stepper1.distanceToGo() > 0)
  {
    stepper1.run();

    //Change direction when the stepper reaches the target position
    if (stepper1.distanceToGo() == 0)
    {
      stepper1.setCurrentPosition(0);
      nourrir_state = 0;
      poissons_nourris++;
    }
  }
  //stepper1.moveTo(qte_bouffe);
}

float getpH() {

  for (int i = 0; i < 10; i++)
  {
    buf[i] = analogRead(PIN_PH);
    delay(30);
  }
  for (int i = 0; i < 9; i++)
  {
    for (int j = i + 1; j < 10; j++)
    {
      if (buf[i] > buf[j])
      {
        tempH = buf[i];
        buf[i] = buf[j];
        buf[j] = tempH;
      }
    }
  }
  avgValue = 0;
  for (int i = 2; i < 8; i++)
    avgValue += buf[i];
  float pHVol = (float)avgValue * 5.0 / 1024 / 6;
  float phValue = -5.70 * pHVol + calibration;
  return phValue;
}
