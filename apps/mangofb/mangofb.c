/*
 * Copyright (C) 2015-2016 ilbers GmbH
 * Alexander Smirnov <asmirnov@ilbers.de>
 *
 * Based on picoTK
 * Copyright (C) Thomas Gallenkamp <tgkamp@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (version 2) as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define CRTX			640
#define CRTY			480
#define BITS_PER_PIXEL		8
#define PIXELS_PER_BYTE		(8 / BITS_PER_PIXEL)
#define PIXEL_MASK		0xff
#define PIXELS_PER_LONG		(PIXELS_PER_BYTE * 4)

#define CHUNKX			32
#define CHUNKY			20

static Display  *display;
static Colormap colormap;
static GC       gc;
static Window   window, root, parent;
static int      depth, screen, visibility;
static char     *host = NULL;
static void     *crtbuf;

static unsigned int crcs[CRTX / CHUNKX][CRTY / CHUNKY];

static int    repaint;
static Pixmap pixmap;

unsigned int colors_24[256] =
{ 
	/* contains 24 bit (00rrggbb) rgb color mappings */ 
	0x00000000, 0x000000c0, 0x0000c000, 0x0000c0c0,
	0x00c00000, 0x00c000c0, 0x00c0c000, 0x00c0c0c0,
	0x00808080, 0x000000ff, 0x0000ff00, 0x0000ffff,
	0x00ff0000, 0x00ff00ff, 0x00ffff00, 0x00ffffff
};

unsigned int colors_X11[256]; /* contains X11 variants, either 8, 16 or 24 bits */

static void X11_initialize();
static void fbe_loop();

static void fbe_setcolors(void)
{
	int i;

	for (i = 16; i < 255; i++)
	{
		colors_24[i] = i * 0x00010101;
	}
}

static void fbe_calcX11colors(void)
{
	int i;
	unsigned int c24;

	/* Calculate X11 "pixel values" from 24 bit rgb (00rrggbb) color and
	 * store them in colors_X11[] array for fast lookup. Mapping of rgb
	 * colors to pixel values depends on the X11 Server color depth 
	 */
	for (i = 0; i < 256; i++)
	{
		XColor xc;

		c24 = colors_24[i];
		xc.red = ((c24 & 0xff0000) >> 16) * 0x0101;
		xc.green = ((c24 & 0x00ff00) >> 8) * 0x0101;
		xc.blue = (c24 & 0x0000ff) * 0x0101;
		xc.flags = 0;
		XAllocColor(display, colormap, &xc);
		colors_X11[i] = xc.pixel;
	}
}

static void fbe_init(void *crtbuf_)
{

	if (host == NULL)
	{
		host = (char *)getenv("DISPLAY");
		if (host == NULL)
		{
			fprintf(stderr, "%s", "error: no environment variable DISPLAY\n");
			exit(1);
		}
	}

	X11_initialize();

	gc = XCreateGC(display, window, 0, NULL);
	crtbuf=crtbuf_;
	fbe_setcolors();
	fbe_calcX11colors();

	fbe_loop();
}

static void X11_initialize(void)
{
	XSetWindowAttributes attr;
	char name[80];
	Pixmap iconPixmap;
	XWMHints xwmhints;

	display = XOpenDisplay(host);
	if (display == NULL) 
	{
		fprintf(stderr,"error: failed to open display.\n");
		exit(1);
	}

	screen   = DefaultScreen(display);
	colormap = DefaultColormap(display, screen);
	root     = RootWindow(display, screen);
	parent   = root;
	depth    = DefaultDepth(display, screen);

	XSelectInput(display, root, SubstructureNotifyMask);

	attr.event_mask = ExposureMask;

	attr.background_pixel=BlackPixel(display,screen);
   
	window = XCreateWindow(display,
			       root,
			       0,
			       0,
			       CRTX,CRTY,
			       0,
			       depth,
			       InputOutput,
			       DefaultVisual(display,screen),
			       CWEventMask | CWBackPixel,
		               &attr);

	sprintf(name, "MangoFB %d * %d * %d bpp", CRTX, CRTY, BITS_PER_PIXEL);

	XChangeProperty(display,
			window,
			XA_WM_NAME,
			XA_STRING,
			8,
			PropModeReplace,
			name,
			strlen(name));

	XMapWindow(display, window);

	xwmhints.icon_pixmap   = iconPixmap;
	xwmhints.initial_state = NormalState;
	xwmhints.flags         = IconPixmapHint | StateHint;

	XSetWMHints(display, window, &xwmhints);
	XClearWindow(display, window);
	XSync(display, 0);
}

