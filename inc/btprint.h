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
#ifndef BT_PRINT_HEADER_H
#define BT_PRINT_HEADER_H

#include <stdarg.h>

#ifndef MODULE
#error "please defined MODULE before #include "btprint.h""
#endif


void btprint(const char* format, ...);

#define BTPRINT(format, ...) 		btprint("%s " format, MODULE, ##__VA_ARGS__)

#define INFO(format, ...) 			BTPRINT("%s " format, "[I]", ##__VA_ARGS__)
#define ERR(format, ...) 			BTPRINT("%s " format, "[E]", ##__VA_ARGS__)
#define BT_PR_WARNING(format, ...) 	BTPRINT("%s " format, "[W]", ##__VA_ARGS__)
#define DBG(format, ...) 			BTPRINT("%s " format, "[D]", ##__VA_ARGS__)
#endif /* BT_PRINT_HEADER_H */
