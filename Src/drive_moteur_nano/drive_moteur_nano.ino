#include <AccelStepper.h>
#define HALFSTEP 8

// Motor pin definitions
int motorPin1 = 8;     // IN1 on the ULN2003 driver 1
int motorPin2 = 9;     // IN2 on the ULN2003 driver 1
int motorPin3 = 10;     // IN3 on the ULN2003 driver 1
int motorPin4 = 11;     // IN4 on the ULN2003 driver 1

  int qte_bouffe = 2;
  int poissons_nourris = 0;
  int nourrir_state = 0;
  
// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
AccelStepper stepper1(HALFSTEP, motorPin1, motorPin3, motorPin2, motorPin4);

void setup() {
  Serial.begin(9600);
  delay(2000); // So that it will not receive unwanted stuff while the esp is starting up)

}//--(end setup )---

void loop() {
  if (nourrir_state == 1) {
    feedFishies();
    nourrir_state = 0;
  } 


  //int temp = b2  + b1 * 256 ; 
  while (!Serial.available()){}
  int temp =  Serial.parseInt();
  Serial.println(temp);

  if (temp == 1)
    nourrir_state = temp;
  else if (temp != 0 && temp < 10)
    qte_bouffe = temp;

  stepper1.setCurrentPosition(0);
  stepper1.moveTo(qte_bouffe*1000);

}

void feedFishies() {
  stepper1.setMaxSpeed(1000.0);
  stepper1.setAcceleration(100.0);
  stepper1.setSpeed(15);
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
