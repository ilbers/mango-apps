/*
 * Partition watchdog daemon for Linux.
 *
 * Copyright (c) 2014-2015 ilbers GmbH
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
#include <fcntl.h>
#include <unistd.h>

/* This is a simple watchdog daemon for Linux. It triggers hypervisor
 * partition timer every 10 seconds. Please refere to getting started
 * guide for more information about Mango watchdog settings.
 */

int wd_loop(void)
{
	int dev;

	dev = open("/dev/wd", O_WRONLY);
	if (dev < 0)
		return dev;

	for (;;) {
		/* Ping watchdog */
		write(dev, "ping", 5);

		sleep(10);
	}
}

int main(int argc, char** argv)
{
	int status;
	int pid;

	pid = fork();

	if (pid == -1)
	{
		printf("error: failed to start daemon\n");
		return -1;
	}
	else if (!pid) /* child */
	{
		umask(0);
		setsid();
		chdir("/");

		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);

		status = wd_loop();

		return status;
	}
	else /* parent */
	{
		return 0;
	}
}
