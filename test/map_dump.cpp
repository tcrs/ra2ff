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

#include "Base64.h"
#include "LZODecompress.h"
#include "MapReader.h"
#include "Palette.h"
#include "Exception.h"
#include "Utils.h"
#include "SDLUtils.h"
#include "Display.h"
#include <SDL/SDL.h>

#define NUM_PACKS	4
#define LZO		0x00
#define F80		0x01

char const * const pack[NUM_PACKS] = {
	"PreviewPack",
	"IsoMapPack5",
	"OverlayDataPack",
	"OverlayPack",
};

uint8_t packFlags[NUM_PACKS] = {
	LZO,
	LZO,
	F80,
	F80,
};

int main(int argc, char** argv) {
	if(argc < 2) {
		fprintf(stderr, "Usage: (bin) <map-file> [<pack-name>]\n");
		return 1;
	}
	uint8_t* data;
	uint8_t* unpacked;
	unsigned int previewWidth = 160;
	unsigned int previewHeight = 80;
	FILE* f;
	size_t len, unpackedLen;
	std::ostringstream fname;
	INIFile ini(argv[1]);
	for(unsigned int i = 0; i < NUM_PACKS; i++) {
		if(argc > 2 && strcmp(pack[i], argv[2])) {
			continue;
		}
		EDEBUG("Dumping %s", pack[i]);
		ini.setCurrentSection(pack[i]);
		data = Base64::decode(ini, len);
		unpacked = MapReader::unpack(data, len, unpackedLen, packFlags[i] & F80 ? MapReader::F80Pack : MapReader::LZOPack);
		delete[] data;
		EDEBUG("Unpacked %lu bytes from %s", unpackedLen, pack[i]);
		fname.str("");
		fname << argv[1];
		fname << "-";
		fname << pack[i];
		fname << ".bin";
		f = fopen(fname.str().c_str(), "wb");
		fwrite(unpacked, 1, unpackedLen, f);
		fclose(f);
		if(strcmp(pack[i], "PreviewPack") == 0) {
			SDL_Surface* img = SDL_CreateRGBSurface(SDL_SWSURFACE, previewWidth, previewHeight, 32, 0, 0, 0, 0);
			if(img == NULL) {
				throw EXCEPTION("Arg");
			}
			{
				SDL::ScopedSurfaceLock lock(img);
				uint32_t* opx = reinterpret_cast<uint32_t*>(img->pixels);
				for(unsigned int j = 0; j != previewWidth * previewHeight; j++) {
					*opx = SDL_MapRGBA(
						img->format,
						unpacked[j * 3],
						unpacked[(j * 3) + 1],
						unpacked[(j * 3) + 2],
						255
					);
					opx++;
				}
			}
			SDL_SaveBMP(img, "tmp.bmp");
			SDL_FreeSurface(img);
		} else if(strcmp(pack[i], "IsoMapPack5") == 0) {
			MapReader map;
			map.readIsoMapPack(unpacked, unpackedLen);
			map.print();
		}
		delete[] unpacked;
	}
}
