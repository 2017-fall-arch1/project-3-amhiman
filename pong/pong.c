
#include <msp430.h>
#include <stdlib.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "speakerState.h"
#include "speaker.h"

#define GREEN_LED BIT6

// paddles
AbRect pad1 = {abRectGetBounds, abRectCheck, {1,15}}; // 1x10 rect paddle 1
AbRect pad2 = {abRectGetBounds, abRectCheck, {1,15}}; // 1x10 rect paddle 2

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 2, screenHeight/2 - 10}
};
  

Layer puckLayer3 = {	    /**< Layer with an blue circle(puck) */
  (AbShape *)&circle4,
  {(screenWidth/2), (screenHeight/2)}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLUE,
  0
};

Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  &puckLayer3
};

Layer padLayer2 = {               /**< Layer with a black square (paddle2) */
  (AbShape *)&pad2,
  {screenWidth-5, screenHeight/2}, /**< middle 5 from right */
  {0,0}, {0,0},                              /* last & next pos */
  COLOR_BLACK,
  &fieldLayer
};

Layer padLayer1 = {		/**< Layer with a black square (paddle1) */
  (AbShape *)&pad1,
  {5, screenHeight/2}, /**< middle 5 from left */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  &padLayer2
};


/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml3 = { &puckLayer3, {2,2}, 0 }; /**< not all layers move (puck)*/  
MovLayer mlPad1 = { &padLayer1, {0,2}, 0};
MovLayer mlPad2 = { &padLayer2, {0,2}, 0};

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  



//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

char score= 0; 
char scoreString[5];    /**< conver char to a "string" */


/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence, Region *pad1Fence, Region *pad2Fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  char hitPaddle = 0;             // boolean if hit paddle or not

  speaker_init();  // no noise
  
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    
    for (axis = 0; axis < 2; axis ++) {
      
      // if outside of fence
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }

      // if puck collides with pad1 
      if ((shapeBoundary.topLeft.axes[0] < pad1Fence->botRight.axes[0]) &&               (shapeBoundary.botRight.axes[1] > pad1Fence->topLeft.axes[1]) &&               (shapeBoundary.topLeft.axes[1] < pad1Fence->botRight.axes[1])) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
	hitPaddle = 1;  //hitPaddle == true
	speaker_state(2);   // hit paddle noise
	break;
      }

      // if puck collides with pad2
      if ((shapeBoundary.botRight.axes[0] > pad2Fence->topLeft.axes[0]) &&               (shapeBoundary.botRight.axes[1] > pad2Fence->topLeft.axes[1]) &&               (shapeBoundary.topLeft.axes[1] < pad2Fence->botRight.axes[1])) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
	hitPaddle = 1;  //hitPaddle == true
	speaker_state(2);   // hit paddle noise
	break;
      }
      
      // if out of bounds
      if ((shapeBoundary.topLeft.axes[0] <= fence->topLeft.axes[0]) || (shapeBoundary.botRight.axes[0] >= fence->botRight.axes[0])) {
        score = 0; // reset score
	speaker_state(3);   // out of bounds noise
	Vec2 temp = {screenWidth/2,screenHeight/2};   // reset to center
	newPos = temp;
      }
	
	
    } /**< for axis */

    if (hitPaddle)  // inc score if a paddle
      score++;
    
    ml->layer->posNext = newPos;

  } /**< for ml */

}





u_int bgColor = COLOR_GREEN;     /**< The background color */
int redrawScreen = 1;           /**< Boolean whether screen need redrawn */

Region fieldFence;		/**< fence around playing field  */
Region pad1Fence;               /**< fence around paddle 1*/
Region pad2Fence;               /**< fence around paddle 2*/


/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);
  speaker_init();

  shapeInit();

  
  layerInit(&padLayer1);
  layerDraw(&padLayer1);


  // bounds of layers
  layerGetBounds(&fieldLayer, &fieldFence);
  layerGetBounds(&padLayer1, &pad1Fence);
  layerGetBounds(&padLayer2, &pad2Fence);

  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */


  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml3, &padLayer1);

    // displaying score
    drawString5x7(25,2,"Score:", COLOR_BLACK, COLOR_GREEN);
    itoa(score, scoreString, 10); /**< convert to "string" */
    drawString5x7(100,2,scoreString, COLOR_BLACK, COLOR_GREEN);

  }
}





void movePaddle(char paddle, char dir)
{
  if (paddle == 'L') {
    if ((dir == 'U') && (mlPad1.layer->posNext.axes[1]-3 >= 25))
      mlPad1.layer->posNext.axes[1]-=3;
    else if ((dir == 'D') && (mlPad1.layer->posNext.axes[1]+3 <= screenHeight -25))
      mlPad1.layer->posNext.axes[1]+=3;
      
    layerGetBounds(&padLayer1, &pad1Fence);
    movLayerDraw(&mlPad1, &padLayer1);
  }
  else {
    if ((dir == 'U') && (mlPad2.layer->posNext.axes[1]-3 >= 25))
      mlPad2.layer->posNext.axes[1]-=3;
    else if ((dir =='D') && (mlPad2.layer->posNext.axes[1]+3 <= screenHeight -25))
      mlPad2.layer->posNext.axes[1]+=3;

    layerGetBounds(&padLayer2, &pad2Fence);
    movLayerDraw(&mlPad2, &padLayer2);
  }
}



/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */

  count ++;
  if (count == 15) {
    mlAdvance(&ml3, &fieldFence, &pad1Fence, &pad2Fence);
    redrawScreen = 1;
    
    // switch one (move left paddle up)
    if (~p2sw_read() & 0x01) {
      movePaddle('L','U');
    }

    // switch two (move left paddle down)
    if (~p2sw_read() & 0x02) {
      movePaddle('L','D');   
    }

    // switch three (move right paddle up)
    if (~p2sw_read() & 0x04) {
      movePaddle('R','U');
    }

    // switch four (move right paddle down)
    if (~p2sw_read() & 0x08) {
      movePaddle('R','D');
    }

    count = 0;
  } 
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}


