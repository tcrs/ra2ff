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

#include "LZODecompress.h"
#include "Exception.h"

extern "C" {
#	include "minilzo.h"
}

char const * LZODecompress::errorText[] = {
	"LZO_E_OK",
	"LZO_E_ERROR",
	"LZO_E_OUT_OF_MEMORY",
	"LZO_E_NOT_COMPRESSIBLE",
	"LZO_E_INPUT_OVERRUN",
	"LZO_E_OUTPUT_OVERRUN",
	"LZO_E_LOOKBEHIND_OVERRUN",
	"LZO_E_EOF_NOT_FOUND",
	"LZO_E_INPUT_NOT_CONSUMED",
	"LZO_E_NOT_YET_IMPLEMENTED",
};

void LZODecompress::initLZO() {
	static bool initialised = false;
	if(!initialised) {
		if(lzo_init() != LZO_E_OK) {
			throw EXCEPTION("Could not initialised LZO library");
		}
		initialised = true;
	}
}

size_t LZODecompress::decompress(uint8_t const* in, uint8_t* out, size_t inlen, size_t outlen) {
	initLZO();
	size_t len;
	int r = lzo1x_decompress_safe(in, inlen, out, &len, NULL);
	if(r == LZO_E_INPUT_NOT_CONSUMED) {
		DEBUG("Not all input data consumed when decompressing");
	} else if(r != LZO_E_OK) {
		if(r == LZO_E_OUTPUT_OVERRUN) {
			throw EXCEPTION("Overran output buffer when LZO decompressing data");
		} else {
			throw EXCEPTION("LZO Error %i (%s) occured", r, r < 0 && r > -10 ? errorText[-r] : "Unknown");
		}
	}
	return len;
}
