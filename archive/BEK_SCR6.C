
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


//DEFINES ///////////////////////////////////

#define beak_width       20   // bitmap width
#define beak_height      30   // bitmap height

#define skittle_width     5   // bitmap width
#define skittle_height    5   // bitmap height

#define infinity      60000   // impossibly high number
#define gravity           5   //  = downward force
#define radius          130   // =XX + YY distance of closest beak
#define lost              1   // lost skittle/money
#define kept              0   // kept skittle/money
#define beak_speed        2   // beaks mean speed

#define num_beaks         4   // maximum no of beaks
#define max_num_skittle  30   // maximum number of skittles on screen
#define skittle_max      30   // maximum skittle time
#define bag_max          20   // maximum bag contents
#define time_max        200   // maximum beak lifetime, without skittle
#define move_time_max     3   // maximum time before change in movement


#define dying_time        1   // constant time for dying_sequence
#define pecking_time      5   // constant time for pecking sequence
#define kicking_time     30   // constant time for kicking + kicked sequences
#define walking_time      5   // constant time for walking sequence

// Direction of beak

#define beak_n       1                         //   9  1  3
#define beak_ne      3                         //   8  0  2
#define beak_e       2                         //  12  4  6
#define beak_se      6
#define beak_s       4
#define beak_sw     12
#define beak_w       8
#define beak_nw      9
#define beak_0       0       // still

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
#define dying       11       // beak is dying
#define dead        12       // beak is dead


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
#define gobble1      9       // beak eats skittle
#define gobble2     10       // beak eats skittle
#define gobble3     11       // beak eats skittle


// skittle colors
#define dead          0      // = dead
#define alive         1      // = r,o,y,g or v
#define grey          2      // = money on groud
#define drop          3      // = dropping money

// shop coordinates

#define shop_x     150       // plots into center of 320 *200 screen
#define shop_y      90

// ground states

#define g_skittle 3      // skittles are on the ground
#define g_money   2      // money is on the gruond
#define g_ns_nm   1      // no skittles or money on the ground

///////////////// PROTOTYPES //////////////

void make_background  (void);
void Init_Beaks       (void);
void Init_Skittles    (void);
void init_drop_money  (void); //from top of screen
void drop_money       (void);
void init_money_kicked(void); // out of beaks hand
void money_kicked     (void);
void init_bag_kicked  (void);
void bag_kicked       (void);
void show_beaks       (void);
void show_skittles    (void);
void show_shop        (void);
void skittle_collide  (void);
void beak_collide     (void);
void animation_control(void);

/////////////////////////////////////////////////////////////////
block scr, scr2;   // 1st & double buffer
block sprites[110]; // no of sprites{i.e. all animation frames} in game

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
			 int curr_frame;
			 int x;
			 int y;
			 int sequence_count;   // count of elements length of duration
			 int sequence_max;     // threshold of sequence count
			 int element_count;    // tracks position in sequence
										  // element_count has no max as -1 is reached in array

		  } Beak, *beak_ptr;

typedef struct skittle_typ          //skittle datastructure
		  {
			//int state;           // alive or dead
			int x_vel;           // x velocity
			int y_vel;           // y velocity
			int color;           // skittle color
			int skittle_time;    // time left on live skittle
			int dropping;            // stores final y coords of dropping skittle

			 int curr_frame;    //
			 int x;             //
			 int y;             //
          int sequence_count;   // count of elements length of duration
			 int sequence_max;     // threshold of sequence count
			 int element_count;    // tracks position in sequence
										  // element_count has no max as -1 is reached in array

		  } Skittle, *skittle_ptr;

// G L O B A L S  ////////////////////////////////////////////////////////////
Beak    beak_data[num_beaks];           // beak datastructures
Skittle skittle_data[max_num_skittle];  // skittle datastructures

int oldmode;                           // stores old display mode to restore later
int beak_tag   =0;                     // stores index no of beak going through main loop
int beak_tag2  =0;                     // stores index no of beaks going through subloop inside beaktag loop
int skittle_tag=0;                     // stores index of skittles going through loop
int x          =0;                     // storing min components for track / evade
int y          =0;                     //
int fifty_fifty=0;                     // outcome of 50/50 decision
int ground_state=g_ns_nm;              // init to no_skit, no_money
int x_move=0;                          // to store X+Y components of random move
int y_move=0;                          //
long minimum=infinity;         // min distance from object=dx^2 +dy^2: LONG for extra size


color palette[256];

///////////////////////////behaviour arrays/////////////////


int beak_5050[2]            = {lost,kept};   //allows 50/50 decisions

int beak_money[4]           = {track_shop,
										 track_shop,
										 track_shop,
										 track_shop,}; //selects money state behaviour

int beak_bag[12]            = {gobble1,evade,evade,
										 gobble2,random_1,random_1,
										 gobble3,random_1,random_1,
										 evade,evade,evade};   //selects bag state behaviour

int beak_skittle[5]         = {random_1,
										 random_1,
										 evade,
										 evade,
										 evade};     //selects skittle state behaviour

int beak_noskittle_ns_nm[2] = {track_b,
										 random_1};  //selects noskittle state behaviour if no skittles or money
														// include track_b back into this array ns_nm
int beak_noskittle_s[3]     = {track_s,
										 track_s,
										 random_1};      //selects noskittle state behaviour if skittles on floor

int beak_noskittle_m[3]     = {track_m,
										 track_m,
										 random_1};   //selects no skittle behaviour if money on floor

int bags[12]                 ={gobble1,gobble2,gobble3,
										 random_1,random_1,random_1,
										 none,none,none,
										 none,none,none};

int move[5]                 = {-2,-1,0,1,2};  // possible values for random movements

///////////////////////////// ANIMATION SETS ///////////////////////////////////////////////
int walk_n[3]    ={2,3,-1};
int walk_e[9]    ={9,10,11,12,13,12,11,10,-1};
int walk_s[3]    ={0,1,-1};             //-1 is END OF SET flag
int walk_w[9]    ={4,5,6,7,8,7,6,5,-1};

int coin_flip[7] ={14,15,16,17,16,15,-1};
int skittles[5]  ={18,19,20,21,22};   // red, orange, yellow, green, purple

int walk_n_m[3]  ={25,26,-1};                     // walk holding money
int walk_e_m[9]  ={32,33,34,35,36,35,34,33,-1};
int walk_s_m[3]  ={23,24,-1};
int walk_w_m[9]  ={27,28,29,30,31,30,29,28,-1};

