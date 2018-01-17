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

#include <sys/signalfd.h>
#include <glib.h>

#define MODULE "main"
#include "btprint.h"

#include "btle_error.h"
#include "cmd.h"

#define CHK_RETURN(condition) {\
	if (condition) { \
		return condition; \
	}\
}

static GMainLoop *main_loop;

static gboolean signal_handler(GIOChannel *channel, GIOCondition condition,
							gpointer user_data)
{
	struct signalfd_siginfo si;
	ssize_t result;
	int fd;

	if (condition & (G_IO_NVAL | G_IO_ERR | G_IO_HUP)) {
		g_main_loop_quit(main_loop);
		return FALSE;
	}

	fd = g_io_channel_unix_get_fd(channel);

	result = read(fd, &si, sizeof(si));
	if (result != sizeof(si))
		return FALSE;

	switch (si.ssi_signo) {
	case SIGINT:
	case SIGTERM:
			INFO("Bluetooth Low Energy daemon exit\n");
			cmd_server_close();
			g_main_loop_quit(main_loop);
		break;
	}

	return TRUE;
}

static guint setup_signal(void)
{
	GIOChannel *channel;
	guint source;
	sigset_t mask;
	int desc;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
		ERR("Setting signal mask failed");
		return 0;
	}

	desc = signalfd(-1, &mask, 0);
	if (desc < 0) {
		ERR("Creating signal descriptor failed");
		return 0;
	}

	channel = g_io_channel_unix_new(desc);

	g_io_channel_set_close_on_unref(channel, TRUE);
	g_io_channel_set_encoding(channel, NULL, NULL);
	g_io_channel_set_buffered(channel, FALSE);

	source = g_io_add_watch(channel,
				G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
				signal_handler, NULL);

	g_io_channel_unref(channel);

	return source;
}

int main(void)
{
	int ret;
	guint signal;

	INFO("Starting Bluetooth Low Energy daemon\n");

	main_loop = g_main_loop_new(NULL, FALSE);

	signal = setup_signal();

	ret = cmd_server_init();
	CHK_RETURN(ret)

	g_main_loop_run(main_loop);
	g_source_remove(signal);
	cmd_server_close();

	return 0;
}
