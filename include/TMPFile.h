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

#ifndef TMPFILE_H__
#define TMPFILE_H__

#include <stdint.h>
#include <string>
#include "Utils.h"

class TMPFile {
public:
	static uint32_t const ra2TileWidth;
	static uint32_t const ra2TileHeight;
	struct Header {
		uint32_t tilesX;
		uint32_t tilesY;
		uint32_t tileWidth;
		uint32_t tileHeight;
		uint32_t* offset;
		Header() : offset(NULL) { }
		~Header() { delete[] offset; }
		void alloc() { offset = new uint32_t[tilesX * tilesY]; }
	};

	struct TileHeader {
		static uint32_t const hasExtraData = 1;
		static uint32_t const hasZData = 1 << 1;
		static uint32_t const hasDamagedData = 1 << 2;

		int32_t x;
		int32_t y;
		uint32_t extraOffset;
		uint32_t zOffset;
		uint32_t extraZOffset;
		int32_t extraX;
		int32_t extraY;
		uint32_t extraW;
		uint32_t extraH;

		uint32_t flags;

		uint8_t height;
		uint8_t terrainType;
		uint8_t rampType;
		
		uint8_t radarLeftColour[3];
		uint8_t radarRightColour[3];
		
		uint8_t pad[3];

		bool hasExtra() {
			return flags & hasExtraData;
		}
		int32_t getX() {
			return x;
		}
		int32_t getExtraX() {
			return extraX;
		}
		int32_t getY(uint8_t maxH) {
			return y + ((static_cast<int32_t>(ra2TileHeight) / 2) * (maxH - height));
		}
		int32_t getExtraY(uint8_t maxH) {
			return extraY + ((static_cast<int32_t>(ra2TileHeight) / 2) * (maxH - height));
		}
	};

	struct TileData {
		uint8_t* tile;
		uint8_t* height;
		uint8_t* extra;

		TileData() : tile(NULL), height(NULL), extra(NULL) { }
		~TileData() {
			delete[] tile;
			delete[] height;
			delete[] extra;
		}
		void alloc(size_t extraSz = 0) {
			tile = new uint8_t[ra2TileWidth * ra2TileHeight];
			height = new uint8_t[ra2TileWidth * ra2TileHeight];
			if(extraSz) {
				extra = new uint8_t[extraSz];
			}
		}
	};
protected:
	Header header;
	TileHeader* tileHeader;
	TileData* tileData;
	uint32_t currentTile;

	void readTile(uint32_t, Utils::FixedRead&);
	void readIsoToSqr(uint8_t*, Utils::FixedRead&);
public:
	TMPFile(std::string const&);
	~TMPFile();

	uint32_t numTiles();

	void setCurrentTile(unsigned int, unsigned int);
	void setCurrentTile(uint32_t);

	void getTileSize(uint32_t&, uint32_t&);

	uint8_t getPixel(unsigned int, unsigned int);
	uint8_t getExtraPixel(unsigned int, unsigned int);
	uint8_t getHeight(unsigned int, unsigned int);

	void getTotalSize(uint32_t&, uint32_t&);
	void getTemplate(uint8_t*, size_t, size_t, bool = true, bool = true);
	void getHeightTemplate(uint8_t*, size_t, size_t);
	void getBounds(int32_t&, int32_t&, int32_t&, int32_t&);
	void getTileBounds(int32_t&, int32_t&, int32_t&, int32_t&);
	void getExtraBounds(int32_t&, int32_t&, int32_t&, int32_t&);
	uint8_t getMaxHeight();

	void print();
};

#endif
