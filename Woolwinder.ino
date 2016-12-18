#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <Fonts/FreeMono9pt7b.h>

#include <gfxfont.h>
#include <Arduino.h>
#include "DRV8825.h"
#include "TimerOne.h"



const int STEPS_PERREV=200;
// using a 200-step motor (most common)
// pins used are DIR, STEP, MS1, MS2, MS3 in that order
DRV8825 stepper(STEPS_PERREV, 8, 9, 10, 11, 12);


#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define Serial_begin Serial.begin
#define Serial_println Serial.println
#define Serial_print Serial.print

void setup() {
    Serial_begin(9600);
    Serial_println("Test DRV8825");

    stepper.setRPM(200);
    stepper.setMicrostep(1);
    stepper.disable();

    pinMode(4, OUTPUT);
    pinMode(2, INPUT_PULLUP);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3D (for the 128x64)
    //display.setFont(&FreeMono9pt7b);

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,1);
    display.print("Woolwinder");
    display.display();
    delay(1000);

    Timer1.initialize(20);  // 10 us = 100 kHz
    Timer1.attachInterrupt(stepperAdvance);
    Serial_println("init.done");
}

int cnt=0;
bool isOn=false;
int iMicro=1;
int iLastMicro=1;
int iRPM=0;
float fLastRPM=0;
unsigned long iStepDur=0;

void loop() {
  handleDisplay();
  delay(20);
  
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
      iRPM=0;
      handleDisplay();
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
  if(fRPM-fLastRPM>20) fRPM=fLastRPM+20;
  fLastRPM=fRPM;
  
  iRPM=fRPM;
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

  iStepDur=fDur/2;
  stepper.setMicrostep(iMicro);
  iLastMicro=iMicro;

  /*int iNM=millis();
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
  }*/

  //Serial.println("loop");
}

unsigned long iLastStep=0;
unsigned long iStepCount=123;
bool bStepOn=false;
void stepperAdvance() {
  unsigned long iNow=micros();

  if(iRPM>0 && iNow>iLastStep+iStepDur) {
    bStepOn=!bStepOn;
    digitalWrite(9, bStepOn);
    iLastStep=iNow;
    if(bStepOn) iStepCount++;
  }
}

void handleDisplay() {
    String rpm=String(iRPM);
    String cnt=String(iStepCount/STEPS_PERREV);
  
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,1);
    display.print("RPM: ");
    display.println(rpm);
    display.setTextSize(4);
    display.setCursor(0,20);
    display.println(cnt);
    display.display();
}