unsigned int calc_patch_crc(int ix, int iy)
{ 
	unsigned int crc;
	int x, y;
	int off;
 
	off = (ix * CHUNKX) / PIXELS_PER_LONG + iy * CHUNKY * (CRTX / PIXELS_PER_LONG);
	crc=0x8154711;

	for (x = 0; x < CHUNKX / PIXELS_PER_LONG; x++) 
	{
		for (y = 0; y < CHUNKY; y++)
		{
			unsigned int dat;

			dat = ((unsigned int *)crtbuf)[off + x + (y * CRTX / PIXELS_PER_LONG)];
			crc += (crc % 211 + dat);
		}
	}

	return crc;
}

void check_and_paint(int ix, int iy)
{
	unsigned int crc;
	int x, y;
	int off, color;
 
	crc = calc_patch_crc(ix, iy);

	if (!repaint && (crc == crcs[ix][iy]))
	{
		return;
	}

	off = ix * (CHUNKX / PIXELS_PER_BYTE) + iy * CHUNKY * (CRTX / PIXELS_PER_BYTE);

	XSetForeground(display, gc, 0x000000);
	XFillRectangle(display, pixmap, gc, 0, 0, CHUNKX, CHUNKY);

	for (y = 0; y < CHUNKY; y++)
	{
		for (x=0; x<CHUNKX; x++)
		{
			unsigned char data;

			data = ((unsigned char *)crtbuf)[off + (x / PIXELS_PER_BYTE) + y * (CRTX / PIXELS_PER_BYTE)];
			color = (data >> ((PIXELS_PER_BYTE - 1) - (x & (PIXELS_PER_BYTE - 1))) * BITS_PER_PIXEL) & PIXEL_MASK;

			XSetForeground(display,gc,colors_X11[color]);
			XDrawPoint(display, pixmap, gc, x, y);   
		}
	}

	XCopyArea(display, pixmap, window, gc, 0, 0, CHUNKX, CHUNKY, ix * CHUNKX, iy * CHUNKY);
	crcs[ix][iy] = crc; 
}
         
static void fbe_loop(void)
{
	int i = 0;
	int x_position, y_position;
	static count = 0;

	pixmap = XCreatePixmap(display, window, CHUNKX, CHUNKY, depth);
	repaint = 0;

	while (1)
	{
		int x, y;

		repaint = 0;

		/* Check if to force complete repaint because of window expose event */
		while (XPending(display) > 0)
		{
			XEvent event;
			XNextEvent(display, &event);

			if (event.type == Expose)
			{
				repaint = 1;
			}
		}
    
		/* Sample all chunks for changes in shared memory buffer and
		 * eventually repaint individual chunks. Repaint everything if
		 * repaint is true (see above)
		 */
		for (y = 0; y < CRTY / CHUNKY; y++)
		{
			for (x = 0; x < CRTX / CHUNKX; x++)
			{
				check_and_paint(x, y);
			}

		}

		usleep(1000);
	}
}
 
int main(int argc, char **argv)
{
	int fd;
	void *fbuf;

	fd = open("/dev/mem", O_RDWR);
	if (fd < 0)
	{
		printf("Error: failed to open /dev/mem\n");
		return -1;
	}

	fbuf = mmap(NULL, (640 * 480), PROT_READ | PROT_WRITE, MAP_SHARED,fd,0x20000000);
	if (fbuf == MAP_FAILED)
	{
		printf("Error: failed to map FB memory\n");
		return -1;
	}

	fbe_init(fbuf);

	return 0;
}