int walk_n_b[3]  ={39,40,-1};                        // walk holding bag
int walk_e_b[9]  ={46,47,48,49,50,49,48,47,-1};
int walk_s_b[3]  ={37,38,-1};
int walk_w_b[9]  ={41,42,43,44,45,44,43,42,-1};


int flapping[4]  ={51,52,53,-1};
int looking[13]  ={54,55,56,57,58,59,60,59,58,57,56,55,-1};
int pecking_w[5] ={61,62,63,62,-1};                            // -2 is END OF SET flag for non-looping sets
int pecking_e[5] ={64,65,66,65,-1};
int kicking_w[11]={67,68,69,70,71,72,73,74,75,76,-2};
int kicking_e[11]={77,78,79,80,81,82,83,84,85,86,-2};
int eating1[10]   ={88,89,90,91,92,93,94,95,96,-1};  // purple
int eating2[10]   ={88,89,90,97,98,93,94,95,96,-1};  // red
int eating3[10]   ={88,89,90,99,100,93,94,95,96,-1}; // green

int kicked_ks[2]  ={1,-1};             // kicked + keep skittle                 L+R
int kicked_ls[2]  ={1,-1};             // kicked +(lose skittle OR no skittle)  L+R
int kicked_bag [2]={1,-1};             // kicked holding bag                    L+R
int dying_1[2]    ={1,-1};             // 1st dying sequence
int dying_2[2]    ={1,-1};
//int kicked_m[2]  ={1,-1};             // kicked holding money                  LOSE or KEEP


// F U N C T I O N S //////////////////////////////////////////////////////////

