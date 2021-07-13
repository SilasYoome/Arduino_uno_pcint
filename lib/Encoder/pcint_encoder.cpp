#include "pcint_encoder.h"

// Yes, all the code is in the header file, to provide the user
// configure options with #define (before they include it), and
// to facilitate some crafty optimizations!

 PCINT_Encoder_internal_state_t * PCINT_Encoder::interruptArgs[];


