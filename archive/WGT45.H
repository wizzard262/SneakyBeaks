#ifndef _WGT45_
#define _WGT45_

/* 
   WordUp Graphics Toolkit V4.5 Header File
   Real Mode Version

   Copyright 1996 Egerter Software
*/

#include <stdio.h>
#include <dos.h>

#ifdef __cplusplus
 extern "C" {
#endif

typedef unsigned char far * block;
typedef unsigned char far * wgtfont;
typedef unsigned short far * wgtmap;

/* Text grid settings */
#define TEXTGRID_OFF 0
#define TEXTGRID_ON 1

/* Text transparency settings */
#define TEXTFG 0
#define TEXTBG 1
#define TEXTFGBG 2

/* Image transfer operations */
#define NORMAL 0
#define XRAY 1

/* CPU detection results */
#define CPUOLD     0
#define CPU286     2
#define CPU386     3
#define CPU486     4 
#define CPUPENTIUM 5

/* whollowpoly types */
#define CLOSED_POLY 0
#define OPEN_POLY 1

/* Flip block operations */
#define VERTICAL 0
#define HORIZONTAL 1

/* Shading operations */
#define SHADE_NORMAL 0
#define SHADE_XRAY 1
#define SHADE_SHADOW 2
#define SHADE_TRANSLUCENT 3
#define SHADE_MONO 4

extern struct {
  short xres;
  short yres;
  short videomode;
  long  videobanksize;
  short (*bankswitch)(short);
  short screenwidth;
  short screenheight;
} WGT_SYS;


/* A single palette register definition */
typedef struct 
{
  unsigned char r,g,b;
} color;



/* Polygon point definition */
typedef struct
{
  short x,y;   /* Coordinate on the screen */
  short sx,sy; /* Coordinate on the texture if texture is used,
		  otherwise sx = color point */
	       /* sx is used for Gouraud shading as well (color of the point) */
} tpolypoint;

/* Scan converted polygon routines */
typedef struct 
{
  short **pointbuffer;       /* Holds a buffer of x coordinates for each
				row of the polygon. */
  unsigned short *numpoints; /* Holds the size of the point buffer in
				bytes. */
} wscanpoly;



/* WGTLIB variables */
extern fpos_t lfpos;
extern FILE *libf;
extern short lresult;
extern long  lsize;
extern char  password[16];
extern char  *wgtlibrary;

/* Keyboard Interrupt */
extern short kbdon[128];
extern short keypressed;

/* Mouse variables */
extern int but;
extern int mx;
extern int my;

/* Clipping boundaries */
extern short bx,by,tx,ty;

/* Current drawing color */
extern unsigned char currentcolor;

/* Software cursor flashing speed */
extern short curspeed;

/* Software cursor coordinates */
extern short xc,yc;

/* Pointer to the active drawing page */
extern block abuf;

/* EMS variables */
extern int      wpageframe;
extern int      emshandle;
extern block    emsptr;
extern unsigned emsoffset;
extern int      emspage;
extern long     emscurpage;
extern long     maxemspage;


/* Function Prototypes */
short  checkpassword (char *temp);
void   findfile (char *filename);
void   installkbd (void);
void * lib2buf (char *filename);
void   mdeinit (void);
short  minit (void);
void   moff (void);
void   mon (void);
void   mouseshape (short x_hotspot, short y_hotspot,
		   void far *mousebitmap);
void   msetbounds (short x, short y, short x2, short y2);
void   msetspeed (short x_speed, short y_speed);
void   msetthreshhold (short speed);
void   msetxy (short x, short y);
void   noclick (void);
void   readheader (void);
void   setlib (char *libraryname);
void   setpassword (char *password);
void   uninstallkbd (void);
void   vga256 (void);
char   vgadetected (void);
block  wallocblock (short width, short height);
void   wbar (short x1, short y1, short x2, short y2);
void   wbezier (tpolypoint *rawpts, short numraw, tpolypoint *curvepts,
		short numcurve);
void   wbutt (short x1, short y1, short x2, short y2);
void   wcircle (short x_center, short y_center, short radius);
void   wclip (short x1, short y1, short x2, short y2);
void   wcls (unsigned char col);
void   wcolrotate (unsigned char start, unsigned char finish, short dir,
		   color *pal);
void   wcopyscreen (short x1, short y1, short x2, short y2, block source,
                    short destx, short desty, block destination);
void   wdeinitpoly (void);
int    wdetectcpu (void);
void   wdissolve (block sourceimage, short *pattern, short speed);
void   wdonetimer (void);
void   wdraw_scanpoly (short x, short y, wscanpoly *scanpoly,
		    void (*customline)(short x1, short x2, short y));
void   wellipse (short x_center, short y_center, short x_radius,
                 short y_radius);
void   wems_close (void);
void   wems_copy (block sb, unsigned offset, unsigned size);
long   wems_free (void);
block  wems_getdata (block mydata);
void   wems_getframe (int pagegroup);
block  wems_getsprite (int spr, block *sprites);
int    wems_init (void);
int    wems_loadsprites (color *pal, char *filename, block *sprite,
                         int start, int end);
int    wems_map (int phys, int logi);
int    wems_open (int pages);
int    wems_present (void);
block  wems_storeblock (block spr);
block  wems_storedata (block sb, unsigned size);
void   wems_reset (void);
void   wfade_between (unsigned char start, unsigned char finish, short speed,
		      color *pal1, color *pal2);
void   wfade_between_once (unsigned char start, unsigned char finish,
		      color *pal1, color *pal2);
void   wfade_in (unsigned char start, unsigned char finish, short speed,
		 color *pal);
void   wfade_in_once (unsigned char start, unsigned char finish, 
		      color *pal, color *temppal);
void   wfade_out (unsigned char start, unsigned char finish, short speed,
		  color *pal);
void   wfade_out_once (unsigned char start, unsigned char finish,
		       color *pal);
void   wfastputpixel (short x, short y);
void   wfill_circle (short x, short y, short radius);
void   wfill_ellipse (short x_center, short y_center, short x_radius,
                      short y_radius);
void   wflashcursor (void);
void   wfline (short x1, short y1, short x2, short y2);
void   wflipblock (block image, short direction);
void   wfree_scanpoly (wscanpoly *scanpoly);
void   wfreeblock (block image);
void   wfreefont (wgtfont font);
void   wfreesprites (block *image_array, short start, short end);
short  wget_capslock (void);
unsigned char wget_imagebit (block image);
unsigned char wget_imagebyte (block image);
short  wget_lalt (void);
short  wget_lctrl (void);
short  wget_lshift (void);
short  wget_numlock (void);
short  wget_ralt (void);
short  wget_rctrl (void);
short  wget_rshift (void);
short  wget_scrlock (void);
short  wgetblockheight (block image);
short  wgetblockwidth (block image);
short  wgetmode (void);
unsigned char wgetpixel (short x, short y);
short  wgettextheight (char *string, wgtfont font);
short  wgettextwidth (char *string, wgtfont font);
void   wgouraudpoly (tpolypoint *vertexlist, short numvertex, short x, short y);
void   wgtprintf (short x, short y, wgtfont font, char *fmt, ... );
void   whline (short x1, short x2, short y);
void   whollowpoly (tpolypoint *vertexlist, short numvertex, short x, short y,
		    short closemode);
void   winitpoly (short maxrows);
void   winittimer (void);
void   wline (short x1, short y1, short x2, short y2);
block  wloadblock (char *filename);
block  wloadbmp (char *filename, color *pal);
block  wloadcel (char *filename, color *pal);
wgtfont wloadfont (char *filename);
block  wloadiff (char *filename, color *pal);
block  wloadpak (char *filename);
void   wloadpalette (char *filename, color *pal);
block  wloadpcx (char *filename, color *pal);
short  wloadsprites (color *pal, char *filename, block *image_array,
                     short start, short end);
block  wnewblock (short x1, short y1, short x2, short y2);
void   wnormscreen (void);
short  woutchar (short asc, short xc, short yc, wgtfont pr);
void   wouttextxy (short x, short y, wgtfont font, char *string);
void   wpan (unsigned short pan_offset);
void   wputblock (short x, short y, block image, short method);
void   wputblock_shade (short x, short y, block image, block shadetable,
			int mode);
void   wputpixel (short x, short y);
void   wreadpalette (unsigned char start, unsigned char finish, color *pal);
void   wrectangle (short x1, short y1, short x2, short y2);
void   wregionfill (short x, short y);
void   wremap (color *pal1, block image, color *pal2);
void   wresize (short x1, short y1, short x2, short y2, block image,
		short mode);
void   wresize_column (short x, short y, short y2, block image,
		       short column, short mode);
void   wresize_shade (short x1, short y1, short x2, short y2,
		      block image, unsigned char *shadetable,
		      short mode);
void   wretrace (void);
short  wsaveblock (char *filename, block image);
short  wsavebmp (char *filename, block image, color *pal);
short  wsavecel (char *filename, block image, color *pal);
short  wsavepak (char *filename, block image);
void   wsavepalette (char *filename, color *pal);
short  wsavepcx (char *filename, block image, color *pal);
short  wsavesprites (color *pal, char *filename, block *image_array,
                     short start, short end);
void   wscan_convertpoly (tpolypoint *vertexlist, short numvertex,
		    wscanpoly *scanpoly);
void   wset_capslock (short state);
void   wset_imagebyte (block image, unsigned char status);
void   wset_numlock (short state);
void   wset_scrlock (short state);
void   wsetcolor (unsigned char col);
void   wsetcursor (short start, short end);
void   wsetmode (short mode);
void   wsetpalette (unsigned char start, unsigned char finish, color *pal);
void   wsetrgb (unsigned char num, unsigned char red, unsigned char green, 
		unsigned char blue, color *pal);
void   wsetscreen (block image);
void   wsettimerspeed (int speed);
void   wskew (short x, short y, block image, short degrees);
void   wsline (short x1, short y1, short x2, short y2, short *y_array);
void   wsolidpoly(tpolypoint *vertexlist, short numvertex, short x, short y,
  		  void (*customline)(short x1, short x2, short y));
void   wstarttimer (void (*rout)(), int speed);
void   wstoptimer (void);
void   wstring (short x, short y, char *string, char *legal, short maxlength);
void   wstyleline (short x, short y, short x2, short y2, unsigned short style);
void   wtextbackground (unsigned char col);
void   wtextcolor (unsigned char col);
void   wtextgrid (short state);
void   wtexttransparent (short mode);
void   wtexturedpoly (tpolypoint *vertexlist, short numvertex, short x,
                      short y, block image, short mode);
void   wupdate_imagebytes (block *image_array, long start, long end);
void   wvertres (short x1, short y1, short y2, block image);
void   wwarp (short x1, short x2, short *top, short *bot, block image,
	      short mode);
void   wwipe (short x1, short y1, short x2, short y2, block image);
void   wxorbox (short x, short y, short x2, short y2, unsigned char col);

/* File Selector */
char *wfilesel (char *mask, char *title, short x, short y, block background);
extern wgtfont filefont;

#ifdef __cplusplus
}
#endif

#endif




