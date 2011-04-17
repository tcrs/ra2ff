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

#include "TMPFile.h"
#include "Utils.h"
#include "Exception.h"
#include <stdio.h>

uint32_t const TMPFile::ra2TileWidth = 60;
uint32_t const TMPFile::ra2TileHeight = 30;

TMPFile::TMPFile(std::string const& file) : tileHeader(NULL), tileData(NULL), currentTile(0) {
	FILE* f = fopen(file.c_str(), "rb");
	if(f == NULL) {
		throw EXCEPTION("Could not open \"%s\" (%s)", file.c_str(), strerror(errno));
	}
	Utils::FixedRead fixed(f);

	fixed.read(&header.tilesX);
	/* Check if the first 16bits of the file are zero (if so, then it is likely a SHP file */
	if((header.tilesX & 0x0000FFFF) == 0) {
		throw EXCEPTION("This looks like a SHP file");
	}
	fixed.read(&header.tilesY);
	fixed.read(&header.tileWidth);
	fixed.read(&header.tileHeight);
	if(header.tileWidth != ra2TileWidth || header.tileHeight != ra2TileHeight) {
		throw EXCEPTION("Incompatible tile size [%u %u], require [%u %u]",
			header.tileWidth, header.tileHeight,
			ra2TileWidth, ra2TileHeight
		);
	}

	if(header.tilesX == 0 || header.tilesY == 0) {
		throw EXCEPTION("TMPFile contains no tiles (tilesX == %u, tilesY == %u)", header.tilesX, header.tilesY);
	}
	header.alloc();

	fixed.read(header.offset, header.tilesX * header.tilesY);
	tileHeader = new TileHeader[header.tilesX * header.tilesY];
	tileData = new TileData[header.tilesX * header.tilesY];

	for(uint32_t i = 0; i != header.tilesX * header.tilesY; i++) {
		if(header.offset[i]) {
			readTile(i, fixed);
		}
	}
}

TMPFile::~TMPFile() {
	delete[] tileHeader;
	delete[] tileData;
}

void TMPFile::readTile(uint32_t n, Utils::FixedRead& fixed) {
	fixed.seek(header.offset[n]);

	fixed.read(&tileHeader[n].x);
	fixed.read(&tileHeader[n].y);
	fixed.read(&tileHeader[n].extraOffset);
	fixed.read(&tileHeader[n].zOffset);
	fixed.read(&tileHeader[n].extraOffset);
	fixed.read(&tileHeader[n].extraX);
	fixed.read(&tileHeader[n].extraY);
	fixed.read(&tileHeader[n].extraW);
	fixed.read(&tileHeader[n].extraH);
	fixed.read(&tileHeader[n].flags);
	fixed.read(&tileHeader[n].height);
	fixed.read(&tileHeader[n].terrainType);
	fixed.read(&tileHeader[n].rampType);
	fixed.read(tileHeader[n].radarLeftColour, 3);
	fixed.read(tileHeader[n].radarRightColour, 3);
	fixed.read(tileHeader[n].pad, 3);

	if(tileHeader[n].hasExtra()) {
		tileData[n].alloc(tileHeader[n].extraW * tileHeader[n].extraH);
	} else {
		tileData[n].alloc();
	}
	readIsoToSqr(tileData[n].tile, fixed);
	readIsoToSqr(tileData[n].height, fixed);
	if(tileHeader[n].hasExtra()) {
		fixed.read(tileData[n].extra, tileHeader[n].extraW * tileHeader[n].extraH);
	}
}

void TMPFile::readIsoToSqr(uint8_t* img, Utils::FixedRead& fixed) {
	unsigned int width = 4;
	int widthInc = 4;
	for(unsigned int y = 0; y != 29; y++) {
		img += (ra2TileWidth - width) / 2;
		fixed.read(img, width);
		img += width + ((ra2TileWidth - width) / 2);
		width += widthInc;
		if(width == ra2TileWidth) {
			widthInc = -4;
		}
	}
}

uint32_t TMPFile::numTiles() {
	return header.tilesX * header.tilesY;
}

void TMPFile::setCurrentTile(unsigned int x, unsigned int y) {
	setCurrentTile((y * header.tilesX) + x);
}