void play(void)
{ // do until
 if(kbhit() && getch()=='m')   // if m hit
	 init_drop_money();         // init coindrop from top of screen

///////////////////// establish state of GROUND ///////////////////////////////
	 ground_state=g_ns_nm;                      // reset to g_ns_nm
	 //direction=0;                         // reset to neutral
for(skittle_tag=0; skittle_tag<max_num_skittle; skittle_tag++)
	{
	  if(skittle_data[skittle_tag].color ==alive)// test for live skittle
		  ground_state=g_skittle;
else if((skittle_data[skittle_tag].color == grey)&&(ground_state!=g_skittle))// test for live coin
		  ground_state=g_money;
	}
	// printf("g_s  %d\n",ground_state);
/////////////////////////////// AI logic///////////////////////////////

for(beak_tag=0; beak_tag<num_beaks; beak_tag++)
	{
	 //printf("state%d\n",beak_data[beak_tag].state);
	 //printf("move%d\n",beak_data[beak_tag].move_state);

	 ++beak_data[beak_tag].move_count;         // increase move_state count
	 ++beak_data[beak_tag].sequence_count;     // increase count of time that element has been displayed for
  
switch(beak_data[beak_tag].state)             //each case tests for state change, then move_state change
		{
//////////////////////////////////////////////////////////////////////////////////

case noskittle:
		 {   //    printf("  case_ns\n");   // !!removeable test
		  --beak_data[beak_tag].count_beak;    // reduce beaks remaining time

		if(beak_data[beak_tag].count_beak ==0)     // test for dying beak = state change
		  {
			beak_data[beak_tag].state      = dying;       // change state to dying
			beak_data[beak_tag].move_state = none;        // assign no move_state
			beak_data[beak_tag].move_count = 0;           // set move_state count to zero
			beak_data[beak_tag].move_max   = dying_time;  // takes constant time for death sequence
		  }
	 else

		  {
			beak_collide();    // test for beak collision
			skittle_collide(); // test for skittle collision
		  }  // end else


// must test for state changes to: pecking, money, kicking (kicking state must register kicked on beak kicked)
// when change to pecking(etc) move_max is assigned the value pecking_time (=constant)
// all involve collision detection
// all states without movestates must be assigned move_state = none to prevent
//  action on old move states kicking, pecking, all kickeds

//////////////////////// assign a move state to each beak based on ground_state //////////////////

	if(beak_data[beak_tag].move_count == beak_data[beak_tag].move_max) // test for move_state change
	  {
		beak_data[beak_tag].move_count = 0;              // set move_state count to zero
		beak_data[beak_tag].move_max = move_time_max; // +rand()%5; // set state maximum count
		
		if(ground_state==g_skittle)
			beak_data[beak_tag].move_state = beak_noskittle_s[rand()%3];  // set behaviour
 else if(ground_state==g_money)
			beak_data[beak_tag].move_state = beak_noskittle_m[rand()%3];  // set behaviour
 else if(ground_state==g_ns_nm)
			beak_data[beak_tag].move_state = beak_noskittle_ns_nm[rand()%2];  // deals with no skittles or money, set behaviour

	  }   // end if move state change

	}break;       // end case noskittle

////////////////////////////////////////////////////////////////////////////
		case skittle:
		 {
		  --beak_data[beak_tag].count_skittle;      // reduce time left on skittle

	if (beak_data[beak_tag].count_skittle ==0)                               // test for finished skittle = state change
		{
		 beak_data[beak_tag].state = noskittle;                             // change state to no_skittle
		 beak_data[beak_tag].count_beak = time_max;                         // set lifetime left to max
		 beak_data[beak_tag].move_count = 0;                                 // move_state count set to zero
		 beak_data[beak_tag].move_max = move_time_max + rand()%5;            // set move_state max_count
		 beak_data[beak_tag].move_state = beak_noskittle_ns_nm[rand()%5];         // assign new move_state
		}    // end if

	if (beak_data[beak_tag].move_count == beak_data[beak_tag].move_max)         // test for move_state change
		{
		 beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
		 beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state max_count
		 beak_data[beak_tag].move_state = beak_skittle[rand()%5];            // assign new move_state
		}    // end if



		 }break;// end case skittle

/////////////////////////////////////////////////////////////////////////
case money:
	 {  
	 if((beak_data[beak_tag].x >= shop_x)&&(beak_data[beak_tag].x <= shop_x+10)&&
		 (beak_data[beak_tag].y >= shop_y)&&(beak_data[beak_tag].y <= shop_y+10))
				 //collision detection with shop = state change

		{
		 beak_data[beak_tag].state = bag;                                      // change state to bag
		 beak_data[beak_tag].bag_count = bag_max;                              // bag contents = max
		 beak_data[beak_tag].move_state = beak_bag[rand()%12];                  // assign new move_state
		 beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
		 beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state max_count
		}

	if(beak_data[beak_tag].move_count == beak_data[beak_tag].move_max)         // test for move_state change
	  {
		beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
		beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state max_count
		beak_data[beak_tag].move_state = beak_money[rand()%4];              // assign new move_state
	  }   // end if
	}break; // end case  money

////////////////////////////////////////////////////////////////////////
case bag:
	{          //    printf("bag\n");
	if(beak_data[beak_tag].bag_count ==0)           		            // test for empty bag = state change
	  {
		beak_data[beak_tag].state = skittle;				    // set state to skittle
		beak_data[beak_tag].count_skittle = skittle_max;                       // set skittle time left to full
		beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
		beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state max_count
		beak_data[beak_tag].move_state = beak_skittle[rand()%4];            // assign new move_state
	  }


	if(beak_data[beak_tag].move_count == beak_data[beak_tag].move_max)           // test for move_state change
	  {
		beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
		beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state max_count
		beak_data[beak_tag].move_state = beak_bag[rand()%12];       	     // assign new move_state
	  }
		 //	 printf("ms  %d\n",beak_data[beak_tag].move_state);
	}break;   // end case bag

//////////////////////////////////////////////////////////////////////
case pecking:    // printf("  case_peck\n");   // !!removeable test
	{               // printf("dir%d\n",beak_data[beak_tag].dir);
				  //	  printf("max%d\n",beak_data[beak_tag].move_max);
					//  printf("state%d\n",beak_data[beak_tag].move_state);

	if (beak_data[beak_tag].move_count == pecking_time) // test for state change = constant time sequence ended
		{
		if(beak_data[beak_tag].move_state == peck_skittle)  // substate peck_skittle
		  {
			beak_data[beak_tag].state = skittle;                            //with skittle
		 //	beak_data[beak_tag].count_skittle = skittle_data[skittle_tag].skittle_time;  //full time !!MUST specify which skittles is eaten
			beak_data[beak_tag].move_count = 0;                            // set move_state count to zero
			beak_data[beak_tag].move_max = move_time_max +rand()%5;        // set move_state max_count
			beak_data[beak_tag].move_state = beak_skittle[rand()%5];       // assign new move_state
		  }
 else if(beak_data[beak_tag].move_state == peck_money)    // substate peck_money
		  {
			beak_data[beak_tag].state = money;                         //with skittle
			beak_data[beak_tag].move_count = 0;                        // set move_state count to zero
			beak_data[beak_tag].move_max = move_time_max +rand()%5;    // set move_state max_count
			beak_data[beak_tag].move_state = beak_money[rand()%4];
		  }
		  }  // end if =peck_time

  // must set to dead relevent skittle structures,

	 }break;   // end case pecking

//////////////////////////////////////////////////////////////////////
case kicking:
	{
	 if(beak_data[beak_tag].move_count == kicking_time) // constant time sequence ended = test for state change
		{
		 beak_data[beak_tag].state = noskittle;                    // return state to no skittles
		 beak_data[beak_tag].move_count = 0;                       // set move_state count to zero
		 beak_data[beak_tag].move_max = move_time_max +rand()%5;   // set move_state maxcount
		 beak_data[beak_tag].move_state = track_s;                 // assign new move_state: track_skittle
		}
	}break; // end case kicking
/////////////////////////////////////////////////////////////////////
case kicked_ns:
	{
	if(beak_data[beak_tag].move_count == kicking_time) // constant time sequence ended = test for state change
	  {
		beak_data[beak_tag].state = noskittle;                            // set state to noskittle
		beak_data[beak_tag].move_count = 0;                               // set move_state count to zero
		beak_data[beak_tag].move_max = move_time_max +rand()%5;           // set move_state max_count
		beak_data[beak_tag].move_state = beak_noskittle_ns_nm[rand()%2];  // assign new move-state
	  }
	}break;      // end case kicked_ns

/////////////////////////////////////////////////////////////////////
case kicked_s:    //!!! some events here only to happen once
	{
  fifty_fifty = beak_5050[rand()%2];  // 50% chance skittle is lost !TEST ONLY ONCE
  if(fifty_fifty == lost)
	 {
	  // not yet defined:    skittle_money_dropped(beak_data[index].count_skittle,kicked_s);
	 }                                   // funtion not done see gravity functions

  if(beak_data[beak_tag].move_count == kicking_time)           // constant time sequence ended = test for state change
	 {
	 if(fifty_fifty == lost)                   // if skittle lost
		{
		 beak_data[beak_tag].state = noskittle;                   // state change to noskittle
		 beak_data[beak_tag].count_beak = time_max;               // set lifetime left to max
		 beak_data[beak_tag].move_count = 0;                       // set move_state count to zero
		 beak_data[beak_tag].move_max = move_time_max +rand()%5;     // set move_state max_count
		 beak_data[beak_tag].move_state = track_s; // assign new move-state: track_skittle
		}
  else                                                        // skittle is kept
		{
		 beak_data[beak_tag].state = skittle;                                // set state to skittle
		 beak_data[beak_tag].move_count = 0;                                  // set move_state count to zero
		 beak_data[beak_tag].move_max = move_time_max +rand()%5;              // set move_state max_count
		 beak_data[beak_tag].move_state = beak_skittle[rand()%5];       	   // assign new move-state
		}
	 }  // end if =kicking time
  }break; // end case kicked_s


/////////////////////////////////////////////////////////////////////
case kicked_m:
	{
	 fifty_fifty = beak_5050[rand()%2];  // 50% chance =1 = skittle is lost !TEST ONLY ONCE
 if(fifty_fifty == lost)
	{
	 // skittle_money_dropped(beak_data[index].count_skittle,kicked_m);
	}  // end if 5050

 if(beak_data[beak_tag].move_count == kicking_time)           // constant time sequence ended = test for state change
	{
	if(fifty_fifty == lost)       // lose money
	  {
		beak_data[beak_tag].state = noskittle;                // state changed to no skittle
		beak_data[beak_tag].move_count = 0;                        // set move_state count to zero
		beak_data[beak_tag].move_max = move_time_max +rand()%5;     // set move_state max_count
		beak_data[beak_tag].move_state = beak_noskittle_m[rand()%3]; // assign new move-state , money on floor
	  }
 else // kept money
	  {
		beak_data[beak_tag].state = money;                         // state change to money
		beak_data[beak_tag].move_count = 0;                        // set move_state count to zero
		beak_data[beak_tag].move_max = move_time_max +rand()%5;   // set move_state max_count
		beak_data[beak_tag].move_state = beak_money[rand()%4];     // assign new move-state
	 }  // end else
	}    // end  if beak
  }break;    // end if case kicked_money

/////////////////////////////////////////////////////////////////////
case kicked_b:
	{
	 //bag_kicked(beak_data[index].x, beak_data[index].y);      // pass x+y coords to function
	 if(beak_data[beak_tag].move_count == kicking_time)  // constant time sequence ended = test for state change
		{
		if(beak_data[beak_tag].bag_count == 0)       //  state change to skittle
		  {
			beak_data[beak_tag].state = skittle;
			beak_data[beak_tag].count_skittle = skittle_max;
			beak_data[beak_tag].move_count = 0;                      // set move_state count to zero
			beak_data[beak_tag].move_max = move_time_max +rand()%50; // set move_state max_count
			beak_data[beak_tag].move_state = beak_skittle[rand()%5]; // assign new move-state
		  }
	 else      // bag not empty
		  {
			beak_data[beak_tag].state = bag;
			beak_data[beak_tag].move_count = 0;                       // set move_state count to zero
			beak_data[beak_tag].move_max = move_time_max +rand()%50;  // set move_state max_count
			beak_data[beak_tag].move_state = beak_bag[rand()%12];      // assign new move-state
		  }
		} // end if beak
	 }break;    // end case kicked_b
////////////////////////////////////////////////////////////////////
case dying:
	{
	 //++beak_data[beak_tag].element_count;      // increase count through sequence

	 if(dying_1[beak_data[beak_tag].element_count]== -1)  //end of sequence
		 beak_data[beak_tag].element_count=0;                // restart sequence

		 beak_data[beak_tag].curr_frame=dying_1[beak_data[beak_tag].element_count];
					
	 if(beak_data[beak_tag].move_count == dying_time)      // constant time sequence ended = test for state change
		{
		 beak_data[beak_tag].state      = dead;               // change state to dead
		 beak_data[beak_tag].move_state = none;               // no move_state, no movement
		 beak_data[beak_tag].x          =0;                   // set x position to 0
		 beak_data[beak_tag].y          =0;                   // set y position to 0

		}
	}break;  // end case dying
/////////////////////////////////////////////////////////////////////
case dead:
	{
	 beak_data[beak_tag].move_count = 0;      // IMPORTANT. prevent overflow
	}break;                                  // set move_state count to zero
	 default:break;
}// end switch

		 // END AI LOGIC    //

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

//  BEGIN MOVEMENT   //  NB presently changes each game loop  !!!!!!!

switch(beak_data[beak_tag].move_state)  // make each beak move according to move_state
		{
/////////////////////////////////////////////////////////////////////////////////////////
case random_1:
	{
	if(beak_data[beak_tag].move_count ==0 /*beak_data[beak_tag].move_max*/)
	  {
		//beak_data[beak_tag].move_count=0;
		beak_data[beak_tag].move_max=move_time_max+5;
		beak_data[beak_tag].dir=0;

      x_move=move[rand()%5];
		y_move=move[rand()%5];
		beak_data[beak_tag].x +=x_move;    // random x pos= (+- beak_speed)
		beak_data[beak_tag].y +=y_move;    // random y pos= (+- beak_speed)

		if(x_move<0)
		  beak_data[beak_tag].dir+=8;
 else if(x_move>0)
		  beak_data[beak_tag].dir+=2;
		if(y_move<0)
		  beak_data[beak_tag].dir+=1;
 else if(y_move>0)
		  beak_data[beak_tag].dir+=4;
	  }   //end if

 else
	 {
	  if(beak_data[beak_tag].dir==9) {beak_data[beak_tag].x-=beak_speed;beak_data[beak_tag].y-=beak_speed;}
	  if(beak_data[beak_tag].dir==1) {beak_data[beak_tag].y-=beak_speed;}
	  if(beak_data[beak_tag].dir==3) {beak_data[beak_tag].x+=beak_speed;beak_data[beak_tag].y-=beak_speed;}
	  if(beak_data[beak_tag].dir==2) {beak_data[beak_tag].x+=beak_speed;}
	  if(beak_data[beak_tag].dir==6) {beak_data[beak_tag].x+=beak_speed;beak_data[beak_tag].y+=beak_speed;}
	  if(beak_data[beak_tag].dir==4) {beak_data[beak_tag].y+=beak_speed;}
	  if(beak_data[beak_tag].dir==12){beak_data[beak_tag].x-=beak_speed;beak_data[beak_tag].y+=beak_speed;}
	  if(beak_data[beak_tag].dir==8) {beak_data[beak_tag].x-=beak_speed;}
	 }

  }break; // end case random

/////////////////////////////////////////////////////////////////////////////////
case track_s:
	{
	beak_data[beak_tag].dir=0;              // reset beaks direction to neutral
	 x   =0;              // reinitialise  coordinates + min
	 y   =0;
	 minimum =infinity;   // impossibly high infinite no

for(skittle_tag=0; skittle_tag<max_num_skittle; skittle_tag++)// for each skittle
	{
	if(skittle_data[skittle_tag].color != (grey||dead))   // test for skittle alive & not money
	  {
	  if((abs(skittle_data[skittle_tag].x - beak_data[beak_tag].x)^2) +(abs(skittle_data[skittle_tag].y - beak_data[beak_tag].y)^2) < minimum)  // if closest  =dx^2 +dy^2
		 {
		  minimum =((abs(skittle_data[skittle_tag].x - beak_data[beak_tag].x)^2) +(abs(skittle_data[skittle_tag].y - beak_data[beak_tag].y)^2));  // assign to min
		  x = skittle_data[skittle_tag].x;                  // assign skittles x pos to x
		  y = skittle_data[skittle_tag].y;                  // assign skittles y pos to y
		 }           // end if closest
	  }             // end if (live) & (not money) skittle
	}               // end for each skittle

		///////////move towards closest with velocity component 10 /////////////////////////

	  if(x > beak_data[beak_tag].x)
		 {
		  beak_data[beak_tag].x += beak_speed +rand()%10;
		  beak_data[beak_tag].dir+=2;
		 }
else if(x < beak_data[beak_tag].x)
		 {
		  beak_data[beak_tag].x -= beak_speed +rand()%10;    // x vector
		  beak_data[beak_tag].dir+=8;
		 }
	  if(y > beak_data[beak_tag].y)
		 {
		  beak_data[beak_tag].y += beak_speed +rand()%10;
		  beak_data[beak_tag].dir+=4;
		 }
else if(y < beak_data[beak_tag].y)
		 {
		  beak_data[beak_tag].y -= beak_speed +rand()%10;    // y vector
		  beak_data[beak_tag].dir+=1;
		 }
	}break;  // end case track_s


/////////////////////////////////////////////////////////////////////////////////
case track_m:
	  {
	  beak_data[beak_tag].dir=0;              // reset beaks direction to neutral
		x   =0;              // reinitialise  coordinates + min
		y   =0;
		minimum =infinity;   // impossibly high infinite no

for(skittle_tag=0; skittle_tag<max_num_skittle; skittle_tag++)  // for each skittle
	{
	if(skittle_data[skittle_tag].color == grey)  // test for skittle alive & money
	  {
	  if((abs(skittle_data[skittle_tag].x - beak_data[beak_tag].x)^2) +(abs(skittle_data[skittle_tag].y - beak_data[beak_tag].y)^2) < minimum)  // if closest  =dx^2 +dy^2
		 {
		  minimum =((abs(skittle_data[skittle_tag].x - beak_data[beak_tag].x)^2) +(abs(skittle_data[skittle_tag].y - beak_data[beak_tag].y)^2));  // assign to min
		  x = skittle_data[skittle_tag].x;                  // assign skittles x pos to x
		  y = skittle_data[skittle_tag].y;                  // assign skittles y pos to y
		 }           // end if closest
	  }             // end if (live) & (not money) skittle
	}               // end for each skittle

	  /////////move towards closest with velocity component beak_speed ///////////////////////////////

	  if(x > beak_data[beak_tag].x)
		 {
		  beak_data[beak_tag].x += beak_speed +rand()%10;
		  beak_data[beak_tag].dir+=2;
		 }
else if(x < beak_data[beak_tag].x)
		 {
		  beak_data[beak_tag].x -= beak_speed +rand()%10;    // x vector
		  beak_data[beak_tag].dir+=8;
		 }

	  if(y > beak_data[beak_tag].y)
		 {
		  beak_data[beak_tag].y += beak_speed +rand()%10;
		  beak_data[beak_tag].dir+=4;
		 }
else if(y < beak_data[beak_tag].y)
		 {
		  beak_data[beak_tag].y -= beak_speed +rand()%10;    // y vector
		  beak_data[beak_tag].dir+=1;
		 }
	}break;  // end case track_s

/////////////////////////////////////////////////////////////////////////////////
case track_b:
	  {
		beak_data[beak_tag].dir=0;              // reset beaks direction to neutral
		x   =0;              // reinitialise  coordinates + min
		y   =0;
		minimum =infinity;   // impossibly high infinite no

for(beak_tag2=0; beak_tag2<num_beaks; beak_tag2++)   // for each beak
	{
	if((beak_tag2 != beak_tag)&&(beak_data[beak_tag2].state != dead))   // test for not same beak & beak alive
	  {
	  if((abs(beak_data[beak_tag2].x - beak_data[beak_tag].x)^2) + (abs(beak_data[beak_tag2].y - beak_data[beak_tag].y)^2) < minimum)  // if closest =dx^2 +dy^2
		 {
		  minimum =((abs(beak_data[beak_tag2].x - beak_data[beak_tag].x)^2) + (abs(beak_data[beak_tag2].y - beak_data[beak_tag].y)^2));  // assign to min
		  x = beak_data[beak_tag2].x;       // assign beaks x pos to x
		  y = beak_data[beak_tag2].y;       // assign beaks y pos to y
		 }                 // end if closest
	  }                   // end if (live) & (not same) beak
	}                     //  end for each beak

			////move towards closest with velocity component 10 ////////////////////////

	  if(x > beak_data[beak_tag].x)
		 {
		  beak_data[beak_tag].x += beak_speed + rand()%2;
		  beak_data[beak_tag].dir+=2;
		 }
else if(x < beak_data[beak_tag].x)
		 {
		  beak_data[beak_tag].x -= beak_speed + rand()%2;  // x vector
		  beak_data[beak_tag].dir+=8;
		 }

	  if(y > beak_data[beak_tag].y)
		 {
		  beak_data[beak_tag].y += beak_speed + rand()%2;  // y vector
		  beak_data[beak_tag].dir+=4;
		 }
else if(y < beak_data[beak_tag].y)
		 {
		  beak_data[beak_tag].y -= beak_speed + rand()%2;
		  beak_data[beak_tag].dir+=1;
		 }


	}break;  // end case track_b


/////////////////////////////////////////////////////////////////////////////////
case evade:
	  {
		beak_data[beak_tag].dir=0;              // reset beaks direction to neutral
		x   =0;              // reinitialise  coordinates + min
		y   =0;
		minimum =infinity;   // impossibly high infinite no

for(beak_tag2=0; beak_tag2<num_beaks; beak_tag2++)   // for each beak
	{
	if((beak_tag2 != beak_tag)&&(beak_data[beak_tag2].state != dead))   // test for not same beak & beak alive
	  {
	  if((abs(beak_data[beak_tag2].x - beak_data[beak_tag].x)^2) + (abs(beak_data[beak_tag2].y - beak_data[beak_tag].y)^2) < minimum)  // if closest =dx^2 +dy^2
		 {
		  minimum =((abs(beak_data[beak_tag2].x - beak_data[beak_tag].x)^2) + (abs(beak_data[beak_tag2].y - beak_data[beak_tag].y)^2));  // assign to min
		  x = beak_data[beak_tag2].x;       // assign beaks x pos to x
		  y = beak_data[beak_tag2].y;       // assign beaks y pos to y
		 }                // end if closest
	  }                  // end if (live) & (not same) beak
	}                    //  end for each beak


  if(minimum<radius)  // if closest is close enough
	 {   ////////////move away from closest with velocity component 10
	  if(x > beak_data[beak_tag].x)
		 {
		  beak_data[beak_tag].x -= beak_speed + rand()%2;
		  beak_data[beak_tag].dir+=8;
		 }
else if(x < beak_data[beak_tag].x)
		 {
		  beak_data[beak_tag].x += beak_speed + rand()%2;    // x vector
		  beak_data[beak_tag].dir+=2;
		 }
	  if(y > beak_data[beak_tag].y)
		 {
		  beak_data[beak_tag].y -= beak_speed + rand()%2;
		  beak_data[beak_tag].dir+=1;
		 }
else if(y < beak_data[beak_tag].y)
		 {
		  beak_data[beak_tag].y += beak_speed + rand()%2;    // y vector
		  beak_data[beak_tag].dir+=4;
		 }
	 } //end if minimum

else
	 {beak_data[beak_tag].move_state=bags[rand()%12];
	  beak_data[beak_tag].element_count=0;	}

	}break;   // end case evade

/////////////////////////////////////////////////////////////////////////////
case track_shop:
	 {     //move towards shop with velocity beak speed
		 beak_data[beak_tag].dir=0;              // reset beaks direction to neutral
	  if(shop_x > beak_data[beak_tag].x)
		 {
		  beak_data[beak_tag].x += beak_speed + rand()%2;     // x vector
		  beak_data[beak_tag].dir+=2;
		 }
else if(shop_x < beak_data[beak_tag].x)
		 {
		  beak_data[beak_tag].x -= beak_speed + rand()%2;
		  beak_data[beak_tag].dir+=8;
		 }
	  if(shop_y > beak_data[beak_tag].y)
		 {
		  beak_data[beak_tag].y += beak_speed + rand()%2;    // y  vector
		  beak_data[beak_tag].dir+=4;
		 }
else if(shop_y < beak_data[beak_tag].y)
		 {
		  beak_data[beak_tag].y -= beak_speed + rand()%2;
		  beak_data[beak_tag].dir+=1;
		 }
	}break; //end case track_shop

////////////////////////////////////////////////////////////////////////////////
case peck_money:
	{
	 // NB direction is esatblished in skittle_collide function
	 // stores if collision has been with money
	}break;

/////////////////////////////////////////////////////////////////////////////
case peck_skittle:
	{
	 // does nothing ,stores if collision has been with skittle
	}break;

/////////////////////////////////////////////////////////////////////////////////
case none:     // not moving
	{
	 if(beak_data[beak_tag].state==bag)      //if holding bag
		beak_data[beak_tag].curr_frame=37;   // show beak with bag

 else{beak_data[beak_tag].curr_frame=0;}
	}break;    // set to none where a state has no movestates
	default:break;

 } // end switch


// Make them bounce off the walls if they go too far in one direction.
	  if(beak_data[beak_tag].x <= 0)
		  beak_data[beak_tag].x += beak_speed;
else if(beak_data[beak_tag].x >= 280)
		  beak_data[beak_tag].x -= beak_speed;

	  if(beak_data[beak_tag].y <= 0)
		  beak_data[beak_tag].y += beak_speed;
else if(beak_data[beak_tag].y >= 160)
		  beak_data[beak_tag].y -= beak_speed;

 animation_control(); // to select + cycle animation frames

  //  printf("ms %d\n",beak_data[beak_tag].move_state);

  } // end for each beak
  drop_money();        // move any dropping money

///////////////////// ANIMATIONS //////////////////////////
  show_skittles();     // show all live skittles
  show_shop();         // show skittle shop
  show_beaks();        // show all live beaks
  wretrace ();
  wcopyscreen (0, 0, 319, 199, scr, 0, 0, NULL);
}   // end play

