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
#ifndef BTLE_ERROR_H
#define BTLE_ERROR_H

enum btle_error {
	BTLE_SUCCESS = 0,
	BTLE_ERROR_INVALID_STATE,
	BTLE_ERROR_INVALID_ARG,
	BTLE_ERROR_NULL_ARG,
	BTLE_ERROR_TIMEOUT,
	BTLE_ERROR_BUSY,
	BTLE_ERROR_ALREADY,
	BTLE_ERROR_INTERNAL,
	BTLE_ERROR_EMPTY,
	BTLE_ERROR_NOT_IMPLEMENTED,
};

#endif /* BTLE_ERROR_H */
