/*
 *  Copyright (C) 2018  Jonathan Gelie <contact@jonathangelie.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "src/shared/mainloop.h"

#define MODULE "main"
#include "btprint.h"

#include "btle_error.h"
#include "cmd.h"

#define CHK_RETURN(condition) {	\
		if (condition) { \
			return condition; \
		} \
}

static void cleaning(void *user_data)
{
	cmd_server_close();
}

static void signal_callback(int signum, void *user_data)
{
	switch (signum) {
	case SIGINT:
	case SIGTERM:
		INFO("Bluetooth Low Energy daemon exit\n");
		cleaning(NULL);
		mainloop_quit();
		break;
	}
}

int main(void)
{
	int ret;
	sigset_t mask;

	INFO("Starting Bluetooth Low Energy daemon\n");

	mainloop_init();

	ret = cmd_server_init();
	CHK_RETURN(ret)

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);

	mainloop_set_signal(&mask, signal_callback, NULL, NULL);

	ret = mainloop_run();

	cmd_server_close();

	return 0;
}
