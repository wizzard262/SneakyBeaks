
#include <wgt45.h>

#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <dos.h>
#include <bios.h>
#include <fcntl.h>
#include <memory.h>
#include <math.h>
#include <string.h>

#include "graph3.h"  // include our graphics stuff. Are 3,4,6 removeable?
#include "graph4.h"
#include "graph6.h"


//DEFINES ///////////////////////////////////

#define beak_width       20   // bitmap width
#define beak_height      30   // bitmap height

#define skittle_width     5   // bitmap width
#define skittle_height    5   // bitmap height

#define infinity      60000   // impossibly high number
#define gravity           9   // + = downward force
#define lost              1   // lost skittle/money
#define kept              0   // kept skittle/money
#define num_beaks        20   // maximum no of beaks
#define beak_speed       10   // beaks mean speed
#define skittle_max      30   // maximum skittle time
#define bag_max          20   // maximum bag contents
#define time_max         10   // maximum beak lifetime, without skittle

#define move_time_max     2   // maximum time before change in movement
#define max_num_skittle  30   // maximum number of skittles on screen

#define dying_time        1   // constant time for dying_sequence
#define pecking_time     30   // constant time for pecking sequence
#define kicking_time     30   // constant time for kicking + kicked sequences
#define walking_time      5   // constant time for walking sequence

// Direction of beak

#define beak_n      1
#define beak_e      2
#define beak_s      3
#define beak_w      4
#define beak_0      0         // still

// State of beak

#define noskittle    1       // beak has no skittle
#define skittle      2       // beak has skittle
#define money        3       // beak holds money
#define bag          4       // beak holds bag
#define pecking      5       // beak is pecking up skittle
#define kicking      6       // beak is kicking other beak
#define kicked_ns    7       // beak is being kicked (no skittle)
#define kicked_s     8       // beak is being kicked (skittle)
#define kicked_b     9       // beak is being kicked (bag)
#define kicked_m    10       // beak is being kicked (money)
#define dying       11     // beak is dying
#define dead        12     // beak is dead


// movements of beak

#define none         0       // for when set sequence is running, this has no move_state
#define random_1     1       // beak moves randomly
#define track_m      2       // beak tracks closest money
#define track_s      3       // beak tracks closest skittle
#define track_shop   4       // beak tracks shop
#define track_b      5       // beak tracks closest beak
#define evade        6       // beak evades closest beak

#define peck_money   7       // pecking substate, peck_money
#define peck_skittle 8       // pecking substate pecking_skittle

// state of skittles

#define skittle_dead  1      // empty array position
#define skittle_alive 2      // on screen

// skittle colors

#define red         1
#define orange      2
#define yellow      3
#define green       4
#define purple      5
#define grey        6     // special case = money

// shop coordinates

#define shop_x     180     // plots into center of 320 *200 screen
#define shop_y     100

///////////////// PROTOTYPES //////////////

void make_background(void);
void Init_Beaks(void);
void Init_Skittles(void);
/////////////////////////////////////////////////////////////////
block scr, scr2;           // 1st & double buffer
block sprites[21];         // no of sprites{i.e. all animation frames} in game

typedef struct beak_typ               //beak datastructure
		  {
			int dir;              // direction of beak
			int state;            // state of beak
			int move_state;       // movement_state of beak
			int move_count;       // count for movement_state time
			int move_max;         // maximum length of move_state
			int count_skittle;    // time left on skittle
			int count_beak;       // beaks life time
			int bag_count;        // skittles left in bag
									// FOR ANIMATION
			 int sequence_count;   // count of elements length of duration
			 int sequence_max;     // threshold of sequence count
			 int element_count;    // tracks position in sequence
										  // element_count has no max as -1 is reached in array

		  } Beak, *beak_ptr;

typedef struct skittle_typ          //skittle datastructure
		  {
			int state;           // alive or dead
			int x_vel;           // x velocity
			int y_vel;           // y velocity
			int color;           // skittle color
			int skittle_time;    // time left on live skittle
		  } Skittle, *skittle_ptr;

// G L O B A L S  ////////////////////////////////////////////////////////////
sprite beaks[num_beaks],            //beak sprites
skittles[max_num_skittle];   //skittle sprites

Beak    beak_data[num_beaks];           // beak datastructures
Skittle skittle_data[max_num_skittle];  // skittle datastructures

int oldmode;
int beak_tag=0;                        // stores index no of beak going through main loop
int beak_tag2=0;                       // stores index no of beaks going through subloop of beaktag loop
int skittle_tag=0;                     // stores index of skittles going through loop
int index=0;                           // loop counter varible
int x=0;                               // storing min components for track / evade
int y=0;                               //
int fifty_fifty=0;                       // outcome of 50/50 decision
long minimum=infinity;         // min distance from object=dx^2 +dy^2: LONG for extra size