//////////////////////////////////////////////////////////////////

void make_background (void)
// Gets two virtual screens
{
 int i;

  scr = wnewblock (0, 0, 319, 199);		// get two virtual screens
  scr2 = wnewblock (0, 0, 319, 199);
  wsetscreen (scr2);				// go to second one
  wcls (0);					// clear it

}

//////////////////////////////////////////////////

void background (void)
{
  wcopyscreen (0, 0, 319, 199, scr2, 0, 0, scr);
 // wcopyscreen (ix, iy, ix2, scrll, scr2, ix, iy2 - scrll, scr);
  // Cut the screen into two sections, then swap them vertically on the
  //	  destination page
	 /*
  scrll += speed;
  if ((scrll < iy) && (speed < 0))
    scrll = iy2 + 1 - abs (scrll);
  if ((scrll > iy2) && (speed > 0))
	 scrll = abs (iy2 - scrll);   */
}

////////////////////////////////////////////////////////////////
void Init_Beaks(void)
{
 // this function initializes all the beaks and places
 // them on the screen and sets them all to noskittle

int init_tag;        // count for beak initialisation

randomize();         // reseed random
for (init_tag=0;init_tag<num_beaks;init_tag++)  // loop through and process all beaks
	 {
	  // set up fields in data structure
	 beak_data[init_tag].dir           = beak_0;            // all start stationary,facing forward
	 beak_data[init_tag].state         = noskittle;         // has no skittle
	 beak_data[init_tag].move_state    = evade;             // moves away from closest beak
	 beak_data[init_tag].move_count    = 0;                 // move_state counter =0
	 beak_data[init_tag].move_max      = move_time_max +rand()%2; // sets time before move_state change
	 beak_data[init_tag].count_skittle = 0;                  // starts with no skittletime
	 beak_data[init_tag].count_beak    = time_max;           // starts with full lifetime
	 beak_data[init_tag].bag_count     = 0;                  // starts carrying no bag

	 beak_data[init_tag].element_count  = 0;                  // start on sequence element one
	 beak_data[init_tag].sequence_count = 0;                //
	 beak_data[init_tag].sequence_max   = 0;                //
	 beak_data[init_tag].curr_frame     = beak_0;
	 beak_data[init_tag].x              = rand()%320;             //place randomly on screen
	 beak_data[init_tag].y              = rand()%200;

	 } // end for init_tag

} // end Init_Beaks

