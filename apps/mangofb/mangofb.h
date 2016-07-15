/*
 * Copyright (C) 2015-2016 ilbers GmbH
 * Alexander Smirnov <asmirnov@ilbers.de>
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

#ifndef __MANGOFB_H__
#define __MANGOFB_H__

#define MANGOFB_BASE_ADDR	0x20000000
#define MANGO_FB_MAGIC		0x11072016

#define PICO_WG_ALLOCATED	(1 << 0)

struct picoWidget {
	int flags;				/* Flags to indicate widget state */
	int x;					/* Top left corner X pos */
	int y;					/* Top left corner Y pos */
	int width;				/* Width of widget in pixels */
	int height;				/* Height of widget in pixels */
	int rcount;				/* Widget refresh counter */
};

struct picoControlBlock {
	int               magic;		/* MangoFB magic signature */
	int               screen_x;		/* Screen X resolution */
	int               screen_y;		/* Screen Y resolution */
	int               colors;		/* Bits per pixel */
	int               data_offset;		/* Raw FB data offset */
	int               max_nr_widgets;	/* Max nr widgets supported */
	int               nr_widgets;		/* Nr widgets allocated */
	struct picoWidget widgets[1];		/* Widgets data */
};

#endif /* __MANGOFB_H__ */
