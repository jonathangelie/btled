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
#ifndef GATTC_HEADER_H
#define GATTC_HEADER_H

uint8_t gattc_write_req(uint8_t devId, uint8_t *data, uint8_t data_len);
uint8_t gattc_write_cmd(uint8_t devId, uint8_t *data, uint8_t data_len);
uint8_t gattc_read_req(uint8_t devId, uint8_t *data, uint8_t data_len);
uint8_t gattc_connect(uint8_t devId, uint8_t *data, uint8_t data_len);
#endif /* GATTC_HEADER_H */
