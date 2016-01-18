/*
 * Data-channel echo client.
 *
 * Copyright (c) 2014-2016 ilbers GmbH
 * Alexander Smirnov <asmirnov@ilbers.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>

int main()
{
	int dev;
	char *msg = "Hello LotOS!";
	char buf[16];

	printf("demoCORE 'hello_dc' application client\n");

	dev = open("/dev/dc0", O_RDWR);
	if (dev < 0) {
		printf("error: failed to open device\n");
		return 1;
	}

	printf("Send request: (%s)\n", msg);

	write(dev, msg, 13);

	/* Invalidate the buffer to keep experiment clean */
	memset(buf, 0, 16);

	read(dev, buf, 13);

	printf("Got reply: (%s)\n", buf);

	close(dev);

	return 0;
}

