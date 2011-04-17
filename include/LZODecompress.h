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

#ifndef LZODECOMPRESS_H__
#define LZODECOMPRESS_H__

#include <string.h>
#include <stdint.h>

class LZODecompress {
protected:
	static char const * errorText[];
	static void initLZO();
	LZODecompress() { }
	~LZODecompress() { }
public:
	static size_t decompress(uint8_t const* in, uint8_t* out, size_t inlen, size_t outlen);
};

#endif
