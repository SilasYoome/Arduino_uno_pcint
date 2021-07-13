#pragma once

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega8__)
   #define CORE_NUM_INTERRUPT 20
   #define PCINT_INT0_PIN 0
   #define PCINT_INT1_PIN 1
   #define PCINT_INT2_PIN 2
   #define PCINT_INT3_PIN 3
   #define PCINT_INT4_PIN 4
   #define PCINT_INT5_PIN 5
   #define PCINT_INT6_PIN 6
   #define PCINT_INT7_PIN 7
   #define PCINT_INT8_PIN 8
   #define PCINT_INT9_PIN 9
   #define PCINT_INT10_PIN 10
   #define PCINT_INT11_PIN 11
   #define PCINT_INT12_PIN 12
   #define PCINT_INT13_PIN 13
   #define PCINT_INT14_PIN 14
   #define PCINT_INT15_PIN 15
   #define PCINT_INT16_PIN 16
   #define PCINT_INT17_PIN 17
   #define PCINT_INT18_PIN 18
   #define PCINT_INT19_PIN 19

#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
   #define CORE_NUM_INTERRUPT 16
   #define PCINT_INT10_PIN 10
   #define PCINT_INT11_PIN 11
   #define PCINT_INT12_PIN 12
   #define PCINT_INT13_PIN 13
   #define PCINT_INT50_PIN 50
   #define PCINT_INT51_PIN 51
   #define PCINT_INT52_PIN 52
   #define PCINT_INT53_PIN 53
   #define PCINT_INT62_PIN 62
   #define PCINT_INT63_PIN 63
   #define PCINT_INT64_PIN 64
   #define PCINT_INT65_PIN 65
   #define PCINT_INT66_PIN 66
   #define PCINT_INT67_PIN 67
   #define PCINT_INT68_PIN 68
   #define PCINT_INT69_PIN 69
#endif