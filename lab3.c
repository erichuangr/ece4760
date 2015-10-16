////////////////////////////////////
#include "config.h"
#include "pt_cornell_1_2.h"
#include "tft_master.h"
#include "tft_gfx.h"
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
////////////////////////////////////
char buffer[60];
int pt_start_time, pt_end_time, sys_time_seconds ;
static struct pt pt_timer, pt_adc, pt_launch, pt_anim ;

// === the fixed point macros ========================================
typedef signed int fix16 ;
#define multfix16(a,b)  ((fix16)(((( signed long long)(a))*(( signed long long)(b)))>>16)) //multiply two fixed 16:16
#define float2fix16(a)  ((fix16)((a)*65536.0)) // 2^16
#define fix2float16(a)  ((float)(a)/65536.0)
#define fix2int16(a)    ((int)((a)>>16))
#define int2fix16(a)    ((fix16)((a)<<16))
#define divfix16(a,b)   ((fix16)((((signed long long)(a)<<16)/(b)))) 
#define sqrtfix16(a)    (float2fix16(sqrt(fix2float16(a)))) 
#define absfix16(a)     abs(a)
#define RADIUS      2
#define DIAMETER    4
#define COUNTER     10
#define MAX_BALLS   30
#define PADDLE_LEN  30

// === Ball Structure ===============================================
typedef struct ball{
	fix16 xc;
	fix16 yc;	
	fix16 vxc;
	fix16 vyc;		
	fix16 hit_counter;
	struct ball *prev;
	struct ball *next;
}ball;
typedef struct paddle{
    fix16 xc;
	fix16 yc;	
	fix16 vyc;	
    fix16 vyi;  //initial velocity
}paddle;

int balls_collide         (ball*,ball*);
void computeVelocityChange(ball*,ball*);
void deleteBall           (ball*);
// global properties for ball and paddle
static paddle *myPaddle     = NULL;
static ball *initial_ball   = NULL;
static ball *end_ball       = NULL;
static ball *iterator       = NULL;
static ball *iterator2      = NULL;

fix16 current_balls     = 0;
fix16 mag_r_ij          = int2fix16(2*RADIUS);
fix16 mag_r_ij_squared  = float2fix16(pow(2*RADIUS,2.0));
static pdrag            = float2fix16(.2);  //drag of paddle when interacting with ball
static drag             = float2fix16(.95); //drag ball
volatile int score      = 0;
volatile int gameover   = 0;
volatile unsigned char ball_scored[256];
volatile signed int sine_table2[256];
int	dmaChn=0;		// the DMA channel to use

//compute radius and see if 2 target balls collide. if they do, return 1.
int balls_collide(ball *ball_i, ball *ball_j){
	//calculate distance between two balls.
	fix16 r_x_ij = (ball_i->xc) - (ball_j->xc);
	fix16 r_y_ij = (ball_i->yc) - (ball_j->yc);       
	//check if both components are within collision.
	if ((abs(r_x_ij) < mag_r_ij) && (abs(r_y_ij) < mag_r_ij))   return 1;
	else    return 0;	
}//function balls_collide
//method to calculate velocity change when two balls collide (less than 2 radii apart)
void computeVelocityChange(ball *ball_i, ball *ball_j){
	//get connecting position line in x and y
	fix16 r_x_ij = (ball_i->xc) - (ball_j->xc);
	fix16 r_y_ij = (ball_i->yc) - (ball_j->yc);
	//get connecting velocity line in x and y
	fix16 v_x_ij = (ball_i->vxc) - (ball_j->vxc);
	fix16 v_y_ij = (ball_i->vyc) - (ball_j->vyc);
    if(r_x_ij == 0) r_x_ij = int2fix16(1);
    if(r_y_ij == 0) r_y_ij = int2fix16(1);
    if(v_x_ij == 0) v_x_ij = int2fix16(1);
    if(v_y_ij == 0) v_y_ij = int2fix16(1);
	//compute delta v using equation in lab handout
	fix16 delta_vel_rv = multfix16(r_x_ij, v_x_ij) + multfix16(r_y_ij, v_y_ij);
	fix16 delta_vel_neg_rrv_pos_x = multfix16(-r_x_ij, divfix16(delta_vel_rv, mag_r_ij_squared));
	fix16 delta_vel_neg_rrv_pos_y = multfix16(-r_y_ij, divfix16(delta_vel_rv, mag_r_ij_squared));	
	//update the balls' velocities
	ball_i->vxc = (ball_i->vxc) + (delta_vel_neg_rrv_pos_x);
	ball_i->vyc = (ball_i->vyc) + (delta_vel_neg_rrv_pos_y);
	ball_j->vxc = (ball_j->vxc) - (delta_vel_neg_rrv_pos_x);
	ball_j->vyc = (ball_j->vyc) - (delta_vel_neg_rrv_pos_y);   
	//set hit counters to not collide again
	ball_i->hit_counter = COUNTER;
	ball_j->hit_counter = COUNTER;	
}//function computeVelocityChange
//method to remove a ball from the linked list and attach
void deleteBall(ball *delete_this){	
	if(iterator->next != NULL)
        iterator = iterator->next;	
    if(current_balls == 1) {
        initial_ball = NULL;
        end_ball = NULL;
    }
	//if we need to delete initial ball
	else if (initial_ball == delete_this){
		initial_ball = delete_this -> next;	
        initial_ball->prev = NULL;
	}
	//if we need to delete end ball
	else if (end_ball == delete_this){
		end_ball = delete_this -> prev;
		end_ball->next = NULL;
	}
	//if it's a middle ball
	else{
		(delete_this->next)->prev = delete_this->prev;
		(delete_this->prev)->next = delete_this->next;
	}
	tft_fillCircle(fix2int16(delete_this->xc), fix2int16(delete_this->yc), RADIUS, ILI9340_BLACK);
	free(delete_this);
    delete_this = NULL;
    current_balls--;
	
}//function deleteBall
 //method to figure out what sound to play