void TMPFile::setCurrentTile(uint32_t n) {
	if(n >= (header.tilesX * header.tilesY)) {
		throw EXCEPTION("Tile %u is out of range [%u %u] tiles", n, header.tilesX, header.tilesY);
	}
	currentTile = n;
}

uint8_t TMPFile::getPixel(unsigned int x, unsigned int y) {
	if(x >= ra2TileWidth || y >= ra2TileHeight) {
		throw EXCEPTION("Pixel (%u, %u) is out of range (tile size is [%u %u])",
			x, y, ra2TileWidth, ra2TileHeight
		);
	}
	if(header.offset[currentTile]) {
		return tileData[currentTile].tile[(y * ra2TileWidth) + x];
	}
	return 0;
}

uint8_t TMPFile::getExtraPixel(unsigned int x, unsigned int y) {
	if(!tileHeader[currentTile].hasExtra()) {
		throw EXCEPTION("Tile does not have extra data");
	}
	if(x > tileHeader[currentTile].extraW) {
		throw EXCEPTION("X coord %u out of bounds, extra is %u wide", x, tileHeader[currentTile].extraW);
	}
	if(y > tileHeader[currentTile].extraH) {
		throw EXCEPTION("Y coord %u out of bounds, extra is %u heigh", x, tileHeader[currentTile].extraH);
	}
	if(header.offset[currentTile]) {
		/*if(x == 0 || y == 0 || x == imgHeader[currentTile].extraW - 1 || y == imgHeader[currentTile].extraH - 1) {
			return 0x04;
		}*/
		return tileData[currentTile].extra[(y * tileHeader[currentTile].extraW) + x];
	} else {
		return 0;
	}
}

uint8_t TMPFile::getHeight(unsigned int x, unsigned int y) {
	if(x >= ra2TileWidth || y >= ra2TileHeight) {
		throw EXCEPTION("Hexel (%u, %u) is out of range (tile size is [%u %u])",
			x, y, ra2TileWidth, ra2TileHeight
		);
	}
	if(header.offset[currentTile]) {
		return tileData[currentTile].height[(y * ra2TileWidth) + x];
	}
	return 0;
}

void TMPFile::getTileSize(uint32_t& x, uint32_t& y) {
	x = ra2TileWidth;
	y = ra2TileHeight;
}

void TMPFile::getTotalSize(uint32_t& x, uint32_t& y) {
	/*x = ra2TileWidth * header.tilesX;
	y = ra2TileHeight * header.tilesY;*/
	int32_t bounds[4];
	getBounds(bounds[0], bounds[1], bounds[2], bounds[3]);
	x = bounds[2] - bounds[0];
	y = bounds[3] - bounds[1];
}

void TMPFile::getTileBounds(int32_t& minX, int32_t& minY, int32_t& maxX, int32_t& maxY) {
	bool first = true;
	uint8_t maxHeight = getMaxHeight();
	for(uint32_t i = 0; i != header.tilesX * header.tilesY; i++) {
		if(header.offset[i] == 0) {
			continue;
		}
		if(first || tileHeader[i].x < minX) {
			minX = tileHeader[i].x;
		}
		if(first || tileHeader[i].getY(maxHeight) < minY) {
			minY = tileHeader[i].getY(maxHeight);
		}
		if(first || tileHeader[i].x + static_cast<int32_t>(ra2TileWidth) > maxX) {
			maxX = tileHeader[i].x + static_cast<int32_t>(ra2TileWidth);
		}
		if(first || tileHeader[i].getY(maxHeight) + static_cast<int32_t>(ra2TileHeight) > maxY) {
			maxY = tileHeader[i].getY(maxHeight) + static_cast<int32_t>(ra2TileHeight);
		}
		first = false;
	}
}

void TMPFile::getExtraBounds(int32_t& minX, int32_t& minY, int32_t& maxX, int32_t& maxY) {
	bool first = true;
	uint8_t maxH = getMaxHeight();
	for(uint32_t i = 0; i != header.tilesX * header.tilesY; i++) {
		if(header.offset[i] == 0 || !tileHeader[i].hasExtra()) {
			continue;
		}
		if(first || tileHeader[i].getExtraX() < minX) {
			minX = tileHeader[i].getExtraX();
		}
		if(first || tileHeader[i].getExtraY(maxH) < minY) {
			minY = tileHeader[i].getExtraY(maxH);
		}
		if(first || tileHeader[i].getExtraX() + static_cast<int32_t>(tileHeader[i].extraW) > maxX) {
			maxX = tileHeader[i].getExtraX() + static_cast<int32_t>(tileHeader[i].extraW);
		}
		if(first || tileHeader[i].getExtraY(maxH) + static_cast<int32_t>(tileHeader[i].extraH) > maxY) {
			maxY = tileHeader[i].getExtraY(maxH) + static_cast<int32_t>(tileHeader[i].extraH);
		}
		first = false;
	}
	if(first) {
		minX = minY = maxX = maxY = 0;
	}
}

