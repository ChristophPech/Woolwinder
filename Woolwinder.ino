#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <Fonts/FreeMono9pt7b.h>

#include <gfxfont.h>
#include <Arduino.h>
#include "TimerOne.h"

#include "AsyncStepperLib.h"
#include <Stepper.h>

const int GEAR_A = 17; //25
const int GEAR_B = 63; //30
const int SIGPR = 24;

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

const int PinCLK = 7;                 // Used for generating interrupts using CLK signal
const int PinDT = 8;                  // Used for reading DT signal
const int PinSW = 9;                  // Used for the push button switch
bool bLastCLK = false;

const int motorPin1 = 2;  
const int motorPin2 = 3;  
const int motorPin3 = 4; 
const int motorPin4 = 5; 
const int numSteps = 8;
const int stepsLookup[8] = { B1000, B1100, B0100, B0110, B0010, B0011, B0001, B1001 };
int stepCounter = 0; 

void clockwise()
{
  stepCounter++;
  if (stepCounter >= numSteps) stepCounter = 0;
  setOutput(stepCounter);
}

void anticlockwise()
{
  stepCounter--;
  if (stepCounter < 0) stepCounter = numSteps - 1;
  setOutput(stepCounter);
}

void setOutput(int step)
{
  digitalWrite(motorPin1, bitRead(stepsLookup[step], 0));
  digitalWrite(motorPin2, bitRead(stepsLookup[step], 1));
  digitalWrite(motorPin3, bitRead(stepsLookup[step], 2));
  digitalWrite(motorPin4, bitRead(stepsLookup[step], 3));
}

const int stepsPerRevolution = 200;
AsyncStepper stepper1(stepsPerRevolution,
  []() {clockwise(); },
  []() {anticlockwise(); }
);

void rotateCW()
{
  stepper1.Rotate(90, AsyncStepper::CW, rotateCCW);
}

void rotateCCW()
{
  stepper1.Rotate(90, AsyncStepper::CCW, rotateCW);
}

void setup() {
  Serial_begin(9600);
  Serial_println("Woolwinder");

  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);

  pinMode(LED_BUILTIN, OUTPUT);

  
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
  stepper1.SetSpeedRpm(10);

  pinMode(PinCLK, INPUT);
  pinMode(PinDT, INPUT);
  pinMode(PinSW, INPUT_PULLUP);
  //attachInterrupt (1,isr,CHANGE);   // interrupt 1 is always connected to pin 3 on Arduino UNO
  bLastCLK = digitalRead(PinCLK);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3D (for the 128x64)
  //display.setFont(&FreeMono9pt7b);

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 1);
  display.print("Woolwinder");
  display.display();
  delay(1000);

  Timer1.initialize(100);  // 10 us = 100 kHz
  Timer1.attachInterrupt(stepperAdvance);
  Serial_println("init.done");
}

unsigned int cnt = 0;
int iRPM = 0;
unsigned long iStepCount = 0;

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
  cnt++;
  digitalWrite(LED_BUILTIN, ((cnt / 10) & 1) != 0);

  const int iMin = 30;
  //analogWrite(2, iMin + (long(iRPM) * (255 - iMin) / 1000));

  stepper1.SetSpeedRpm(iRPM);
  if(iRPM>0) {
    stepper1.RotateContinuos(AsyncStepper::CCW);
  } else {
    stepper1.Stop();
  }

  if (iRPM < 1) return;


#ifdef DBG
  //*
  unsigned int iNM = millis();
  static unsigned int iLastLog = 0;
  if (iNM > iLastLog + 500) {
    iLastLog = iNM;

    const long iRot = (iStepCount * GEAR_A) / (GEAR_B * SIGPR);
    //const int iMSTEPS=(long(STEPS_PERREV*32)*GEAR_B)/GEAR_A;

    Serial_print(iRPM);
    Serial_print(",");
    Serial_print(iStepCount);
    Serial_print(",");
    Serial_print(iRot);
    Serial_print("\n");
    cnt = 0;
  }/**/
#endif

  //Serial.println("loop");
}

unsigned long iLastStep = 0;
bool bStepOn = false;
unsigned int iClick = 0;
bool bSig = false;

void stepperAdvance() {
  unsigned long iNow = micros();
  if (iNow < iLastStep) {
    iLastStep = iNow;
  }; //overflow

  bool bCLK = digitalRead(PinCLK);
  if (bLastCLK != bCLK) {
    bLastCLK = bCLK;
    bool bDT = digitalRead(PinDT);
    if (bDT != bCLK) {
      iRPM += 20;
      if (iRPM > 1000) iRPM = 1000;
    } else {
      iRPM -= 20;
      if (iRPM < 0) iRPM = 0;
    }
  }

  if (!(digitalRead(PinSW))) {
    //Serial_print(iClick);
    iClick++;
    if (iClick > 7000) {
      iStepCount = 0;
    };
    iRPM = 0;
  } else {
    iClick = 0;
  }


  //digitalWrite(3, iRPM > 0);
  stepper1.Update();

}

void handleDisplay() {
  String rpm = String(iRPM);
  const long iRot = (iStepCount * GEAR_A) / (GEAR_B * SIGPR);
  //iRot=iRevCount;
  String cnt = String(iRot);

  int iW = (long(iRPM) * 126) / 1000;

  display.clearDisplay();
  display.writeFastHLine(0, 0, 128, 1);
  display.writeFastVLine(0, 0, 16, 1);
  display.writeFastHLine(0, 15, 128, 1);
  display.writeFastVLine(127, 0, 16, 1);

  display.writeFillRect(1, 1, iW, 14, 1);

  //display.setTextSize(2);
  //display.setTextColor(WHITE);
  //display.setCursor(0,1);
  //display.print("RPM: ");
  //display.println(rpm);


  display.setTextSize(4);
  display.setCursor(0, 28);
  display.println(cnt);
  display.display();
}
