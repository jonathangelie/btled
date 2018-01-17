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
#ifndef BTLOG_HEADER_H
#define BTLOG_HEADER_H


#include <stdarg.h>

#ifndef MODULE
#error "please defined MODULE before #include "bt_log.h""
#endif


uint8_t bt_log_init(void);
uint8_t bt_log_close(void);

void bt_log(const char* format, ...);

#define BT_LOG(format, ...) 		bt_log("%s " format, MODULE, ##__VA_ARGS__)

#define BT_LOG_INFO(format, ...) 	BT_LOG("%s " format, "[I]", ##__VA_ARGS__)
#define BT_LOG_ERR(format, ...) 	BT_LOG("%s " format, "[E]", ##__VA_ARGS__)
#define BT_LOG_WARNING(format, ...) BT_LOG("%s " format, "[W]", ##__VA_ARGS__)
#define BT_LOG_DBG(format, ...) 	BT_LOG("%s " format, "[D]", ##__VA_ARGS__)

#endif /* BTLOG_HEADER_H */
