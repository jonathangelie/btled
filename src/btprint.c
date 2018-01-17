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
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "btle_error.h"

#define DBG_MSG_LEN	80

void btprint(const char* format, ...)
{
	va_list args;
	uint8_t data[DBG_MSG_LEN] = {0};
	uint8_t *p_data = &data[0];
	time_t t = time(NULL);
	struct tm *tm;

	tm = localtime(&t);

	va_start(args, format);
	p_data += snprintf((char *)p_data, sizeof(data) -1, "%02d:%02d:%02d ",
						tm->tm_hour, tm->tm_min, tm->tm_sec);

	vsnprintf((char *)p_data, &data[sizeof(data) -1] - p_data, format, args);

	fprintf(stdout,"%s", data);
	fflush (stdout);

	va_end(args);
}
