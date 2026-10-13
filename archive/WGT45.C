/*
ษออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
บ                     WordUp Graphics Toolkit Version 4.5                    บ
บ                            Demonstration Program 45                        บ
บ                                                                            บ
บ This program loads in some sprites created with the WGT Sprite Editor and  บ
บ saves them in a new file using wsavesprites.                               บ
บ                                                                            บ
บ ฑฑฑ PROJECT ฑฑฑ                                                            บ
บ This program requires the WGT45.LIB file to be linked.                     บ
บ                                                                            บ
บ ฑฑฑ DATA FILES ฑฑฑ                                                         บ
บ Make sure that SPACE.SPR is in your executable directory.                  บ
บ                                                                            บ
ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
*/

#include <wgt45.h>

void main(void)
{
block sprites[10];      /* An array of blocks to load the sprites into.
			   Version 5.0 allows for any number to be
			   loaded in, as long as you pass the same size
			   of the array to the wloadsprites and wfreesprites
			   commands. */
short oldmode, i;
color palette[256];

  if (!vgadetected ())
    {
     printf ("Error - VGA card required for any WGT program.\n");
     exit (0);
    }
  printf ("WGT Example #45\n\n");
  printf ("10 sprites are loaded from a file, and then 5 of them are written out to\n");
  printf ("a new file using WSAVESPRITES. Press a key to end the program.\n");
  printf ("\n\nPress any key to continue.\n");
  getch ();

  oldmode = wgetmode ();
  vga256 ();
  
  wloadsprites (palette, "space.spr", sprites, 0, 9);
  /* loads the first 10 sprites */
  wsetpalette (0, 255, palette);
  
  for (i = 0; i < 10; i++)                      /* Display them */
    wputblock (i * 30, 0, sprites[i], NORMAL);

  /* Now save sprites 0 through 4 in a different file */
  wsavesprites (palette, "saved.spr", sprites, 0, 4);

  wtextcolor (1);
  wgtprintf (0, 50, NULL, "Sprites saved.");
  getch ();
  wsetmode (oldmode);
}

