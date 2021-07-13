/* Encoder Library, for measuring quadrature encoded signals
 * http://www.pjrc.com/teensy/td_libs_Encoder.html
 * Copyright (c) 2011,2013 PJRC.COM, LLC - Paul Stoffregen <paul@pjrc.com>
 *
 * Version 1.2 - fix -2 bug in C-only code
 * Version 1.1 - expand to support boards with up to 60 interrupts
 * Version 1.0 - initial release
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#endif

#if defined(__AVR__) || (defined(__arm__) && defined(CORE_TEENSY))   
#define IO_REG_TYPE			uint8_t
#define PIN_TO_BASEREG(pin)             (portInputRegister(digitalPinToPort(pin)))
#define PIN_TO_BITMASK(pin)             (digitalPinToBitMask(pin))
#define DIRECT_PIN_READ(base, mask)     (((*(base)) & (mask)) ? 1 : 0)
#endif

#define ENCODER_ARGLIST_SIZE CORE_NUM_INTERRUPT

#include <PinChangeInterrupt.h>
#include <PinChangeInterruptPins.h>
#include "pcint_config/pcint_pins.h"


// All the data needed by interrupts is consolidated into this ugly struct
// to facilitate assembly language optimizing of the speed critical update.
// The assembly code uses auto-incrementing addressing modes, so the struct
// must remain in exactly this order.
typedef struct {
	uint8_t					  pin1;
	uint8_t					  pin2;
	uint8_t                state;
	int32_t                position;
} PCINT_Encoder_internal_state_t;

class PCINT_Encoder
{
public:
	PCINT_Encoder(uint8_t pin1, uint8_t pin2) {
		#ifdef INPUT_PULLUP
		pinMode(pin1, INPUT_PULLUP);
		pinMode(pin2, INPUT_PULLUP);
		#else
		pinMode(pin1, INPUT);
		digitalWrite(pin1, HIGH);
		pinMode(pin2, INPUT);
		digitalWrite(pin2, HIGH);
		#endif
		interrupts_in_use = attach_pin_change_interrupt(pin1, &encoder);
		interrupts_in_use += attach_pin_change_interrupt(pin2, &encoder);

		//update_finishup();  // to force linker to include the code (does not work)
	}

	inline int32_t read() {
		update(&encoder);
		return encoder.position;
	}
	inline void write(int32_t p) {
		encoder.position = p;
	}

private:
	PCINT_Encoder_internal_state_t encoder;
	uint8_t interrupts_in_use;
public:
	static PCINT_Encoder_internal_state_t * interruptArgs[ENCODER_ARGLIST_SIZE];

//                           _______         _______       
//               Pin1 ______|       |_______|       |______ Pin1
// negative <---         _______         _______         __      --> positive
//               Pin2 __|       |_______|       |_______|   Pin2

		//	new	new	old	old
		//	pin2	pin1	pin2	pin1	Result
		//	----	----	----	----	------
		//	0	0	0	0	no movement
		//	0	0	0	1	+1
		//	0	0	1	0	-1
		//	0	0	1	1	+2  (assume pin1 edges only)
		//	0	1	0	0	-1
		//	0	1	0	1	no movement
		//	0	1	1	0	-2  (assume pin1 edges only)
		//	0	1	1	1	+1
		//	1	0	0	0	+1
		//	1	0	0	1	-2  (assume pin1 edges only)
		//	1	0	1	0	no movement
		//	1	0	1	1	-1
		//	1	1	0	0	+2  (assume pin1 edges only)
		//	1	1	0	1	-1
		//	1	1	1	0	+1
		//	1	1	1	1	no movement
/*
	// Simple, easy-to-read "documentation" version :-)
	//
	void update(void) {
		uint8_t s = state & 3;
		if (digitalRead(pin1)) s |= 4;
		if (digitalRead(pin2)) s |= 8;
		switch (s) {
			case 0: case 5: case 10: case 15:
				break;
			case 1: case 7: case 8: case 14:
				position++; break;
			case 2: case 4: case 11: case 13:
				position--; break;
			case 3: case 12:
				position += 2; break;
			default:
				position -= 2; break;
		}
		state = (s >> 2);
	}
*/

