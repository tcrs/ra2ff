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
#include "Palette.h"
#include "SDLUtils.h"
#include <stdio.h>
#include <SDL/SDL.h>
#include <sstream>

int main(int argc, char** argv) {
	if(argc < 3) {
		fprintf(stderr, "Usage: (bin) <tmp-file> <pal-file>\n");
		return -1;
	}
	TMPFile tmp(argv[1]);
	Palette pal(argv[2]);

	tmp.print();
	
	SDL_Surface* img;
	std::ostringstream fname;
	unsigned int xSz, ySz;
	uint8_t r, g, b;

	tmp.getTotalSize(xSz, ySz);
	img = SDL_CreateRGBSurface(SDL_SWSURFACE, xSz, ySz, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
	/*
	{
		uint8_t* imgData = new uint8_t[xSz * ySz];
		for(unsigned int i = 0; i != xSz * ySz; i++) {
			imgData[i] = 0x00;
		}
		Utils::ScopedArray<uint8_t> imgData_free(imgData);
		SDL::ScopedSurfaceLock lock(img);
		tmp.getTemplate(imgData, xSz, ySz, true, false);
		for(unsigned int i = 0; i != xSz * ySz; i++) {
			pal.getRGB(imgData[i], r, g, b);
			reinterpret_cast<uint32_t*>(img->pixels)[i] = SDL_MapRGBA(img->format, r, g, b, 255);
		}
	}
	fname.str("");
	fname << argv[1];
	fname << "-Template.bmp";
	SDL_SaveBMP(img, fname.str().c_str());
	{
		uint8_t* imgData = new uint8_t[xSz * ySz];
		for(unsigned int i = 0; i != xSz * ySz; i++) {
			imgData[i] = 0x00;
		}
		Utils::ScopedArray<uint8_t> imgData_free(imgData);
		SDL::ScopedSurfaceLock lock(img);
		tmp.getTemplate(imgData, xSz, ySz, false, true);
		for(unsigned int i = 0; i != xSz * ySz; i++) {
			pal.getRGB(imgData[i], r, g, b);
			reinterpret_cast<uint32_t*>(img->pixels)[i] = SDL_MapRGBA(img->format, r, g, b, 255);
		}
	}
	fname.str("");
	fname << argv[1];
	fname << "-Extra.bmp";
	SDL_SaveBMP(img, fname.str().c_str());
	*/
	{
		uint8_t* imgData = new uint8_t[xSz * ySz];
		for(unsigned int i = 0; i != xSz * ySz; i++) {
			imgData[i] = 0x00;
		}
		Utils::ScopedArray<uint8_t> imgData_free(imgData);
		SDL::ScopedSurfaceLock lock(img);
		tmp.getTemplate(imgData, xSz, ySz, true, true);
		for(unsigned int i = 0; i != xSz * ySz; i++) {
			pal.getRGB(imgData[i], r, g, b);
			reinterpret_cast<uint32_t*>(img->pixels)[i] = SDL_MapRGBA(img->format, r, g, b, 255);
		}
	}
	fname.str("");
	fname << argv[1];
	fname << "-template.bmp";
	SDL_SaveBMP(img, fname.str().c_str());
	/*{
		uint8_t* imgData = new uint8_t[xSz * ySz];
		for(unsigned int i = 0; i != xSz * ySz; i++) {
			imgData[i] = 0x00;
		}
		Utils::ScopedArray<uint8_t> imgData_free(imgData);
		SDL::ScopedSurfaceLock lock(img);
		tmp.getHeightTemplate(imgData, xSz, ySz);
		for(unsigned int i = 0; i != xSz * ySz; i++) {
			reinterpret_cast<uint32_t*>(img->pixels)[i] = SDL_MapRGBA(img->format, imgData[i], imgData[i], imgData[i], 255);
		}
	}
	fname.str("");
	fname << argv[1];
	fname << "-Height.bmp";
	SDL_SaveBMP(img, fname.str().c_str());*/
	SDL_FreeSurface(img);
}
