

#include <wgt45.h>
#include <stdlib.h>


block scr, scr2;
block sprites[21];

int oldmode;
color palette[256];



void main (void)
{

  oldmode = wgetmode ();
  vga256 ();

  minit ();
  wsetpalette (0, 255, palette);

  wloadsprites (palette, "space.spr", sprites, 0, 20);  /* load sprites */
  wsetpalette (0, 255, palette);

 

  //wnormscreen ();			/* go back to normal screen */
  //wcls (0);				/* clear it */


  wsetscreen (scr);				/* go to first screen */
  noclick ();


  do {
		wputblock (rand() % 320, rand() % 200, sprites[rand()%10], 1);     // draw beaks

	  }  while (but != 2);		/* until right button is pressed */
  mdeinit ();                   /* Deinitialize the mouse handler */
  wfreeblock (scr);
  wfreeblock (scr2);
  wfreesprites (sprites, 0, 20);
  wsetmode (oldmode);
}