public:
	static void update(PCINT_Encoder_internal_state_t *arg) {
		uint8_t p1val = digitalRead(arg->pin1);
		uint8_t p2val = digitalRead(arg->pin2);
		uint8_t state = arg->state & 3;
		if (p1val) state |= 4;
		if (p2val) state |= 8;
		arg->state = (state >> 2);
		switch (state) {
			case 1: case 7: case 8: case 14:
				arg->position++;
				return;
			case 2: case 4: case 11: case 13:
				arg->position--;
				return;
			case 3: case 12:
				arg->position += 2;
				return;
			case 6: case 9:
				arg->position -= 2;
				return;
		}
	}
private:
	static uint8_t attach_pin_change_interrupt(uint8_t pin, PCINT_Encoder_internal_state_t *state) {
		switch (pin)
		{
			#ifdef PCINT_INT0_PIN
				case PCINT_INT0_PIN:
					interruptArgs[0] = state;
					attachPCINT(digitalPinToPCINT(pin),isr0,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT1_PIN
				case PCINT_INT1_PIN:
					interruptArgs[1] = state;
					attachPCINT(digitalPinToPCINT(pin),isr1,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT2_PIN
				case PCINT_INT2_PIN:
					interruptArgs[2] = state;
					attachPCINT(digitalPinToPCINT(pin),isr2,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT3_PIN
				case PCINT_INT3_PIN:
					interruptArgs[3] = state;
					attachPCINT(digitalPinToPCINT(pin),isr3,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT4_PIN
				case PCINT_INT4_PIN:
					interruptArgs[4] = state;
					attachPCINT(digitalPinToPCINT(pin),isr4,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT5_PIN
				case PCINT_INT5_PIN:
					interruptArgs[5] = state;
					attachPCINT(digitalPinToPCINT(pin),isr5,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT6_PIN
				case PCINT_INT6_PIN:
					interruptArgs[6] = state;
					attachPCINT(digitalPinToPCINT(pin),isr6,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT7_PIN
				case PCINT_INT7_PIN:
					interruptArgs[7] = state;
					attachPCINT(digitalPinToPCINT(pin),isr7,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT8_PIN
				case PCINT_INT8_PIN:
					interruptArgs[8] = state;
					attachPCINT(digitalPinToPCINT(pin),isr8,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT9_PIN
				case PCINT_INT9_PIN:
					interruptArgs[9] = state;
					attachPCINT(digitalPinToPCINT(pin),isr9,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT10_PIN
				case PCINT_INT10_PIN:
					interruptArgs[10] = state;
					attachPCINT(digitalPinToPCINT(pin),isr10,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT11_PIN
				case PCINT_INT11_PIN:
					interruptArgs[11] = state;
					attachPCINT(digitalPinToPCINT(pin),isr11,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT12_PIN
				case PCINT_INT12_PIN:
					interruptArgs[12] = state;
					attachPCINT(digitalPinToPCINT(pin),isr12,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT13_PIN
				case PCINT_INT13_PIN:
					interruptArgs[13] = state;
					attachPCINT(digitalPinToPCINT(pin),isr13,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT14_PIN
				case PCINT_INT14_PIN:
					interruptArgs[14] = state;
					attachPCINT(digitalPinToPCINT(pin),isr14,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT15_PIN
				case PCINT_INT15_PIN:
					interruptArgs[15] = state;
					attachPCINT(digitalPinToPCINT(pin),isr15,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT16_PIN
				case PCINT_INT16_PIN:
					interruptArgs[16] = state;
					attachPCINT(digitalPinToPCINT(pin),isr16,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT17_PIN
				case PCINT_INT17_PIN:
					interruptArgs[17] = state;
					attachPCINT(digitalPinToPCINT(pin),isr17,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT18_PIN
				case PCINT_INT18_PIN:
					interruptArgs[18] = state;
					attachPCINT(digitalPinToPCINT(pin),isr18,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT19_PIN
				case PCINT_INT19_PIN:
					interruptArgs[19] = state;
					attachPCINT(digitalPinToPCINT(pin),isr19,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT20_PIN
				case PCINT_INT20_PIN:
					interruptArgs[20] = state;
					attachPCINT(digitalPinToPCINT(pin),isr20,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT21_PIN
				case PCINT_INT21_PIN:
					interruptArgs[21] = state;
					attachPCINT(digitalPinToPCINT(pin),isr12,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT22_PIN
				case PCINT_INT22_PIN:
					interruptArgs[22] = state;
					attachPCINT(digitalPinToPCINT(pin),isr22,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT23_PIN
				case PCINT_INT23_PIN:
					interruptArgs[23] = state;
					attachPCINT(digitalPinToPCINT(pin),isr23,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT24_PIN
				case PCINT_INT24_PIN:
					interruptArgs[24] = state;
					attachPCINT(digitalPinToPCINT(pin),isr24,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT25_PIN
				case PCINT_INT25_PIN:
					interruptArgs[25] = state;
					attachPCINT(digitalPinToPCINT(pin),isr25,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT26_PIN
				case PCINT_INT26_PIN:
					interruptArgs[26] = state;
					attachPCINT(digitalPinToPCINT(pin),isr26,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT27_PIN
				case PCINT_INT27_PIN:
					interruptArgs[27] = state;
					attachPCINT(digitalPinToPCINT(pin),isr27,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT28_PIN
				case PCINT_INT28_PIN:
					interruptArgs[28] = state;
					attachPCINT(digitalPinToPCINT(pin),isr28,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT29_PIN
				case PCINT_INT29_PIN:
					interruptArgs[29] = state;
					attachPCINT(digitalPinToPCINT(pin),isr29,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT30_PIN
				case PCINT_INT30_PIN:
					interruptArgs[30] = state;
					attachPCINT(digitalPinToPCINT(pin),isr30,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT31_PIN
				case PCINT_INT31_PIN:
					interruptArgs[31] = state;
					attachPCINT(digitalPinToPCINT(pin),isr31,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT32_PIN
				case PCINT_INT32_PIN:
					interruptArgs[32] = state;
					attachPCINT(digitalPinToPCINT(pin),isr32,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT33_PIN
				case PCINT_INT33_PIN:
					interruptArgs[33] = state;
					attachPCINT(digitalPinToPCINT(pin),isr33,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT34_PIN
				case PCINT_INT34_PIN:
					interruptArgs[34] = state;
					attachPCINT(digitalPinToPCINT(pin),isr34,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT35_PIN
				case PCINT_INT35_PIN:
					interruptArgs[35] = state;
					attachPCINT(digitalPinToPCINT(pin),isr35,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT36_PIN
				case PCINT_INT36_PIN:
					interruptArgs[36] = state;
					attachPCINT(digitalPinToPCINT(pin),isr36,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT37_PIN
				case PCINT_INT37_PIN:
					interruptArgs[37] = state;
					attachPCINT(digitalPinToPCINT(pin),isr37,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT38_PIN
				case PCINT_INT38_PIN:
					interruptArgs[38] = state;
					attachPCINT(digitalPinToPCINT(pin),isr38,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT39_PIN
				case PCINT_INT39_PIN:
					interruptArgs[39] = state;
					attachPCINT(digitalPinToPCINT(pin),isr39,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT40_PIN
				case PCINT_INT40_PIN:
					interruptArgs[40] = state;
					attachPCINT(digitalPinToPCINT(pin),isr40,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT41_PIN
				case PCINT_INT41_PIN:
					interruptArgs[41] = state;
					attachPCINT(digitalPinToPCINT(pin),isr41,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT42_PIN
				case PCINT_INT42_PIN:
					interruptArgs[42] = state;
					attachPCINT(digitalPinToPCINT(pin),isr42,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT43_PIN
				case PCINT_INT43_PIN:
					interruptArgs[43] = state;
					attachPCINT(digitalPinToPCINT(pin),isr43,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT44_PIN
				case PCINT_INT44_PIN:
					interruptArgs[44] = state;
					attachPCINT(digitalPinToPCINT(pin),isr44,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT45_PIN
				case PCINT_INT45_PIN:
					interruptArgs[45] = state;
					attachPCINT(digitalPinToPCINT(pin),isr45,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT46_PIN
				case PCINT_INT46_PIN:
					interruptArgs[46] = state;
					attachPCINT(digitalPinToPCINT(pin),isr46,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT47_PIN
				case PCINT_INT47_PIN:
					interruptArgs[47] = state;
					attachPCINT(digitalPinToPCINT(pin),isr47,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT48_PIN
				case PCINT_INT48_PIN:
					interruptArgs[48] = state;
					attachPCINT(digitalPinToPCINT(pin),isr48,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT49_PIN
				case PCINT_INT49_PIN:
					interruptArgs[49] = state;
					attachPCINT(digitalPinToPCINT(pin),isr49,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT50_PIN
				case PCINT_INT50_PIN:
					interruptArgs[50] = state;
					attachPCINT(digitalPinToPCINT(pin),isr50,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT51_PIN
				case PCINT_INT51_PIN:
					interruptArgs[51] = state;
					attachPCINT(digitalPinToPCINT(pin),isr51,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT52_PIN
				case PCINT_INT52_PIN:
					interruptArgs[52] = state;
					attachPCINT(digitalPinToPCINT(pin),isr52,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT53_PIN
				case PCINT_INT53_PIN:
					interruptArgs[53] = state;
					attachPCINT(digitalPinToPCINT(pin),isr53,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT54_PIN
				case PCINT_INT54_PIN:
					interruptArgs[54] = state;
					attachPCINT(digitalPinToPCINT(pin),isr54,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT55_PIN
				case PCINT_INT55_PIN:
					interruptArgs[55] = state;
					attachPCINT(digitalPinToPCINT(pin),isr55,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT56_PIN
				case PCINT_INT56_PIN:
					interruptArgs[56] = state;
					attachPCINT(digitalPinToPCINT(pin),isr56,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT57_PIN
				case PCINT_INT57_PIN:
					interruptArgs[57] = state;
					attachPCINT(digitalPinToPCINT(pin),isr57,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT58_PIN
					case PCINT_INT58_PIN:
					interruptArgs[58] = state;
					attachPCINT(digitalPinToPCINT(pin),isr58,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT59_PIN
					case PCINT_INT59_PIN:
					interruptArgs[59] = state;
					attachPCINT(digitalPinToPCINT(pin),isr59,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT60_PIN
				case PCINT_INT60_PIN:
					interruptArgs[60] = state;
					attachPCINT(digitalPinToPCINT(pin),isr60,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT61_PIN
				case PCINT_INT61_PIN:
					interruptArgs[61] = state;
					attachPCINT(digitalPinToPCINT(pin),isr61,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT62_PIN
				case PCINT_INT62_PIN:
					interruptArgs[62] = state;
					attachPCINT(digitalPinToPCINT(pin),isr62,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT63_PIN
				case PCINT_INT63_PIN:
					interruptArgs[63] = state;
					attachPCINT(digitalPinToPCINT(pin),isr63,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT64_PIN
				case PCINT_INT64_PIN:
					interruptArgs[64] = state;
					attachPCINT(digitalPinToPCINT(pin),isr64,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT65_PIN
				case PCINT_INT65_PIN:
					interruptArgs[65] = state;
					attachPCINT(digitalPinToPCINT(pin),isr65,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT66_PIN
				case PCINT_INT66_PIN:
					interruptArgs[66] = state;
					attachPCINT(digitalPinToPCINT(pin),isr66,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT67_PIN
				case PCINT_INT67_PIN:
					interruptArgs[67] = state;
					attachPCINT(digitalPinToPCINT(pin),isr67,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT68_PIN
				case PCINT_INT68_PIN:
					interruptArgs[68] = state;
					attachPCINT(digitalPinToPCINT(pin),isr68,CHANGE);
					break;
			#endif
			#ifdef PCINT_INT69_PIN
				case PCINT_INT69_PIN:
					interruptArgs[69] = state;
					attachPCINT(digitalPinToPCINT(pin),isr69,CHANGE);
					break;
			#endif
		}
	}

	#ifdef PCINT_INT0_PIN
	static void isr0(void) { update(interruptArgs[0]); }
	#endif
	#ifdef PCINT_INT1_PIN
	static void isr1(void) { update(interruptArgs[1]); }
	#endif
	#ifdef PCINT_INT2_PIN
	static void isr2(void) { update(interruptArgs[2]); }
	#endif
	#ifdef PCINT_INT3_PIN
	static void isr3(void) { update(interruptArgs[3]); }
	#endif
	#ifdef PCINT_INT4_PIN
	static void isr4(void) { update(interruptArgs[4]); }
	#endif
	#ifdef PCINT_INT5_PIN
	static void isr5(void) { update(interruptArgs[5]); }
	#endif
	#ifdef PCINT_INT6_PIN
	static void isr6(void) { update(interruptArgs[6]); }
	#endif
	#ifdef PCINT_INT7_PIN
	static void isr7(void) { update(interruptArgs[7]); }
	#endif
	#ifdef PCINT_INT8_PIN
	static void isr8(void) { update(interruptArgs[8]); }
	#endif
	#ifdef PCINT_INT9_PIN
	static void isr9(void) { update(interruptArgs[9]); }
	#endif
	#ifdef PCINT_INT10_PIN
	static void isr10(void) { update(interruptArgs[10]); }
	#endif
	#ifdef PCINT_INT11_PIN
	static void isr11(void) { update(interruptArgs[11]); }
	#endif
	#ifdef PCINT_INT12_PIN
	static void isr12(void) { update(interruptArgs[12]); }
	#endif
	#ifdef PCINT_INT13_PIN
	static void isr13(void) { update(interruptArgs[13]); }
	#endif
	#ifdef PCINT_INT14_PIN
	static void isr14(void) { update(interruptArgs[14]); }
	#endif
	#ifdef PCINT_INT15_PIN
	static void isr15(void) { update(interruptArgs[15]); }
	#endif
	#ifdef PCINT_INT16_PIN
	static void isr16(void) { update(interruptArgs[16]); }
	#endif
	#ifdef PCINT_INT17_PIN
	static void isr17(void) { update(interruptArgs[17]); }
	#endif
	#ifdef PCINT_INT18_PIN
	static void isr18(void) { update(interruptArgs[18]); }
	#endif
	#ifdef PCINT_INT19_PIN
	static void isr19(void) { update(interruptArgs[19]); }
	#endif
	#ifdef PCINT_INT20_PIN
	static void isr20(void) { update(interruptArgs[20]); }
	#endif
	#ifdef PCINT_INT21_PIN
	static void isr21(void) { update(interruptArgs[21]); }
	#endif
	#ifdef PCINT_INT22_PIN
	static void isr22(void) { update(interruptArgs[22]); }
	#endif
	#ifdef PCINT_INT23_PIN
	static void isr23(void) { update(interruptArgs[23]); }
	#endif
	#ifdef PCINT_INT24_PIN
	static void isr24(void) { update(interruptArgs[24]); }
	#endif
	#ifdef PCINT_INT25_PIN
	static void isr25(void) { update(interruptArgs[25]); }
	#endif
	#ifdef PCINT_INT26_PIN
	static void isr26(void) { update(interruptArgs[26]); }
	#endif
	#ifdef PCINT_INT27_PIN
	static void isr27(void) { update(interruptArgs[27]); }
	#endif
	#ifdef PCINT_INT28_PIN
	static void isr28(void) { update(interruptArgs[28]); }
	#endif
	#ifdef PCINT_INT29_PIN
	static void isr29(void) { update(interruptArgs[29]); }
	#endif
	#ifdef PCINT_INT30_PIN
	static void isr30(void) { update(interruptArgs[30]); }
	#endif
	#ifdef PCINT_INT31_PIN
	static void isr31(void) { update(interruptArgs[31]); }
	#endif
	#ifdef PCINT_INT32_PIN
	static void isr32(void) { update(interruptArgs[32]); }
	#endif
	#ifdef PCINT_INT33_PIN
	static void isr33(void) { update(interruptArgs[33]); }
	#endif
	#ifdef PCINT_INT34_PIN
	static void isr34(void) { update(interruptArgs[34]); }
	#endif
	#ifdef PCINT_INT35_PIN
	static void isr35(void) { update(interruptArgs[35]); }
	#endif
	#ifdef PCINT_INT36_PIN
	static void isr36(void) { update(interruptArgs[36]); }
	#endif
	#ifdef PCINT_INT37_PIN
	static void isr37(void) { update(interruptArgs[37]); }
	#endif
	#ifdef PCINT_INT38_PIN
	static void isr38(void) { update(interruptArgs[38]); }
	#endif
	#ifdef PCINT_INT39_PIN
	static void isr39(void) { update(interruptArgs[39]); }
	#endif
	#ifdef PCINT_INT40_PIN
	static void isr40(void) { update(interruptArgs[40]); }
	#endif
	#ifdef PCINT_INT41_PIN
	static void isr41(void) { update(interruptArgs[41]); }
	#endif
	#ifdef PCINT_INT42_PIN
	static void isr42(void) { update(interruptArgs[42]); }
	#endif
	#ifdef PCINT_INT43_PIN
	static void isr43(void) { update(interruptArgs[43]); }
	#endif
	#ifdef PCINT_INT44_PIN
	static void isr44(void) { update(interruptArgs[44]); }
	#endif
	#ifdef PCINT_INT45_PIN
	static void isr45(void) { update(interruptArgs[45]); }
	#endif
	#ifdef PCINT_INT46_PIN
	static void isr46(void) { update(interruptArgs[46]); }
	#endif
	#ifdef PCINT_INT47_PIN
	static void isr47(void) { update(interruptArgs[47]); }
	#endif
	#ifdef PCINT_INT48_PIN
	static void isr48(void) { update(interruptArgs[48]); }
	#endif
	#ifdef PCINT_INT49_PIN
	static void isr49(void) { update(interruptArgs[49]); }
	#endif
	#ifdef PCINT_INT50_PIN
	static void isr50(void) { update(interruptArgs[50]); }
	#endif
	#ifdef PCINT_INT51_PIN
	static void isr51(void) { update(interruptArgs[51]); }
	#endif
	#ifdef PCINT_INT52_PIN
	static void isr52(void) { update(interruptArgs[52]); }
	#endif
	#ifdef PCINT_INT53_PIN
	static void isr53(void) { update(interruptArgs[53]); }
	#endif
	#ifdef PCINT_INT54_PIN
	static void isr54(void) { update(interruptArgs[54]); }
	#endif
	#ifdef PCINT_INT55_PIN
	static void isr55(void) { update(interruptArgs[55]); }
	#endif
	#ifdef PCINT_INT56_PIN
	static void isr56(void) { update(interruptArgs[56]); }
	#endif
	#ifdef PCINT_INT57_PIN
	static void isr57(void) { update(interruptArgs[57]); }
	#endif
	#ifdef PCINT_INT58_PIN
	static void isr58(void) { update(interruptArgs[58]); }
	#endif
	#ifdef PCINT_INT59_PIN
	static void isr59(void) { update(interruptArgs[59]); }
	#endif
	#ifdef PCINT_INT60_PIN
	static void isr60(void) { update(interruptArgs[60]); }
	#endif
	#ifdef PCINT_INT61_PIN
	static void isr62(void) { update(interruptArgs[62]); }
	#endif
	#ifdef PCINT_INT63_PIN
	static void isr63(void) { update(interruptArgs[63]); }
	#endif
	#ifdef PCINT_INT64_PIN
	static void isr64(void) { update(interruptArgs[64]); }
	#endif
	#ifdef PCINT_INT65_PIN
	static void isr65(void) { update(interruptArgs[65]); }
	#endif
	#ifdef PCINT_INT66_PIN
	static void isr66(void) { update(interruptArgs[66]); }
	#endif
	#ifdef PCINT_INT67_PIN
	static void isr67(void) { update(interruptArgs[67]); }
	#endif
	#ifdef PCINT_INT68_PIN
	static void isr68(void) { update(interruptArgs[68]); }
	#endif
	#ifdef PCINT_INT69_PIN
	static void isr69(void) { update(interruptArgs[69]); }
	#endif
};