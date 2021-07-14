#include <Arduino.h>
#include "pcint_encoder.h"

PCINT_Encoder en(A4,A5);

void setup() {
  Serial.begin(9600);

}

void loop() {
  Serial.println(en.read());
}