color palette[256];

///////////////////////////behaviour arrays/////////////////

int skittle_color[5] = {red,        // select skittle color
								orange,
								yellow,
								green,
								purple};


int beak_5050[2] = {lost,kept};           //allows 50/50 decisions

int beak_money[4] = {evade,
							track_shop,
							track_shop,
							track_shop,};         //selects money state behaviour

int beak_bag[4] = {evade,
						 evade,
						 random_1,
						 random_1};               //selects bag state behaviour

int beak_skittle[5] = {random_1,
							  random_1,
							  evade,
							  evade,
							  evade};            //selects skittle state behaviour

int beak_noskittle_ns_nm[2] = {random_1,
										 track_b};  //selects noskittle state behaviour if no skittles or money
														// include track_b back into this array ns_nm
int beak_noskittle_s[3] = {track_s,
									track_s,
									random_1};      //selects noskittle state behaviour if skittles on floor

int beak_noskittle_m[3] = {track_m,
									track_m,
									random_1};   //selects no skittle behaviour if money on floor


// F U N C T I O N S //////////////////////////////////////////////////////////

void make_background (void)
// Gets two virtual screens and puts the star sprites on them to make
//	a starfield background.
{
int i;

  scr = wnewblock (0, 0, 319, 199);		// get two virtual screens
  scr2 = wnewblock (0, 0, 319, 199);
  wsetscreen (scr2);				// go to second one
  wcls (0);					// clear it


}



////////////////////////////////////////////////////////////////
void Init_Beaks(void)
{
// this function initializes all the beaks and places
// them on the screen and sets them all to noskittle

int init_tag;        // count for beak initialisation
		 //		 printf("init for beaks \n");
// loop through and process all beaks

for (init_tag=0;init_tag<num_beaks;init_tag++)
	 {

	 // set up fields in data structure

	 beak_data[init_tag].dir        = beak_0;            // all start stationary,facing forward
	 beak_data[init_tag].state      = noskittle;               // has no skittle
	 beak_data[init_tag].move_state  = evade;                   // moves away from closest beak
	 beak_data[init_tag].move_count  = 0;                       // move_state counter =0
	 beak_data[init_tag].move_max    = move_time_max +rand()%2; // sets time before move_state change
	 beak_data[init_tag].count_skittle = 0;                       // starts with no skittletime
	 beak_data[init_tag].count_beak = time_max;                // starts with full lifetime
	 beak_data[init_tag].bag_count  = 0;                       // starts carrying no bag

	 beak_data[beak_tag].element_count = 0;                    // start on sequence element one
	 beak_data[beak_tag].sequence_count  = 0;                  //
	 beak_data[beak_tag].sequence_max    = 0;                  //
	 // set up fields in sprite structure

	 beaks[init_tag].curr_frame   = beak_0;
	 beaks[init_tag].x            = rand()%320;             //place randomly on screen
	 beaks[init_tag].y            = rand()%200;

  //	 printf("init x %d\n",beaks[init_tag].x);   // !!removeable test
  //	 printf("init y %d\n",beaks[init_tag].y);   // !!removeable test

	 } // end for init_tag

} // end Init_Beaks

///////////////////////////////////////////////////////////////////////////////

void Init_Skittles(void)
{
// this function initializes all the skittles and sets them all to skittle_dead

int init_tag;

// loop through and process all skittles (= max_num_skittles)
	 // printf("init skittles\n");
for (init_tag=0; init_tag<max_num_skittle; init_tag++)
	 {

	 // set up fields in data structure

	 skittle_data[init_tag].state = skittle_dead;        // dead
	 skittle_data[init_tag].x_vel =0;        // x velocity
	 skittle_data[init_tag].y_vel =0;        // y velocity
	 skittle_data[init_tag].color =1;        // red


	 // set up fields in sprite structure

	 skittles[init_tag].curr_frame   = 0;
	 skittles[init_tag].x            = 0;             //place nowhere
	 skittles[init_tag].y            = 0;

	 } // end for index
} // end Init_Skittles

/////////////////////////////////////////////////////////////////////////////

int Beak_Collide(sprite_ptr sprite_1,sprite_ptr sprite_2)
{

int dx,dy;

// compute overlap, if any.

dx = abs(sprite_1->x - sprite_2->x);
dy = abs(sprite_1->y - sprite_2->y);


// reduce size of bounding box
// NB shift multiplication 100 >>3 = 3 shifts of binary decimal point
// to the right = 100 *.5 *.5 *.5 = 12.5

if(dx<(beak_width-(beak_width>>3)) && dy<(beak_height-(beak_height>>3)))
  {

	return(1);

  }
else
  {

	return(0);

  }

} //end Beak_Collide

///////////////////////////////////////////////////////////////////////////////