///////////////////////////////////////////////////////////////////////////////

void Init_Skittles(void)
{
// this function initializes all the skittles and sets them all to skittle_dead

int init_tag;
			  // loop through and process all skittles (= max_num_skittles)

for (init_tag=0; init_tag<max_num_skittle; init_tag++)
	 {

	 // set up fields in data structure
	 skittle_data[init_tag].x_vel      =0;        // x velocity
	 skittle_data[init_tag].y_vel      =0;        // y velocity
	 skittle_data[init_tag].color      =0;        // dead
	 skittle_data[init_tag].curr_frame =0;
	 skittle_data[init_tag].x          =0;        //place nowhere
	 skittle_data[init_tag].y          =0;

	 } // end for init_tag
} // end Init_Skittles

/////////////////////////////////////////////////////////////////////////////

void beak_collide(void)      //responsible for changing state of both beaks
{
 for(beak_tag2=0; beak_tag2 < num_beaks; beak_tag2++)
	 {
	  if ((beak_tag!=beak_tag2)&&(beak_data[beak_tag2].state!=dead)&&
			(beak_data[beak_tag2].x > beak_data[beak_tag].x) && (beak_data[beak_tag2].x < beak_data[skittle_tag].x + 7) &&
			(beak_data[beak_tag2].y > beak_data[beak_tag].y) && (beak_data[beak_tag2].y < beak_data[skittle_tag].y + 7))
			{					////// beak_tag beak set to kicking
			 beak_data[beak_tag].state = kicking;     // change state to kicking
			 beak_data[beak_tag].move_count = 0;     // set move_count to zero
			 beak_data[beak_tag].move_max  = kicking_time;        // takes constant time for kicking sequence
										  ////////  change state of kicked duck = beak_tag2

switch(beak_data[beak_tag2].state)
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

			// establish kicking direction
	  if(beak_data[beak_tag2].x > beak_data[beak_tag].x)
		  beak_data[beak_tag].dir=2;
	  if(beak_data[beak_tag].x < beak_data[beak_tag].x)
		  beak_data[beak_tag].dir=8;
	 }   // end for
}
///////////////////////////////////////////////////////////////////////////////

void skittle_collide(void)   //responsible for changing state of skittle + beak
{
 for (skittle_tag=0; skittle_tag < max_num_skittle; skittle_tag++)
	  {
		if ((skittle_data[skittle_tag].color !=dead)&&
			(beak_data[beak_tag].x > skittle_data[skittle_tag].x) && (beak_data[beak_tag].x < skittle_data[skittle_tag].x + 10) &&
			(beak_data[beak_tag].y > skittle_data[skittle_tag].y) && (beak_data[beak_tag].y < skittle_data[skittle_tag].y + 10))
			{
			if(skittle_data[skittle_tag].color == grey)        // test for money
			  { 
				beak_data[beak_tag].state = pecking;
				beak_data[beak_tag].move_state = peck_money;    // register sub_state
				beak_data[beak_tag].move_count = 0;
				beak_data[beak_tag].move_max = pecking_time;
			  }
	 else if(skittle_data[skittle_tag].color == alive)       // test for not money
			  {
				beak_data[beak_tag].state = pecking;
				beak_data[beak_tag].move_state = peck_skittle; // register sub_state
				beak_data[beak_tag].move_count = 0;
				beak_data[beak_tag].move_max = pecking_time;
				beak_data[beak_tag].count_skittle=skittle_data[skittle_tag].skittle_time;
			  }

		  if(skittle_data[skittle_tag].x > beak_data[beak_tag].x)  // set L or R peck
			  beak_data[beak_tag].dir=2;
		  if(skittle_data[skittle_tag].x < beak_data[beak_tag].x)
			  beak_data[beak_tag].dir=8;

           ///////////////////// reinitialise the money-skittle  ///////////////////
			 skittle_data[skittle_tag].x_vel      =0;
			 skittle_data[skittle_tag].y_vel      =0;
			 skittle_data[skittle_tag].color      =0;
			 skittle_data[skittle_tag].curr_frame =0;
			 skittle_data[skittle_tag].x          =0;
			 skittle_data[skittle_tag].y          =0;

			}   // end big if
	  }        // end for eack skittle

} //end Skittle_Collide
// NB  NB  establish pecking direction

///////////////////////////////////////////////////////////////////////////////////////////

void init_drop_money (void)
{
 int money_tag;    // loop control
 int flag=0;       // reinitialise: 0=not found slot, 1=found slot


for(money_tag=0;money_tag<max_num_skittle;money_tag++)    //loop all skittles
	{
  if((skittle_data[money_tag].color ==0)&&(flag==0))      // if free
	 {
	 skittle_data[money_tag].color     = drop;         // set to money dropping
	 skittle_data[money_tag].x         = rand() %320;  // random x pos
	 skittle_data[money_tag].y         = 0;            // top of screen
	 skittle_data[money_tag].dropping  = rand() %200;  // threshold for the drop
	 skittle_data[money_tag].curr_frame= 14;           // 1st coin frame
	 flag                              = 1;            // slot found
	 }
	}
}
/////////////////////////////////////////////////////////////////////////////

void drop_money(void)
{
 int d_money_tag=0;   // loop control

 for(d_money_tag=0;d_money_tag<max_num_skittle;d_money_tag++)  // loop all skittles
	 {
	  if((skittle_data[d_money_tag].color ==drop)&&(skittle_data[d_money_tag].y<skittle_data[d_money_tag].dropping))
		 {
		  ++skittle_data[d_money_tag].element_count;      // increase count through sequence

		  if(coin_flip[skittle_data[d_money_tag].element_count]== -1)  //end of sequence
			  skittle_data[d_money_tag].element_count=0;                // restart sequence

			 skittle_data[d_money_tag].curr_frame=coin_flip[skittle_data[d_money_tag].element_count];
			 skittle_data[d_money_tag].y += gravity;      // move skittle down
		 }

	else if((skittle_data[d_money_tag].color ==drop)&&(skittle_data[d_money_tag].y>=skittle_data[d_money_tag].dropping))
			{
			 skittle_data[d_money_tag].color =grey;
			 skittle_data[d_money_tag].curr_frame=14;
          skittle_data[d_money_tag].element_count=0;
			}
	 }
}      //element_count //curr_frame
///////////////////////////////////////////////////////////////////////
void show_beaks(void)
{
int beak_count;

for(beak_count=0;beak_count<num_beaks;beak_count++)
	{
	 if(beak_data[beak_count].state !=dead) // test for live beaks
		{
		 wputblock(beak_data[beak_count].x,beak_data[beak_count].y,sprites[beak_data[beak_count].curr_frame],1);
		}
	}
}
///////////////////////////////////////////////////////////////////////
void show_skittles(void)
{
int skit_count=0;

for(skit_count=0;skit_count<max_num_skittle;skit_count++)
	{
	if(skittle_data[skit_count].color !=0)  // if a live skittle
	  {
		wputblock(skittle_data[skit_count].x,skittle_data[skit_count].y,sprites[skittle_data[skit_count].curr_frame],1);
		// need to change to show curr_frame
	  }
	}
}
///////////////////////////////////////////////////////////////////////
void show_shop(void)
 {
  wputblock(shop_x,shop_y,sprites[87],1);
 }

///////////////////////////////////////////////////////////////////////
 void animation_control(void)
{
	switch(beak_data[beak_tag].state)
			{
			//////////////////////////////////////////////////////////////////////////////////////////////////
			case noskittle:
				{
				 ++beak_data[beak_tag].element_count;   // move to next frame in sequence

				switch(beak_data[beak_tag].dir)
						{
						 case beak_n:
						 {
						 if((walk_n[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>3))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_n[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_e:
						 {
						 if((walk_e[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_e[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_ne:
						 {
						 if((walk_e[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_e[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_se:
						 {
						 if((walk_e[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_e[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_s:
						 {
						 if((walk_s[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>3))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_s[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_w:
						 {
						 if((walk_w[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_w[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_nw:
						 {
						 if((walk_w[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_w[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_sw:
						 {
						 if((walk_w[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_w[beak_data[beak_tag].element_count];
						 }break; //end case

						} // end switch
				 }break; // end case noskittle

////////////////////////////////////////////////////////////////////////////////////////////////////////
			case money:
				{
				 ++beak_data[beak_tag].element_count;   // move to next frame in sequence

				switch(beak_data[beak_tag].dir)
						{
						 case beak_n:
						 {
						 if((walk_n_m[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>3))  //end of sequence
							 beak_data[beak_tag].element_count=0;                // restart sequence
							 beak_data[beak_tag].curr_frame=walk_n_m[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_e:
						 {
						 if((walk_e_m[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_e_m[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_ne:
						 {
						 if((walk_e_m[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_e_m[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_se:
						 {
						 if((walk_e_m[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_e_m[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_s:
						 {
						 if((walk_s_m[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>3))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_s_m[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_w:
						 {
						 if((walk_w_m[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_w_m[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_nw:
						 {
						 if((walk_w_m[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_w_m[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_sw:
						 {
						 if((walk_w_m[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_w_m[beak_data[beak_tag].element_count];
						 }break; //end case
						}   // end switch DIR
				}break;    // end case money
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
			case bag:
				{
				 ++beak_data[beak_tag].element_count;   // move to next frame in sequence

			if(beak_data[beak_tag].move_state==gobble1) // signals eating purple skittle
			  {
				if(eating1[beak_data[beak_tag].element_count]== -1)
					beak_data[beak_tag].element_count=0;

					beak_data[beak_tag].curr_frame=eating1[beak_data[beak_tag].element_count];
			  }
				//////////////////////////
			if(beak_data[beak_tag].move_state==gobble2) // signals eating red skittle
			  {
				if(eating2[beak_data[beak_tag].element_count]== -1)
					beak_data[beak_tag].element_count=0;

					beak_data[beak_tag].curr_frame=eating2[beak_data[beak_tag].element_count];
			  }
				/////////////////////////
			if(beak_data[beak_tag].move_state==gobble3) // signals eating green skittle
			  {
				if(eating3[beak_data[beak_tag].element_count]== -1)
					beak_data[beak_tag].element_count=0;

					beak_data[beak_tag].curr_frame=eating3[beak_data[beak_tag].element_count];
			  }
		 else
			  {
				switch(beak_data[beak_tag].dir)
						{
						 case beak_n:
						 {
						 if((walk_n_b[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>3))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_n_b[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_e:
						 {
						 if((walk_e_b[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_e_b[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_ne:
						 {
						 if((walk_e_b[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_e_b[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_se:
						 {
						 if((walk_e_b[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_e_b[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_s:
						 {
						 if((walk_s_b[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>3))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_s_b[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_w:
						 {
						 if((walk_w_b[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_w_b[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_nw:
						 {
						 if((walk_w_b[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_w_b[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_sw:
						 {
						 if((walk_w_b[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>9))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=walk_w_b[beak_data[beak_tag].element_count];
						 }break; //end case
						}   // end switch DIR
					} //end else
				}break;    // end case bag

/////////////////////////////////////////////////////////////////////////////////////////
 /*			case pecking:
				{
				 ++beak_data[beak_tag].element_count;   // move to next frame in sequence

				switch(beak_data[beak_tag].dir)
						{
						 case beak_e:
						 {
						 if((pecking_e[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>5))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=pecking_e[beak_data[beak_tag].element_count];
						 }break; //end case

						 case beak_w:
						 {
						 if((pecking_w[beak_data[beak_tag].element_count]== -1)||(beak_data[beak_tag].element_count>5))  //end of sequence
							beak_data[beak_tag].element_count=0;                // restart sequence
							beak_data[beak_tag].curr_frame=pecking_w[beak_data[beak_tag].element_count];
						 }break; //end case
						 default:{printf("error");}break;
						}   // end switch DIR
				}break;    // end case pecking
                                                 */
//////////////////////////////////////////////////////////////////////////////////
 }     // end switch STATE
}
//////////////////////////////////////////
void main (void)
{                      // start main
  randomize();
  Init_Beaks();
  Init_Skittles();
  // make_background();
  printf ("\n SNEAKY BEAKS\n");
  getch ();

  oldmode = wgetmode ();
  vga256 ();
  minit ();

  wsetpalette (0, 255, palette);
  wloadsprites (palette, "beak.spr", sprites, 0, 110);  // load sprites
  wsetpalette (0, 255, palette);
  make_background();   // creates background
  wnormscreen ();			// go back to normal screen
  wcls (0);				//clear screen
  wsetscreen (scr);				// go to first screen
  noclick ();                 // wait for release of mouse
  init_drop_money();          // drop 1st coin

//////////////MAIN EVENT LOOP //////////////////

  do {
		background();        // clears screen for each loop
		play();  // main loop
		delay(50); // where delay =(INT/18) secs
	  }
while(but != 2);		// until right button is pressed

////////////// END OF MAIN LOOP /////////////////////////////

				  mdeinit ();                   // Deinitialize the mouse handler
				  wfreeblock (scr);
				  wfreeblock (scr2);
				  wfreesprites (sprites, 0, 110);
				  wsetmode (oldmode);
}                       // end main