uint8_t TMPFile::getMaxHeight() {
	bool first = true;
	uint8_t maxH;
	for(uint32_t i = 0; i != header.tilesX * header.tilesY; i++) {
		if(header.offset[i] == 0) {
			continue;
		}
		if(first || tileHeader[i].height > maxH) {
			maxH = tileHeader[i].height;
		}
		first = false;
	}
	return maxH;
}

void TMPFile::getBounds(int32_t& minX, int32_t& minY, int32_t& maxX, int32_t& maxY) {
	int32_t tileBounds[4], extraBounds[4];
	getTileBounds(tileBounds[0], tileBounds[1], tileBounds[2], tileBounds[3]);
	getExtraBounds(extraBounds[0], extraBounds[1], extraBounds[2], extraBounds[3]);
	if(tileBounds[0] < extraBounds[0]) {
		minX = tileBounds[0];
	} else {
		minX = extraBounds[0];
	}
	if(tileBounds[1] < extraBounds[1]) {
		minY = tileBounds[1];
	} else {
		minY = extraBounds[1];
	}
	if(tileBounds[2] > extraBounds[2]) {
		maxX = tileBounds[2];
	} else {
		maxX = extraBounds[2];
	}
	if(tileBounds[3] > extraBounds[3]) {
		maxY = tileBounds[3];
	} else {
		maxY = extraBounds[3];
	}
}

void TMPFile::getTemplate(uint8_t* img, size_t scanWidth, size_t numScanLines, bool drawTiles, bool drawExtras) {
	uint8_t px;
	uint32_t tx, ty;
	int32_t tileBounds[4];
	int32_t extraBounds[4];
	getTotalSize(tx, ty);
	getBounds(extraBounds[0], extraBounds[1], extraBounds[2], extraBounds[3]);
	getBounds(tileBounds[0], tileBounds[1], tileBounds[2], tileBounds[3]);
	uint8_t maxHeight = getMaxHeight();
	/*EDEBUG("tx == %u, ty == %u, \
		tile bounding box (%i, %i) -> (%i, %i) \
		extra bounding box (%i, %i) -> (%i, %i)",
		tx, ty,
		tileBounds[0], tileBounds[1], tileBounds[2], tileBounds[3],
		extraBounds[0], extraBounds[1], extraBounds[2], extraBounds[3]
	);*/
	int32_t tileX, tileY;
	for(uint32_t i = 0; i != header.tilesX * header.tilesY; i++) {
		if(header.offset[i] == 0) {
			continue;
		}
		setCurrentTile(i);
		tileX = tileHeader[i].getX() - tileBounds[0];
		tileY = tileHeader[i].getY(maxHeight) - tileBounds[1];
		if(tileX < 0 || tileY < 0) {
			throw EXCEPTION("Tile out of range (%i, %i)", tileX, tileY);
		}
		if(tileX + static_cast<int32_t>(ra2TileWidth) > static_cast<ptrdiff_t>(scanWidth) ||
				tileY + static_cast<int32_t>(ra2TileHeight) > static_cast<ptrdiff_t>(numScanLines)) {
			throw EXCEPTION("Tile out of range (%i, %i)", tileX, tileY);
		}
		//EDEBUG("Tile %u - drawing at (%i, %i)", i, tileX, tileY);
		if(drawTiles) {
			for(uint32_t y = 0; y != ra2TileHeight; y++) {
				if(y >= numScanLines) {
					throw EXCEPTION(
						"Tile %u - Writing too many scanlines (y == %u, numScanLines == %u)",
						i, y, numScanLines
					);
				}
				for(uint32_t x = 0; x != ra2TileWidth; x++) {
					px = getPixel(x, y);
					if(px != 0) {
						img[((tileY + y) * scanWidth) + (tileX + x)] = px;
					}
				}
			}
		}
		if(drawExtras && tileHeader[i].hasExtra()) {
			tileX = tileHeader[i].getExtraX() - extraBounds[0];
			tileY = tileHeader[i].getExtraY(maxHeight) - extraBounds[1];
			if(tileX < 0 || tileY < 0) {
				throw EXCEPTION("Extra out of range (%i, %i)", tileX, tileY);
			}
			/*EDEBUG("Extra %u - drawing at (%i, %i) [%u %u]",
				i, tileX, tileY, tileHeader[i].extraW, tileHeader[i].extraH
			);*/
			for(uint32_t y = 0; y != tileHeader[i].extraH; y++) {
				for(uint32_t x = 0; x != tileHeader[i].extraW; x++) {
					px = getExtraPixel(x, y);
					if(px) {
						img[((tileY + y) * scanWidth) + (tileX + x)] = px;
					}
				}
			}
		}
	}
}

