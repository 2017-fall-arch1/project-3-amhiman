#include <msp430.h>
#include "speakerState.h"
#include "speaker.h"


/** method to hand state transitions */
void speaker_state(char state) {
  
  // switch over called state
  switch (state) {
  

  // ball hit paddle sound
  case 1:
    speaker_set_period(200);
    break;

  // ball hits out of bounds
  case 2:
    speaker_set_period(12000);   // deeper sound
    break;    
  }
}