int Skittle_Collide(sprite_ptr sprite_1,sprite_ptr sprite_2)
{

int dx,dy;

// compute overlap, if any.

dx = abs(sprite_1->x - sprite_2->x);
dy = abs(sprite_1->y - sprite_2->y);


// reduce size of bounding box
// NB shift multiplication 100 >>3 = 3 shifts of binary decimal point
// to the right = 100 *.5 *.5 *.5 = 12.5

if(dx<(beak_width-(beak_width>>3)) && dy<(beak_height-(beak_height>>3)))
  {

	return(1);

  }
else
  {

	return(0);

  }

} //end Skittle_Collide
//////////////////////////////////////////
void main (void)
{                      // start main
  printf ("\n SNEAKY BEAKS\n");
  getch ();

  oldmode = wgetmode ();
  vga256 ();
  minit ();
  wloadsprites (palette, "space.spr", sprites, 0, 20);  // load sprites
  wsetpalette (0, 255, palette);
  wnormscreen ();			// go back to normal screen
  wcls (0);				//clear screen
  wsetscreen (scr);				// go to first screen
  noclick ();

///////////////////////////////////////////////

  do {     // do until
		randomize();        // very important, resets pseudorandom seed  for every run of main loop
// AI logic///////////////////////////////

for(beak_tag=0; beak_tag<num_beaks; beak_tag++)            // NEEDS CHANGING TO All BEAKS   $$$$$$
	{
	 wputblock (rand() % 320, rand() % 200, sprites[5], 0);     // draw beaks

 	 randomize();        // very important, resets pseudorandom seed  for every beak in main loop
		//  printf("1    beak_tag    count%d\n",beak_tag);   // !!removeable test
	 ++beak_data[beak_tag].move_count;         // increase move_state count
		//  printf("2  beak_movecount%d\n",beak_data[beak_tag].move_count);   // !!removeable test
	 ++beak_data[beak_tag].sequence_count;     // increase count of time that element has been displayed for
		//  printf("3 x%d\n",beaks[beak_tag].x);   // !!removeable test
		//  printf("4 y%d\n",beaks[beak_tag].y);   // !!removeable test
switch(beak_data[beak_tag].state)             //each case tests for state change, then move_state change
		{
//////////////////////////////////////////////////////////////////////////////////

case noskittle:
		 {  //   printf("a  case_ns\n");   // !!removeable test
	  //	  printf("b count_beak%d\n",beak_data[beak_tag].count_beak);   // !!removeable test
		  --beak_data[beak_tag].count_beak;                      // reduce beaks remaining time
	  //			 printf("c count_beak%d\n",beak_data[beak_tag].count_beak);   // !!removeable test

		  if(beak_data[beak_tag].count_beak ==0)                 // test for dying beak = state change
			 {
			  beak_data[beak_tag].state = dying;                  // change state to dying
			  beak_data[beak_tag].move_state = none;               // assign no move_state
			  beak_data[beak_tag].move_count = 0;                  // set move_state count to zero
			  beak_data[beak_tag].move_max  = dying_time;          // takes constant time for death sequence
		 //	  printf(" state%d\n",beak_data[beak_tag].state);

			 }
		else
			{
//////////////////// not dead, test beak for collision ///////////////////////
		 /*
for(beak_tag2=0;beak_tag2<num_beaks;beak_tag2++)     // for every beak
	{
//if(beak_data[beak_tag2].state != dead)&&(Beak_Collide(beaks[beak_tag2],beaks[beak_tag]);)//test for hit beak_alive
	 {
	 if(beak_tag2 != beak_tag)    // test against collision with self
		{               ////// beak_tag beak set to kicking
		 beak_data[beak_tag].state = kicking;     // change state to kicking
		 beak_data[beak_tag].move_count = 0;     // set move_count to zero
		 beak_data[beak_tag].move_max  = kicking_time;        // takes constant time for kicking sequence
										  ////////  change state of kicked duck = beak_tag2

switch(beak_data[beak_tag].state)
		{

		case noskittle:            // test if beak has no skittle
			{
			 beak_data[beak_tag2].state = kicked_ns;            // state change to kicked with no skittle
			 beak_data[beak_tag2].move_state = none;            // state has no move sub state
			 beak_data[beak_tag2].move_count = 0;               // set move_count to zero
			 beak_data[beak_tag2].move_max = kicking_time;     // constant time for kicked
			}break;

		case skittle:              // test if beak has skittle
			{
			 beak_data[beak_tag2].state = kicked_s;             // state change to kicked with skittle
			 beak_data[beak_tag2].move_state = none;            // state has no move sub state
			 beak_data[beak_tag2].move_count = 0;               // set move_count to zero
			 beak_data[beak_tag2].move_max = kicking_time;     // constant time for kicked
			}break;

		case money:                // test if beak has money
			{
			 beak_data[beak_tag2].state = kicked_m;             // state change to kicked with money
			 beak_data[beak_tag2].move_state = none;            // state has no move sub state
			 beak_data[beak_tag2].move_count = 0;               // set move_count to zero
			 beak_data[beak_tag2].move_max = kicking_time;     // constant time for kicked
			}break;

		case bag:                  // test if beak has bag
			{
			 beak_data[beak_tag2].state = kicked_b;             // state change to kicked with bag
			 beak_data[beak_tag2].move_state = none;            // state has no move sub state
			 beak_data[beak_tag2].move_count = 0;               // set move_count to zero
			 beak_data[beak_tag2].move_max = kicking_time;     // constant time for kicked
			}break;

		default:break;

		 } //end switch
		}// end if not same beak
	  } // end if beak_collide
	 } // end for every beak

////////////////////////// test for skittle collision/////////////////////////////

	 for(skittle_tag=0;skittle_tag<max_num_skittle;skittle_tag++) // for every skittle
		 {
		 if(skittle_data[index].state == skittle_alive)&&(Skittle_collide(skittles[index],beaks[beak_tag]))//test for hit live
			{
			if(skittle_data[skittle_tag].color == grey)        // test for money
			  {
				beak_data[beak_tag].state = pecking;
				beak_data[beak_tag].move_state = peck_money;    // register sub_state
				beak_data[beak_tag].move_count = 0;
				beak_data[beak_tag].move_max = pecking_time;
			  }

	 else if(skittle_data[skittle_tag].color != grey)       // test for not money
			  {
				beak_data[beak_tag].state = pecking;
				beak_data[beak_tag].move_state = peck_skittle; // register sub_state
				beak_data[beak_tag].move_count = 0;
				beak_data[beak_tag].move_max = pecking_time;
			  }
			} // end if skittle collision
		 } // end for every skittle  */

	}  // end else




// must test for state changes to: pecking, money, kicking (kicking state must register kicked on beak kicked)
// when change to pecking(etc) move_max is assigned the value pecking_time (=constant)
// all involve collision detection
// all states without movestates must be assigned move_state = none to prevent
//  action on old move states kicking, pecking, all kickeds


	if(beak_data[beak_tag].move_count == beak_data[beak_tag].move_max) // test for move_state change
	  { //  printf("10 m_state_change\n");   // !!removeable test
		beak_data[beak_tag].move_count = 0;              // set move_state count to zero
		beak_data[beak_tag].move_max = move_time_max; // +rand()%5; // set state maximum count

	  if((skittle_data[skittle_tag].state == skittle_alive)&&(skittle_data[skittle_tag].color != grey))// test for live skittle
		 {
		  beak_data[beak_tag].move_state = beak_noskittle_s[rand()%3];                  // set behaviour
		 // printf("10a movestate s %d\n",beak_data[beak_tag].move_state);   // !!removeable test
		 }

else if((skittle_data[skittle_tag].state == skittle_alive)&&(skittle_data[skittle_tag].color == grey))// test for live coin
		 {
		  beak_data[beak_tag].move_state = beak_noskittle_m[rand()%3];                        // set behaviour
		 // printf("10b movestate m %d\n",beak_data[beak_tag].move_state);   // !!removeable test
		 }
	else
		 {
		  beak_data[beak_tag].move_state = beak_noskittle_ns_nm[rand()%2];  // deals with no skittles or money, set behaviour
		  // printf("10c movestate ns_nm %d\n",beak_data[beak_tag].move_state);   // !!removeable test
		 }   // end else
	  }   // end if move state change

	}break;       // end case noskittle

////////////////////////////////////////////////////////////////////////////
		case skittle:
		 { // printf("  case_s\n");   // !!removeable test
--beak_data[beak_tag].count_skittle;      // reduce time left on skittle

	if (beak_data[beak_tag].count_skittle ==0)                               // test for finished skittle = state change
		{
		 beak_data[beak_tag].state = noskittle;                             // change state to no_skittle
		 beak_data[beak_tag].count_beak = time_max;                         // set lifetime left to max
		 beak_data[beak_tag].move_count = 0;                                 // move_state count set to zero
		 beak_data[beak_tag].move_max = move_time_max + rand()%5;            // set move_state max_count
		 beak_data[beak_tag].move_state = beak_noskittle_ns_nm[rand()%5];         // assign new move_state
		}    // end if

	if (beak_data[beak_tag].move_count == beak_data[index].move_max)         // test for move_state change
		{
		 beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
		 beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state max_count
		 beak_data[beak_tag].move_state = beak_skittle[rand()%5];            // assign new move_state
		}    // end if



		 }break;// end case skittle

/////////////////////////////////////////////////////////////////////////
		case money:
		 { //  printf("  case_m\n");   // !!removeable test
		  if ((beaks[beak_tag].x == shop_x)&&(beaks[index].y == shop_y))      //collision detection with shop = state change
			{
			 beak_data[beak_tag].state = bag;                                      // change state to bag
			 beak_data[beak_tag].bag_count = bag_max;                              // bag contents = max
			 beak_data[beak_tag].move_state = beak_bag[rand()%4];                  // assign new move_state
			 beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
			 beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state max_count
			}


		 if (beak_data[beak_tag].move_count == beak_data[index].move_max)         // test for move_state change
			  {
				beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
				beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state max_count
				beak_data[beak_tag].move_state = beak_money[rand()%4];              // assign new move_state
			  }   // end if
		  }break; // end case  money

////////////////////////////////////////////////////////////////////////
		case bag:
		 {  //  printf("  case_bag\n");   // !!removeable test
		  if (beak_data[beak_tag].bag_count ==0)           		            // test for empty bag = state change
			 {
			  beak_data[beak_tag].state = skittle;				    // set state to skittle
			  beak_data[beak_tag].count_skittle = skittle_max;                       // set skittle time left to full
			  beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
			  beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state max_count
			  beak_data[beak_tag].move_state = beak_skittle[rand()%4];            // assign new move_state
			 }


		  if (beak_data[beak_tag].move_count == beak_data[index].move_max)           // test for move_state change
			  {
				beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
				beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state max_count
				beak_data[beak_tag].move_state = beak_bag[rand()%5];       	     // assign new move_state
			  }

		 }break;   // end case bag

//////////////////////////////////////////////////////////////////////
		case pecking:  //   printf("  case_peck\n");   // !!removeable test
		 {
			if (beak_data[beak_tag].move_count == pecking_time) // test for state change = constant time sequence ended
			{
			if(beak_data[index].move_state == peck_skittle)     // substate peck_skittle
			 {
			  beak_data[beak_tag].state = skittle;                                //with skittle
			  beak_data[beak_tag].count_skittle = skittle_data[index].skittle_time;      //full time !!MUST specify which skittles is eaten
			  beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
			  beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state max_count
			  beak_data[beak_tag].move_state = beak_skittle[rand()%5];       	     // assign new move_state
			  }
	 else if (beak_data[index].move_state == peck_money)    // substate peck_money
			 {
			  beak_data[beak_tag].state = money;                                //with skittle
			  beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
			  beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state max_count
			  beak_data[beak_tag].move_state = beak_money[rand()%4];
			 }
		  }  // end if =peck_time

  // must set to dead relevent skittle structures,

	 }break;   // end case pecking

//////////////////////////////////////////////////////////////////////
		case kicking:
		 { // printf(" m_case_kicking\n");   // !!removeable test
		  // registor kick on beak;                 ////!!!!!!!!! needs doing
		  if(beak_data[beak_tag].move_count == kicking_time)           // constant time sequence ended = test for state change
		  {
			 beak_data[beak_tag].state = noskittle;                         // return state to no skittles
			 beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
			 beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state maxcount
			 beak_data[beak_tag].move_state = beak_noskittle_ns_nm[rand()%2]; // !!! assign new move_state,but he should chase skittle
		  }
		 }break; // end case kicking
/////////////////////////////////////////////////////////////////////
		case kicked_ns:
		 { //  printf(" m_case_kicked_ns\n");   // !!removeable test
		  if(beak_data[beak_tag].move_count == kicking_time)                     // constant time sequence ended = test for state change
		  {
			 beak_data[beak_tag].state = noskittle;                              // set state to noskittle
			 beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
			 beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state max_count
			 beak_data[beak_tag].move_state = beak_noskittle_ns_nm[rand()%2];   // assign new move-state
		  }
		 }break;      // end case kicked_ns

/////////////////////////////////////////////////////////////////////
		case kicked_s:    //!!! some events here only to happen once
		 {// printf(" m_case_kicked_s\n");   // !!removeable test
		 fifty_fifty = beak_5050[rand()%2];
		 // if(fifty_fifty == lost)                              // 50% chance skittle is lost !TEST ONLY ONCE
// not yet defined:    skittle_money_dropped(beak_data[index].count_skittle,kicked_s);                                   // funtion not done see gravity functions
		  {
		  if(beak_data[beak_tag].move_count == kicking_time)           // constant time sequence ended = test for state change
		  {
			if(fifty_fifty == lost)                   // if skittle lost
			  {
			  beak_data[beak_tag].state = noskittle;                   // state change to noskittle
			  beak_data[beak_tag].count_beak = time_max;               // set lifetime left to max
			  beak_data[beak_tag].move_count = 0;                       // set move_state count to zero
			  beak_data[beak_tag].move_max = move_time_max +rand()%5;     // set move_state max_count
			  beak_data[beak_tag].move_state = beak_noskittle_s[rand()%3]; // assign new move-state, skitle on floor

			  }
		  else                                                        // skittle is kept
			 {
			 beak_data[beak_tag].state = skittle;                                // set state to skittle
			 beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
			 beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state max_count
			 beak_data[beak_tag].move_state = beak_skittle[rand()%5];       	   // assign new move-state
			 }
		  }  // end if
		} // end if
	}break; // end case kicked_s


/////////////////////////////////////////////////////////////////////
		case kicked_m:
		 { //  printf(" m_case_kicked_m\n");   // !!removeable test
		  fifty_fifty = beak_5050[rand()%2];  // 50% chance =1 = skittle is lost !TEST ONLY ONCE
	  //	 if(fifty_fifty == lost)
			{
			// skittle_money_dropped(beak_data[index].count_skittle,kicked_m);
			}  // end if 5050
		 if(beak_data[beak_tag].move_count == kicking_time)           // constant time sequence ended = test for state change
			{
		  if(fifty_fifty == lost)
			  {
				beak_data[beak_tag].state = noskittle;                // state changed to no skittle
				beak_data[beak_tag].move_count = 0;                        // set move_state count to zero
				beak_data[beak_tag].move_max = move_time_max +rand()%5;     // set move_state max_count
				beak_data[beak_tag].move_state = beak_noskittle_m[rand()%3]; // assign new move-state , money on floor
			  }    // end if 5050

			else         // kept money
			 {
			  beak_data[beak_tag].state = money;                         // state change to money
			  beak_data[beak_tag].move_count = 0;                        // set move_state count to zero
			  beak_data[beak_tag].move_max = move_time_max +rand()%50;     // set move_state max_count
			  beak_data[beak_tag].move_state = beak_money[rand()%4]; // assign new move-state
			 }  // end else
		 }    // end  if beak
		}break;    // end if case kicked_money

/////////////////////////////////////////////////////////////////////
		case kicked_b:
		 { //  printf(" m_case_kicked_b\n");   // !!removeable test
	  //   bag_kicked(beaks[index].x, beaks[index].y);               // pass x+y coords to function
		  if(beak_data[beak_tag].move_count == kicking_time)              // constant time sequence ended = test for state change
			{
			 if(beak_data[beak_tag].bag_count == 0)                   //  state change to skittle
			  {
				beak_data[beak_tag].state = skittle;
				beak_data[beak_tag].count_skittle = skittle_max;
				beak_data[beak_tag].move_count = 0;                        // set move_state count to zero
				beak_data[beak_tag].move_max = move_time_max +rand()%50;     // set move_state max_count
				beak_data[beak_tag].move_state = beak_skittle[rand()%5]; // assign new move-state
			  }
			 else      // bag != empty
			  {
				beak_data[beak_tag].state = bag;
				beak_data[beak_tag].move_count = 0;                        // set move_state count to zero
				beak_data[beak_tag].move_max = move_time_max +rand()%50;     // set move_state max_count
				beak_data[beak_tag].move_state = beak_bag[rand()%4]; // assign new move-state
			  }
		  } // end if beak
	 }break;    // end case kicked_b
////////////////////////////////////////////////////////////////////
		case dying:
		 {  //  printf("case dying \n");   // !!removeable test
		  if(beak_data[beak_tag].move_count == dying_time)      // constant time sequence ended = test for state change
			 {
			  beak_data[beak_tag].state = dead;                    // change state to dead
			  beak_data[beak_tag].move_state = none;               // no move_state, no movement
			  beaks[beak_tag].x =0;                                // set x position to 0
			  beaks[beak_tag].y =0;                                // set y position to 0

			  // TEST FOR HOLDING MONEY +so REPLACE IN SKITTLE DATASTRUCTURE
			 }
		 }break;  // end case dying
/////////////////////////////////////////////////////////////////////
		case dead:
		 {
		// printf("  case dead\n");
		 beak_data[beak_tag].move_count = 0;      // IMPORTANT. prevent overflow
		 }break;                                  // set move_state count to zero
		  default:break;
		}// end switch
		 // printf("  end switch 1\n");   // !!removeable test
		 // END AI LOGIC    //

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

//  BEGIN MOVEMENT   //  NB presently changes each game loop  !!!!!!!

switch(beak_data[beak_tag].move_state)             // make each beak move according to move_state
		{
		 case random_1:
		  {
		 //	printf(" m_case_rand\n");   // !!removeable test
			beaks[beak_tag].x +=(beak_speed-(rand()%beak_speed<<1));    // random x pos= (+- beak_speed)
			beaks[beak_tag].y +=(beak_speed-(rand()%beak_speed<<1));    // random y pos= (+- beak_speed)

		  }break; // end case random
/////////////////////////////////////////////////////////////////////////////////
		case track_s:
		 {//  printf(" m_case_track_sk\n");   // !!removeable test
		  x   =0;              // reinitialise  coordinates + min
		  y   =0;
		  minimum =infinity;           // impossibly high infinite no

for(skittle_tag=0; skittle_tag<max_num_skittle; skittle_tag++)        // for each skittle
	{     // printf("skittle loop %d\n",skittle_tag);  //removeable
	if((skittle_data[skittle_tag].color != grey)&&(skittle_data[skittle_tag].state != skittle_dead))   // test for skittle alive & not money
	  {
	  if((abs(skittles[skittle_tag].x - beaks[beak_tag].x)^2) +(abs(skittles[skittle_tag].y - beaks[beak_tag].y)^2) < minimum)  // if closest  =dx^2 +dy^2
		 {
		  minimum =((abs(skittles[skittle_tag].x - beaks[beak_tag].x)^2) +(abs(skittles[skittle_tag].y - beaks[beak_tag].y)^2));  // assign to min
		  x = skittles[skittle_tag].x;                  // assign skittles x pos to x
		  y = skittles[skittle_tag].y;                  // assign skittles y pos to y
		 }           // end if closest
	  }        // end if (live) & (not money) skittle
	}     // end for each skittle

	  if(x > beaks[beak_tag].x) //move towards closest with velocity component 10
		  beaks[beak_tag].x += beak_speed +rand()%10;
else if(x < beaks[beak_tag].x)
		  beaks[beak_tag].x -= beak_speed +rand()%10;    // x vector

	  if(y > beaks[beak_tag].y)
		  beaks[beak_tag].y += beak_speed +rand()%10;
else if(y < beaks[beak_tag].y)
		  beaks[beak_tag].y -= beak_speed +rand()%10;    // y vector

	}break;  // end case track_s


/////////////////////////////////////////////////////////////////////////////////
		case track_m:
		 { // printf(" m_case_track_m\n");   // !!removeable test
		  x   =0;              // reinitialise  coordinates + min
		  y   =0;
		  minimum =infinity;           // impossibly high infinite no

for(skittle_tag=0; skittle_tag<max_num_skittle; skittle_tag++)        // for each skittle
	{    //  printf("money_skittle loop %d\n",skittle_tag);  //removeable
	if((skittle_data[skittle_tag].color == grey)&&(skittle_data[skittle_tag].state != skittle_dead))   // test for skittle alive & money
	  {
	  if((abs(skittles[skittle_tag].x - beaks[beak_tag].x)^2) +(abs(skittles[skittle_tag].y - beaks[beak_tag].y)^2) < minimum)  // if closest  =dx^2 +dy^2
		 {
		  minimum =((abs(skittles[skittle_tag].x - beaks[beak_tag].x)^2) +(abs(skittles[skittle_tag].y - beaks[beak_tag].y)^2));  // assign to min
		  x = skittles[skittle_tag].x;                  // assign skittles x pos to x
		  y = skittles[skittle_tag].y;                  // assign skittles y pos to y
		 }           // end if closest
	  }        // end if (live) & (not money) skittle
	}     // end for each skittle

	  if(x > beaks[beak_tag].x) //move towards closest with velocity component 10
		  beaks[beak_tag].x += beak_speed +rand()%10;
else if(x < beaks[beak_tag].x)
		  beaks[beak_tag].x -= beak_speed +rand()%10;    // x vector

	  if(y > beaks[beak_tag].y)
		  beaks[beak_tag].y += beak_speed +rand()%10;
else if(y < beaks[beak_tag].y)
		  beaks[beak_tag].y -= beak_speed +rand()%10;    // y vector

	}break;  // end case track_s

/////////////////////////////////////////////////////////////////////////////////
		case track_b:
		 { // printf(" m_case_track_b\n");   // !!removeable test
		  x   =0;              // reinitialise  coordinates + min
		  y   =0;
		  minimum =infinity;           // impossibly high infinite no

for(beak_tag2=0; beak_tag2<num_beaks; beak_tag2++)   // for each beak
	{ //printf("2nd beak loop No: %d\n",beak_tag2);   // !!removeable test
	if((beak_tag2 != beak_tag)&&(beak_data[beak_tag2].state != dead))   // test for not same beak & beak alive
	  {  //  printf(" is not dead\n");
	  if((abs(beaks[beak_tag2].x - beaks[beak_tag].x)^2) + (abs(beaks[beak_tag2].y - beaks[beak_tag].y)^2) < minimum)  // if closest =dx^2 +dy^2
		 {//   printf(" is present closest\n");
		  minimum =((abs(beaks[beak_tag2].x - beaks[beak_tag].x)^2) + (abs(beaks[beak_tag2].y - beaks[beak_tag].y)^2));  // assign to min
		  x = beaks[beak_tag2].x;       // assign beaks x pos to x
		  y = beaks[beak_tag2].y;       // assign beaks y pos to y
		 // printf("min set to %d \n",minimum);
		 // printf("x set to %d \n",x);
		 // printf("y set to %d \n",y);
		 }           // end if closest
	  }           // end if (live) & (not same) beak
	}           //  end for each beak
	  //	printf("closest x %d\n",x);   // !!removeable test
	  //	printf("closest y %d\n",y);   // !!removeable test

	  if(x > beaks[beak_tag].x)  //move towards closest with velocity component 10
		  beaks[beak_tag].x += beak_speed + rand()%2;
else if(x < beaks[beak_tag].x)
		  beaks[beak_tag].x -= beak_speed + rand()%2;  // x vector

	  if(y > beaks[beak_tag].y)
		  beaks[beak_tag].y += beak_speed + rand()%2;  // y vector
else if(y < beaks[beak_tag].y)
		  beaks[beak_tag].y -= beak_speed + rand()%2;


		 }break;  // end case track_b


/////////////////////////////////////////////////////////////////////////////////
		case evade:
		 {// printf(" m_case_evade\n");   // !!removeable test
		 x   =0;              // reinitialise  coordinates + min
		 y   =0;
		 minimum =infinity;           // impossibly high infinite no
for(beak_tag2=0; beak_tag2<num_beaks; beak_tag2++)   // for each beak
	{ //printf("2nd beak loop No: %d\n",beak_tag2);   // !!removeable test
	if((beak_tag2 != beak_tag)&&(beak_data[beak_tag2].state != dead))   // test for not same beak & beak alive
	  { //   printf(" is not dead\n");
	  if((abs(beaks[beak_tag2].x - beaks[beak_tag].x)^2) + (abs(beaks[beak_tag2].y - beaks[beak_tag].y)^2) < minimum)  // if closest =dx^2 +dy^2
		 { //  printf(" is present closest\n");
		  minimum =((abs(beaks[beak_tag2].x - beaks[beak_tag].x)^2) + (abs(beaks[beak_tag2].y - beaks[beak_tag].y)^2));  // assign to min
		  x = beaks[beak_tag2].x;       // assign beaks x pos to x
		  y = beaks[beak_tag2].y;       // assign beaks y pos to y

		 }           // end if closest
	  }           // end if (live) & (not same) beak
	}           //  end for each beak
  //	 printf("min set to %d \n",minimum);  //rem
  //	 printf("x set to %d \n",x);               //rem
  //	 printf("y set to %d \n",y);                    //rem

	  if(x > beaks[beak_tag].x)  //move away from closest with velocity component 10
		  beaks[beak_tag].x -= beak_speed + rand()%2;
else if(x < beaks[beak_tag].x)
			beaks[beak_tag].x += beak_speed + rand()%2;    // x vector

	  if(y > beaks[beak_tag].y)
		  beaks[beak_tag].y -= beak_speed + rand()%2;
else if(y < beaks[beak_tag].y)
		  beaks[beak_tag].y += beak_speed + rand()%2;    // y vector

		 }break;   // end case evade
												 
/////////////////////////////////////////////////////////////////////////////
		case track_shop:
	 {                //move towards shop with velocity beak speed
		// printf(" m_case_track_shop\n");   // !!removeable test
	  if(shop_x > beaks[beak_tag].x)
		  beaks[beak_tag].x += beak_speed + rand()%2;     // x vector
else if(shop_x < beaks[beak_tag].x)
		  beaks[beak_tag].x -= beak_speed + rand()%2;

	  if(shop_y > beaks[beak_tag].y)
		  beaks[beak_tag].y += beak_speed + rand()%2;    // y  vector
else if(shop_y < beaks[beak_tag].y)
		  beaks[beak_tag].y -= beak_speed + rand()%2;

  	 }break; //end case track_shop

////////////////////////////////////////////////////////////////////////////////
		case peck_money:
	  { //printf(" m_case_peck_m\n");   // !!removeable test
		// does nothing ,stores if collision has been with money
	  }break;

/////////////////////////////////////////////////////////////////////////////
		case peck_skittle:
	  { //printf(" m_case_peck_s\n");   // !!removeable test
		// does nothing ,stores if collision has been with skittle
	  }break;

/////////////////////////////////////////////////////////////////////////////////
		case none:
	  {          // do nothing stops old move_states being acted on
		//printf(" m_case_none\n");   // !!removeable test
	  }break;    // set to none where a state has no movestates
		default:break;

  } // end switch


 //  animation frame chosing here

  //	printf("  end switch 2\n");   // !!removeable test

} // end for each beak

  //	printf(" MAIN LOOP       \n");   // removeable test line
	  } while (but != 2);		// until right button is pressed

////////////// END OF MAIN LOOP /////////////////////////////

				  mdeinit ();                   // Deinitialize the mouse handler
				  wfreeblock (scr);
				  wfreeblock (scr2);
				  wfreesprites (sprites, 0, 20);
				  wsetmode (oldmode);
}                       // end main

