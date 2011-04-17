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

#include "SHPFile.h"
#include "Palette.h"
#include <sstream>
#include <SDL/SDL.h>

int main(int argc, char** argv) {
	if(argc < 3) {
		fprintf(stderr, "Usage: (bin) <shp-file> <pal-file>\n");
		return 1;
	}
	SHPFile shp(argv[1]);
	Palette pal(argv[2]);
	SDL_Surface* img;
	std::ostringstream fname;
	unsigned int xSz, ySz;
	uint8_t px, r, g, b;
	for(unsigned int i = 0; i != shp.numImages(); i++) {
		shp.setCurrentImage(i);
		shp.getImageSize(xSz, ySz);
		if(xSz == 0 || ySz == 0) {
			EDEBUG("Skipping image %u as it is empty", i);
			continue;
		}
		img = SDL_CreateRGBSurface(SDL_SWSURFACE, xSz, ySz, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
		SDL_LockSurface(img);
		uint32_t* opx = reinterpret_cast<uint32_t*>(img->pixels);
		for(unsigned int y = 0; y != ySz; y++) {
			for(unsigned int x = 0; x != xSz; x++) {
				px = shp.getPixel(x, y);
				pal.getRGB(px, r, g, b);
				*opx = SDL_MapRGBA(img->format, r, g, b, 255);
				opx++;
			}
		}
		SDL_UnlockSurface(img);
		fname.str("");
		fname << argv[1];
		fname << "-";
		if(i < 100) 
			fname << "0";
		if(i < 10)
			fname << "0";
		fname << i;
		fname << ".bmp";
		EDEBUG("Writing frame %u to %s", i, fname.str().c_str());
		SDL_SaveBMP(img, fname.str().c_str());
		SDL_FreeSurface(img);
	}
}
