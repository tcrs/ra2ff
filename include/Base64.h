/*
 * Part of the Red Alert 2 File Format Tools.
 * Copyright (C) 2008 Thomas Spurden <thomasspurden@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#ifndef BASE64_H__
#define BASE64_H__

#include <stdint.h>
#include <string.h>
#include "INIFile.h"

class Base64 {
protected:
	static uint8_t alphabet[64];
	static uint8_t lookup[256];

	static unsigned int decode64Chunk(uint8_t const*, uint8_t*);
	static void initialiseTable();
	Base64() { }
	~Base64() { }
public:
	static size_t decode(uint8_t* inout, size_t len) {
		return decode(inout, inout, len);
	}
	static size_t decode(uint8_t const*, uint8_t*, size_t);
	static uint8_t* decode(INIFile&, size_t&);
};

#endif
