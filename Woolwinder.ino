

#include <Arduino.h>
#include "DRV8825.h"

const int STEPS_PERREV=200;
// using a 200-step motor (most common)
// pins used are DIR, STEP, MS1, MS2, MS3 in that order
DRV8825 stepper(STEPS_PERREV, 8, 9, 10, 11, 12);

#define Serial_begin Serial.begin
#define Serial_println Serial.println
#define Serial_print Serial.print

void setup() {
    // Set target motor RPM to 1RPM
    stepper.setRPM(200);
    // Set full speed mode (microstepping also works for smoother hand movement
    stepper.setMicrostep(1);
    //stepper.disable();

    pinMode(4, OUTPUT);
    //pinMode(13, INPUT);
    //digitalWrite(3, LOW);

    pinMode(2, INPUT_PULLUP);

    Serial_begin(9600);
    Serial_println("Test DRV8825");
}

int cnt=0;
bool isOn=false;
//bool stepOn=false;
int iMicro=1;
unsigned long iNow=0;
int iLastMicro=1;

void loop() {
  cnt++;
  int v = analogRead(1);
  bool on=digitalRead(2)==0; 
  //Serial.println(on);
  if(v<10) {
    //on=false;
  }
  
  if(!on) {
    digitalWrite(4, LOW);
    if(isOn) {
      isOn=false;
      Serial_print("off\n");
      delay(500);
    }
    delay(1);
    return;
  } else {
    if(!isOn) {
      isOn=true;
      digitalWrite(4, HIGH);
      Serial_print("on\n");
    }
  }

  iMicro=1;

  float fRPM=float(v)/2.5;
  if(fRPM<1) return;
  
  float fHZ=fRPM/60.0;
  float fDur=50000;
  if(fHZ>0.001) {
    fDur=(1000000.0/fHZ)/float(STEPS_PERREV);
  }

  const int iMaxM=2000;int iMax;
  iMax=(iMicro==iLastMicro)?iMaxM+100:iMaxM;
  if(fDur>iMax) {iMicro*=2;fDur/=2.0;} //2
  iMax=(iMicro==iLastMicro)?iMaxM+100:iMaxM;
  if(fDur>iMax) {iMicro*=2;fDur/=2.0;} //4
  iMax=(iMicro==iLastMicro)?iMaxM+100:iMaxM;
  if(fDur>iMax) {iMicro*=2;fDur/=2.0;} //8
  iMax=(iMicro==iLastMicro)?iMaxM+100:iMaxM;
  if(fDur>iMax) {iMicro*=2;fDur/=2.0;} //16
  iMax=(iMicro==iLastMicro)?iMaxM+100:iMaxM;
  if(fDur>iMax) {iMicro*=2;fDur/=2.0;} //32

  long dur=fDur;
  stepper.setMicrostep(iMicro);
  iLastMicro=iMicro;

  int iNM=millis();
  static int iLastLog=0;
  if(iNM>iLastLog+500) {
    iLastLog=iNM;
    float d=1000000.0;
    d/=dur*float(STEPS_PERREV*iMicro);
    int rpm=d*60.0;
    
    Serial_print(fRPM);
    Serial_print(",");
    Serial_print(fDur);
    Serial_print(",");
    Serial_print(iMicro);
    Serial_print(",");
    Serial_print(rpm);
    cnt=0;
  }
  
  unsigned long next_edge = iNow + dur;
  microWaitUntil(next_edge);
  digitalWrite(9, HIGH);
  delayMicroseconds(4);
  digitalWrite(9, LOW);

  iNow=micros();
  //Serial.println("loop");
}