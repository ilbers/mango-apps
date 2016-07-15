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

#include "mangofb.h"

#define PIXEL_MASK		0xff

static Display  *display;
static Colormap colormap;
static GC       gc;
static Window   window, root, parent;
static int      depth, screen, visibility;
static char     *host = NULL;
static void     *crtbuf;

static int xres = 0;	/* Screen X resolution */
static int yres = 0;	/* Screen Y resolution */
static int bpp = 0;	/* Bits-Per-Pixel */
static int ppb = 0;	/* Pixels-Per-Byte */

static struct picoControlBlock *cb;

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

static void fbe_init(void)
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

	xres = cb->screen_x;
	yres = cb->screen_y;
	bpp = cb->colors;
	ppb = 8 / bpp;

	X11_initialize();

	gc = XCreateGC(display, window, 0, NULL);
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
			       xres, yres,
			       0,
			       depth,
			       InputOutput,
			       DefaultVisual(display,screen),
			       CWEventMask | CWBackPixel,
		               &attr);

	sprintf(name, "MangoFB %d * %d * %d bpp", xres, yres, bpp);

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

void repaint(int x, int y, int w, int h)
{
	unsigned int crc;
	int ix, iy;
	int off, color;
 
	XSetForeground(display, gc, 0x000000);
	XFillRectangle(display, pixmap, gc, 0, 0, w, h);

	off = x + (y * xres);

	for (iy = 0; iy < h; iy++)
	{
		for (ix = 0; ix < w; ix++)
		{
			unsigned char data;
			data = ((unsigned char *)crtbuf)[off + ix + iy * xres];
			color = (data >> ((ppb - 1) - (ix & (ppb - 1))) * bpp) & PIXEL_MASK;

			XSetForeground(display, gc, colors_X11[color]);
			XDrawPoint(display, pixmap, gc, ix, iy);   
		}
	}

	XCopyArea(display, pixmap, window, gc, 0, 0, w, h, x, y);
}
         
static void fbe_loop(void)
{
	int i;
	int widgets_cnt[cb->max_nr_widgets];

	pixmap = XCreatePixmap(display, window, xres, yres, depth);

	for (i = 0; i < cb->max_nr_widgets; i++)
	{
		widgets_cnt[i] = 0;
	}

	while (1)
	{
		int n = cb->nr_widgets;

		/* Check if to force complete repaint because of window expose event */
		while (XPending(display) > 0)
		{
			XEvent event;
			XNextEvent(display, &event);

			if (event.type == Expose)
			{
				repaint(0, 0, xres, yres);
			}
		}
    
		for (i = 0; i < cb->max_nr_widgets; i++)
		{
			struct picoWidget *wg = &cb->widgets[i];

			if (wg->flags & PICO_WG_ALLOCATED)
			{
				if (wg->rcount != widgets_cnt[i])
				{
					repaint(wg->x, wg->y, wg->width, wg->height);
					widgets_cnt[i] = wg->rcount;
				}

				if (--n == 0)
				{
					/* No more widgets remain */
					break;
				}
			}
		}
	}
}
 
static void print_mangofb_info(void)
{
	printf("MangoFB Control Block information:\n");
	printf("  Revision: 0x%08x\n", cb->magic);
	printf("  Screen X: %d\n", cb->screen_x);
	printf("  Screen Y: %d\n", cb->screen_y);
	printf("  Colors  : %d bits\n", cb->colors);
	printf("  Data    : 0x%08x\n", cb->data_offset);
	printf("  Widgets : %d\n", cb->max_nr_widgets);
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

	/* Map 2MB window */
	cb = mmap(NULL, 0x200000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, MANGOFB_BASE_ADDR);
	if (cb == MAP_FAILED)
	{
		printf("Error: failed to map FB memory\n");
		return -1;
	}

	if (cb->magic != MANGO_FB_MAGIC)
	{
		printf("error: invalid MangoFB magic, device not ready\n");
		return -1;
	}

	/* Initialize display buffer */
	crtbuf = (void *)((char *)cb + cb->data_offset);

	print_mangofb_info();

	fbe_init();

	return 0;
}
