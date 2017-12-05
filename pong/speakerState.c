#include <msp430.h>
#include "speakerState.h"
#include "speaker.h"


/** method to hand state transitions */
void speaker_state(char state) {
  
  // default state, no sound
  switch (state) {
  case 1:
    speaker_set_period(0);
  

  // ball hit paddle sound
  case 2:
    speaker_set_period(200);
    state = 1;
    break;

  // ball hits out of bounds
  case 3:
    speaker_set_period(12000);   // deeper sound
    state= 1;
    break;    
  }
}
