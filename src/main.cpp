#include <Arduino.h>
#include "PinChangeInterrupt.h"
  
const byte encoderPinA = 14;//outputA digital pin2
const byte encoderPinB = 15;//outoutB digital pin3
volatile int count = 0;
int protectedCount = 0;
int previousCount = 0;

int flagA = 0;
int flagB = 0;
int total = 0;

#define readA digitalRead(2)
#define readB digitalRead(3)



void isrA();
void isrB();

void setup() {
  Serial.begin(9600);
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  // Attach the new PinChangeInterrupt and enable event function below
  attachPCINT(digitalPinToPCINT(encoderPinA), isrA, CHANGE);
  attachPCINT(digitalPinToPCINT(encoderPinB), isrB, CHANGE);
}

void isrA() {
  total++;
}
void isrB() {
  total++;
}

void loop() {
  Serial.println(total);
}
