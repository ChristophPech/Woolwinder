#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <Fonts/FreeMono9pt7b.h>

#include <gfxfont.h>
#include <Arduino.h>
#include "DRV8825.h"
#include "TimerOne.h"


const int GEAR_A=5; //25
const int GEAR_B=6; //30
const int STEPS_PERREV=200;
// using a 200-step motor (most common)
// pins used are DIR, STEP, MS1, MS2, MS3 in that order
DRV8825 stepper(STEPS_PERREV, 8, 9, 10, 11, 12);


#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define D_BG

#ifdef DBG
#define Serial_begin Serial.begin
#define Serial_println Serial.println
#define Serial_print Serial.print
#else
#define Serial_begin(x)
#define Serial_println(x) 
#define Serial_print(x)
#endif

const int PinCLK=3;                   // Used for generating interrupts using CLK signal
const int PinDT=7;                    // Used for reading DT signal
const int PinSW=6;                    // Used for the push button switch
bool bLastCLK=false;

void setup() {
    Serial_begin(9600);
    Serial_println("Test DRV8825");

    stepper.setRPM(200);
    stepper.setMicrostep(1);
    stepper.disable();

    pinMode(4, OUTPUT);
    pinMode(2, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);

    pinMode(PinCLK,INPUT);
    pinMode(PinDT,INPUT);  
    pinMode(PinSW,INPUT_PULLUP);
    //attachInterrupt (1,isr,CHANGE);   // interrupt 1 is always connected to pin 3 on Arduino UNO
    bLastCLK=digitalRead(PinCLK);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3D (for the 128x64)
    //display.setFont(&FreeMono9pt7b);

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,1);
    display.print("Woolwinder");
    display.display();
    delay(1000);

    Timer1.initialize(100);  // 10 us = 100 kHz
    Timer1.attachInterrupt(stepperAdvance);
    Serial_println("init.done");
}

unsigned int cnt=0;
bool isOn=false;
int iMicro=1;
unsigned int iLastMicro=1;
int iRPM=0;
//float fLastRPM=0;
unsigned long iStepDur=0;
unsigned int iStepCount=0;
unsigned int iRevCount=0;
bool bAdvance=false;

/*void isr ()  {                    // Interrupt service routine is executed when a HIGH to LOW transition is detected on CLK
  bool bCLK=digitalRead(PinCLK);
  bool bDT=digitalRead(PinDT);

      Serial_print(bCLK);
      Serial_print(",");
      Serial_print(bDT);
      Serial_print("\n");
  
  bool up = bCLK == bDT;
  if(up) iRPM-=5; else iRPM+=5;
  if(iRPM<0) iRPM=0;
  if(iRPM>400) iRPM=400;
}*/

void loop() {
  handleDisplay();
  delay(20);
  
  cnt++;
  digitalWrite(LED_BUILTIN, ((cnt/10)&1)!=0);

  bool on=iRPM>0;
  bAdvance=false;
  
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
      Serial_print(iRPM);
      Serial_print(",");
      Serial_print("on\n");
    }
  }

  iMicro=1;
  if(iRPM<1) return;
  
  float fHZ=float(iRPM)/60.0;
  float fDur=50000;
  if(fHZ>0.001) {
    fDur=(1000000.0/fHZ)/float(STEPS_PERREV);
    fDur=(fDur*GEAR_A)/GEAR_B;
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
  if(fDur>iMax) {iMicro*=2;fDur/=2.0;} //32 /**/

  iStepDur=fDur;
  stepper.setMicrostep(iMicro);
  iLastMicro=iMicro;
  bAdvance=true;

#ifdef DBG
  //*
  unsigned int iNM=millis();
  static unsigned int iLastLog=0;
  if(iNM>iLastLog+500) {
    iLastLog=iNM;
    float d=1000000.0;
    d/=(iStepDur*2)*float(STEPS_PERREV*iMicro);
    int rpm=d*60.0;

    const int iMSTEPS=(long(STEPS_PERREV*32)*GEAR_B)/GEAR_A;

    Serial_print(iRPM);
    Serial_print(",");
    Serial_print(iStepDur);
    Serial_print(",");
    Serial_print(iMicro);
    Serial_print(",");
    Serial_print(iStepCount);
    Serial_print(",");
    Serial_print(iMSTEPS);
    //Serial_print(",");
    //Serial_print(v);
    Serial_print("\n");
    cnt=0;
  }/**/
#endif

  //Serial.println("loop");
}

unsigned long iLastStep=0;
//bool bStepOn=false;
unsigned int iClick=0;
void stepperAdvance() {
  unsigned long iNow=micros();
  if(iNow<iLastStep) {iLastStep=iNow;}; //overflow

  bool bCLK=digitalRead(PinCLK);
  if(bLastCLK!=bCLK) {
    bLastCLK=bCLK;
    bool bDT=digitalRead(PinDT);
    if(bDT!=bCLK) {
      iRPM+=5;
      if(iRPM>400) iRPM=400;
    } else {
      iRPM-=5;
      if(iRPM<0) iRPM=0;
    }
 }
  
  if (!(digitalRead(PinSW))) {
    //Serial_print(iClick);
    iClick++;
    if(iClick>7000) {iStepCount=0;iRevCount=0;};
    iRPM=0;
  } else {iClick=0;}

  if(bAdvance && (iNow>iLastStep+iStepDur)) {
    //bStepOn=!bStepOn;
    //digitalWrite(9, bStepOn);
    digitalWrite(9, HIGH);
    iLastStep+=iStepDur;
    if(iNow>iLastStep+iStepDur) iLastStep=iNow; //fast forward

    //if(bStepOn) {
    switch (iMicro) {
      case 1: iStepCount+=32;break;
      case 2: iStepCount+=16;break;
      case 4: iStepCount+=8;break;
      case 8: iStepCount+=4;break;
      case 16: iStepCount+=2;break;
      case 32: iStepCount+=1;break;
    }

    //iDbg++;
    //}
    const int iMSTEPS=(long(STEPS_PERREV*32)*GEAR_B)/GEAR_A;
    if(iStepCount>=iMSTEPS) {
      iStepCount-=iMSTEPS;iRevCount++;
    }

    delayMicroseconds(4);
    digitalWrite(9, LOW);
  }
}

void handleDisplay() {
    String rpm=String(iRPM);
    String cnt=String(iRevCount);
  
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


