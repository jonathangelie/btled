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
#ifndef BTSOCKET_HEADER_H
#define BTSOCKET_HEADER_H

#define SOCKET_MTU	128

typedef void (*socket_rx_notifier)(uint8_t *data, uint8_t data_len);

struct btsocket_param {
	uint16_t mtu;
	socket_rx_notifier rx_cb;
};
uint8_t btsocket_init(struct btsocket_param *param);
uint8_t btsocket_send(uint8_t *data, uint8_t data_len);
void btsocket_close();

#endif /* BTSOCKET_HEADER_H */