void TMPFile::getHeightTemplate(uint8_t* img, size_t scanWidth, size_t numScanLines) {
	uint8_t px;
	uint32_t tx, ty;
	int32_t bounds[4];
	getTotalSize(tx, ty);
	getBounds(bounds[0], bounds[1], bounds[2], bounds[3]);
	EDEBUG("tx == %u, ty == %u, bounding box (%i, %i) -> (%i, %i)",
		tx, ty, bounds[0], bounds[1], bounds[2], bounds[3]
	);
	int32_t tileX, tileY;
	for(uint32_t i = 0; i != header.tilesX * header.tilesY; i++) {
		if(header.offset[i] == 0) {
			continue;
		}
		setCurrentTile(i);
		tileX = tileHeader[i].x - bounds[0];
		tileY = tileHeader[i].y - bounds[1];
		if(tileX < 0 || tileY < 0) {
			throw EXCEPTION("Tile out of range");
		}
		EDEBUG("Tile %u - drawing at (%i, %i)", i, tileX, tileY);
		for(uint32_t y = 0; y != ra2TileHeight; y++) {
			if(y >= numScanLines) {
				throw EXCEPTION(
					"Tile %u - Writing too many scanlines (y == %u, numScanLines == %u)",
					i, y, numScanLines
				);
			}
			for(uint32_t x = 0; x != ra2TileWidth; x++) {
				px = getHeight(x, y);
				if(px != 0) {
					img[((tileY + y) * scanWidth) + (tileX + x)] = px;
				}
			}
		}
	}
}

void TMPFile::print() {
	printf("Contains %u x %u tiles of size %u x %u\n", header.tilesX, header.tilesY, header.tileWidth, header.tileHeight);
	for(uint32_t i = 0; i != header.tilesX * header.tilesY; i++) {
		if(header.offset[i]) {
			printf("Tile %u - offset %u {\n\t.x = %i, .y = %i,\n\t.extraOffset = %u, .zOffset = %u, .extraZOffset = %u,\n\t.extraX = %i, .extraY = %i, .extraW = %u, .extraH = %i,\n\t.flags = 0x%08X (%s extra data),\n\t.height = %u, .terrainType = %u, .rampType = %u,\n\t.radarRedLeft = %u, .radarGreenLeft = %u, .radarBlueLeft = %u,\n\t.radarRedRight = %u, .radarGreenRight = %u, .radarBlueRight = %u\n\t.pad = { %X, %X, %X }\n}\n",
				i, header.offset[i], tileHeader[i].x, tileHeader[i].y,
				tileHeader[i].extraOffset, tileHeader[i].zOffset, tileHeader[i].extraZOffset,
				tileHeader[i].extraX, tileHeader[i].extraY, tileHeader[i].extraW, tileHeader[i].extraH,
				tileHeader[i].flags, tileHeader[i].hasExtra() ? "has" : "no",
				tileHeader[i].height, tileHeader[i].terrainType, tileHeader[i].rampType,
				tileHeader[i].radarLeftColour[0], tileHeader[i].radarLeftColour[1],
				tileHeader[i].radarLeftColour[2],
				tileHeader[i].radarRightColour[0], tileHeader[i].radarRightColour[1],
				tileHeader[i].radarRightColour[2],
				tileHeader[i].pad[0], tileHeader[i].pad[1], tileHeader[i].pad[2]
			);
		} else {
			printf("Tile %u - empty\n", i);
		}
	}
}