// === Timer Thread =================================================
static PT_THREAD (protothread_timer(struct pt *pt))
{
    PT_BEGIN(pt);
    tft_setCursor(100, 0);
    tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
    tft_writeString("Elapsed time : \t Score");
    while(1) {
        PT_YIELD_TIME_msec(1000) ;
        sys_time_seconds++ ;
        tft_fillRect(100, 10, 150, 8, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(100, 10);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(1);
        sprintf(buffer,"%*d %*d", 7, sys_time_seconds, 12, score);
        tft_writeString(buffer);
        tft_fillRect(175, 18,   2, 7, ILI9340_CYAN);
        tft_fillRect(248, 18,   2, 7, ILI9340_CYAN);
        tft_fillRect(175, 18,  75, 2, ILI9340_CYAN);
        tft_fillRect(175, 233,  2, 7, ILI9340_CYAN);
        tft_fillRect(248, 233,  2, 7, ILI9340_CYAN);
        tft_fillRect(175, 238, 75, 2, ILI9340_CYAN);

        if(score >= 50 && sys_time_seconds <= 60)  {
            tft_setCursor(100, 90);
            tft_setTextColor(ILI9340_GREEN);  tft_setTextSize(3);
            tft_writeString("You Win!");
        }
        else if(score <= -50 || sys_time_seconds > 60) {
            tft_setCursor(100, 90);
            tft_setTextColor(ILI9340_RED);  tft_setTextSize(3);
            tft_writeString("You Lose!");
        }
    } 
  PT_END(pt);
} // timer thread
// === ADC Thread =============================================
static PT_THREAD (protothread_adc(struct pt *pt))
{
    PT_BEGIN(pt);
    myPaddle = (paddle*) malloc(sizeof(paddle));	
    myPaddle->xc = 4;
    static int adc_10;
    while(1) {      
        PT_YIELD_TIME_msec(2); 
        // read the ADC from pin 24 (AN11)
        adc_10 = ReadADC10(0);   // read the result of channel 9 conversion from the idle buffer
        AcquireADC10(); // not needed if ADC_AUTO_SAMPLING_ON below      
        //READADC10 value is between 0-1020
        tft_fillRect(myPaddle->xc, myPaddle->yc, 2, PADDLE_LEN, ILI9340_BLACK);
        if(adc_10 < 3) adc_10 = 0;
        else if (adc_10 > 920) adc_10 = 920;
        myPaddle->vyc = (adc_10 >> 2) - myPaddle->yc;
        myPaddle->yc = (adc_10 >> 2);
        tft_fillRect(myPaddle->xc, myPaddle->yc, 2, PADDLE_LEN, ILI9340_WHITE);
    } // END WHILE(1)
  PT_END(pt);
} // ADC thread
// === Launching balls =================================================
static PT_THREAD (protothread_launch_balls(struct pt *pt))
{
  PT_BEGIN(pt);
  while(1){
	 //make sure we don't put in too many balls
	PT_WAIT_UNTIL(pt, current_balls < MAX_BALLS);
    while(current_balls < MAX_BALLS)    {
        PT_YIELD_TIME_msec(200); //5 balls per second
		current_balls++; //increase ball count in field				
		ball *new_ball = (ball*) malloc (sizeof(ball));	
		new_ball->xc   = int2fix16(315);
		new_ball->yc   = int2fix16(120);
        new_ball->vxc  = int2fix16(-8 + (rand()>>28)); 
        new_ball->vyc  = int2fix16(-8 + (rand()>>27));  
		new_ball->next = NULL;
        new_ball->hit_counter = 0;
        
		if (initial_ball == NULL) { //if chain is empty, this ball is first ball
			initial_ball = new_ball;
            new_ball->prev = NULL;
        }
        else {//otherwise, put it at end
			end_ball->next = new_ball;
            new_ball->prev = end_ball;
        }
        end_ball = new_ball;
    }
  }
  PT_END(pt);
} // end launch ball thread
// === Animation Thread =============================================
static PT_THREAD (protothread_anim_balls(struct pt *pt))
{
    PT_BEGIN(pt); 
    while(1) {    
        PT_YIELD_TIME_msec(20);
        iterator = initial_ball;
        while(iterator != NULL) {       
            iterator2 = iterator->next;
            tft_fillCircle(fix2int16(iterator->xc), fix2int16(iterator->yc), RADIUS, ILI9340_BLACK);
            (iterator->xc) = (iterator->xc) + multfix16((iterator->vxc), drag);
            (iterator->yc) = (iterator->yc) + multfix16((iterator->vyc), drag);
            tft_fillCircle(fix2int16(iterator->xc), fix2int16(iterator->yc), RADIUS, ILI9340_WHITE);
            while(iterator2 != NULL)    {
                if (balls_collide(iterator, iterator2) && (iterator->hit_counter <= 0) && (iterator2->hit_counter <= 0))
                    computeVelocityChange(iterator, iterator2);
                else if (balls_collide(iterator, iterator2))
                {
                    iterator->hit_counter = COUNTER;
                    iterator2->hit_counter = COUNTER;
                }
                else{
                    iterator->hit_counter--;
					iterator2->hit_counter--;
                }
                iterator2 = iterator2->next;
            }
            // If the ball hits the paddle
            if ((myPaddle->xc+DIAMETER >= fix2int16(iterator->xc)) && (myPaddle->yc <= fix2int16(iterator->yc)) 
                    && ((myPaddle->yc+PADDLE_LEN) >= fix2int16(iterator->yc))){
                iterator->vxc = -(iterator->vxc);
                iterator->vyc = iterator->vyc + multfix16(int2fix16(myPaddle->vyc),pdrag);
                //DmaChnSetTxfer(dmaChn, sine_table2, (void*)&CVRCON, sizeof(sine_table2), 1, 1);
                //DmaChnEnable(dmaChn);
                //PT_YIELD_TIME_msec(2);
                //DmaChnDisable(dmaChn);  
            }
            // Else the ball went off the right edge
            else if (myPaddle->xc+DIAMETER >= fix2int16(iterator->xc))                                                    
            { 
                DmaChnSetTxfer(dmaChn, ball_scored, (void*)&CVRCON, sizeof(ball_scored), 1, 1);
                DmaChnEnable(dmaChn);
                PT_YIELD_TIME_msec(2);
                DmaChnDisable(dmaChn);  
                deleteBall(iterator); 
                score--;
            }
            // Calculate top bin
            else if ((iterator->xc) <= int2fix16(250+2) && iterator->xc >= int2fix16(175-2) && iterator->yc <= int2fix16(25+9))    
                { deleteBall(iterator); score++;}   
            // Calculate bottom bin
            else if ((iterator->xc) <= int2fix16(250+2) && iterator->xc >= int2fix16(175-2) && iterator->yc >= int2fix16(233-2))   
                { deleteBall(iterator); score++;}
            // Bounce off right wall
            else if ((iterator->xc) >= int2fix16(315))                                      
                (iterator->vxc) = -(iterator->vxc); 
            // Bounce off top or bottom wall
            else if ((iterator->yc) <= int2fix16(25) ||(iterator->yc) >= int2fix16(233))    
                (iterator->vyc) = -(iterator->vyc);         
            iterator = iterator->next;   
        }
    } // END WHILE(1)
    PT_END(pt);
} // animation thread

// === Main  ======================================================
void main(void) {
    //SYSTEMConfigPerformance(PBCLK);
    ANSELA = 0; ANSELB = 0; 
    // === config threads ==========
    // turns OFF UART support and debugger pin, unless defines are set
    PT_setup();
    // === setup system wide interrupts  ========
    INTEnableSystemMultiVectoredInt();
    CloseADC10();	// ensure the ADC is off before setting the configuration
    #define PARAM1  ADC_FORMAT_INTG16 | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_OFF //
    // define setup parameters for OpenADC10
    // ADC ref external  | disable offset test | disable scan mode | do 1 sample | use single buf | alternate mode off
    #define PARAM2  ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_OFF | ADC_SAMPLES_PER_INT_1 | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_OFF
    // Define setup parameters for OpenADC10
    // use peripherial bus clock | set sample time | set ADC clock divider
    // ADC_CONV_CLK_Tcy2 means divide CLK_PB by 2 (max speed)
    // ADC_SAMPLE_TIME_5 seems to work with a source resistance < 1kohm
    #define PARAM3 ADC_CONV_CLK_PB | ADC_SAMPLE_TIME_5 | ADC_CONV_CLK_Tcy2 //ADC_SAMPLE_TIME_15| ADC_CONV_CLK_Tcy2
    // define setup parameters for OpenADC10
    // set AN11 and  as analog inputs
    #define PARAM4	ENABLE_AN11_ANA // pin 24
    // define setup parameters for OpenADC10
    // do not assign channels to scan
    #define PARAM5	SKIP_SCAN_ALL
    // use ground as neg ref for A | use AN11 for input A     
    // configure to sample AN11 
    SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN11 ); // configure to sample AN4 
    OpenADC10( PARAM1, PARAM2, PARAM3, PARAM4, PARAM5 ); // configure ADC using the parameters defined above
    EnableADC10(); // Enable the ADC      
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, 400); 
    mPORTAClearBits(BIT_0 );		//Clear bits to ensure light is off.
    mPORTASetPinsDigitalOut(BIT_0 );    //Set port as output
    int CVRCON_setup;
    // set up the Vref pin and use as a DAC
    // enable module| eanble output | use low range output | use internal reference | desired step
    CVREFOpen( CVREF_ENABLE | CVREF_OUTPUT_ENABLE | CVREF_RANGE_LOW | CVREF_SOURCE_AVDD | CVREF_STEP_0 );
    // And read back setup from CVRCON for speed later
    // 0x8060 is enabled with output enabled, Vdd ref, and 0-0.6(Vdd) range
    CVRCON_setup = CVRCON; //CVRCON = 0x8060
	// Open the desired DMA channel.
	// We enable the AUTO option, we'll keep repeating the same transfer over and over.
	DmaChnOpen(dmaChn, 0, DMA_OPEN_AUTO);
	// set the transfer parameters: source & destination address, source & destination size, number of bytes per event
    // Setting the last parameter to one makes the DMA output one byte/interrut
    //DmaChnSetTxfer(dmaChn, sine_table, (void*) &CVRCON, sizeof(sine_table), 1, 1);
	// set the transfer event control: what event is to start the DMA transfer
    // In this case, timer2 
	DmaChnSetEventControl(dmaChn,  DMA_EV_START_IRQ(_TIMER_2_IRQ));
	// once we configured the DMA channel we can enable it
	// now it's ready and waiting for an event to occur...
	//DmaChnEnable(dmaChn);
  
    int i;
    for(i=0; i <256; i++){
        ball_scored[i] = 0x60|((unsigned char)((float)255/2*(sin(6.2832*(((float)i)/(float)256))+1))) ;
        //ball_lost[i]
       // game_over
    }

        
    // init the threads
    PT_INIT(&pt_timer);
    PT_INIT(&pt_adc);
    PT_INIT(&pt_launch);
    PT_INIT(&pt_anim);
    // init the display
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK);
    tft_setRotation(1); // Use tft_setRotation(1) for 320x240
    while (1){
        PT_SCHEDULE(protothread_timer(&pt_timer));
        PT_SCHEDULE(protothread_adc(&pt_adc));
        PT_SCHEDULE(protothread_launch_balls(&pt_launch));
        PT_SCHEDULE(protothread_anim_balls(&pt_anim));
    }
  } // main
// === end  ======================================================
