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

#ifndef SHPFILE_H__
#define SHPFILE_H__

#include <stdint.h>
#include <string>
#include "Utils.h"

class SHPFile {
public:
	struct Header {
		uint16_t zero;			/* Always zero (to differentiate between formats?) */
		uint16_t width;			/* Width of the images */
		uint16_t height;		/* Height of the images */
		uint16_t numImages;		/* Number of images in the file */
	};
	struct IMGHeader {
		uint16_t x, y;
		uint16_t w, h;			/* Width and height of stored image */
		uint8_t compressionType;	/* Type of compression for this image 1/2/3 */
		uint8_t unknown;		/* Unknown (seems to be zero) */
		uint16_t unknown2;
		uint32_t transparent;		/* Transparent colour (?) */
		uint32_t zero;
		uint32_t offset;

		uint8_t* img;			/* Decoded image data */
		IMGHeader() : img(NULL) { }
		~IMGHeader() { delete[] img; }
		void alloc() {
			img = new uint8_t[w * h];
		}
	};
protected:
	Header header;
	IMGHeader* imgHeaders;
	unsigned int currentImage;

	void readIMGHeader(uint16_t, Utils::FixedRead&);
	void readIMG(uint16_t, Utils::FixedRead&);
	void readIMGType1(uint16_t, Utils::FixedRead&);
	void readIMGType2(uint16_t, Utils::FixedRead&);
	void readIMGType3(uint16_t, Utils::FixedRead&);
public:
	SHPFile(std::string const&);
	~SHPFile();

	void setCurrentImage(unsigned int);
	unsigned int numImages();
	void getImageSize(unsigned int&, unsigned int&);
	uint8_t getPixel(unsigned int, unsigned int);

	void print();
};

#endif
