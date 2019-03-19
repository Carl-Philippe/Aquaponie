#include <AccelStepper.h>
#define HALFSTEP 8

// Motor pin definitions
#define motorPin1  8     // IN1 on the ULN2003 driver 1
#define motorPin2  9     // IN2 on the ULN2003 driver 1
#define motorPin3  10     // IN3 on the ULN2003 driver 1
#define motorPin4  11     // IN4 on the ULN2003 driver 1

  int qte_bouffe = 10000;
  int poissons_nourris = 0;
  int nourrir_state = 1;
  
// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
AccelStepper stepper1(HALFSTEP, motorPin1, motorPin3, motorPin2, motorPin4);

void setup() {
  Serial.begin(115200);

}

void loop() {
  if (nourrir_state == 1) {
    feedFishies();
    nourrir_state = 0;
  } 
  while (!Serial.available()){}
    byte b1 = Serial.read();
  while (!Serial.available()){}
    byte b2 = Serial.read();

    nourrir_state = b2  + b1 * 256 ; 
    Serial.println(nourrir_state);
    stepper1.moveTo(qte_bouffe);
    Serial.println(stepper1.distanceToGo());

}

void feedFishies() {
  stepper1.setMaxSpeed(1000.0);
  stepper1.setAcceleration(100.0);
  stepper1.setSpeed(15);
  stepper1.moveTo(qte_bouffe);
  while(stepper1.distanceToGo() > 0)
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
