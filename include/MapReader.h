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

#ifndef MAPREADER_H__
#define MAPREADER_H__

#include "Palette.h"
#include <vector>

class MapReader {
public:
	static int const LZOPack;
	static int const F80Pack;
	struct Pack {
		uint16_t packedLen;
		uint16_t unpackedLen;
		uint8_t* unpacked;
		Pack() : unpacked(NULL) { }
		~Pack() { delete[] unpacked; }
		void alloc() {
			unpacked = new uint8_t[unpackedLen];
		}
	};
	typedef std::vector<Pack*> PackVec;
	PackVec packs;
	struct Entry {
		int16_t x;
		int16_t y;
		int16_t tile;
		int8_t zero1[2];
		int8_t subTile;
		int8_t z;
		int8_t zero2;
	};
	uint32_t numEntries;
	Entry* entry;
protected:
	struct PackSectionInfo {
		uint16_t packedLen;
		uint16_t unpackedLen;
		uint8_t const* packedData;
	};
public:	
	static size_t decode4(uint16_t*, size_t, uint8_t*, size_t, Palette&);
	static size_t decode80(uint8_t const*, uint8_t*, size_t, size_t);
	static uint8_t* unpack(uint8_t const*, size_t, size_t&, int = LZOPack);

	MapReader() : entry(NULL) { }
	~MapReader() { delete[] entry; }
	void readIsoMapPack(uint8_t*, size_t);
	void print();
};

#endif
