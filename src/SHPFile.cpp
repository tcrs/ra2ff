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
#include "Exception.h"
#include <stdio.h>

SHPFile::SHPFile(std::string const& file) : imgHeaders(NULL), currentImage(0) {
	FILE* f = fopen(file.c_str(), "rb");
	if(f == NULL) {
		throw EXCEPTION("Could not open \"%s\" (%s)", file.c_str(), strerror(errno));
	}
	Utils::FixedRead fixed(f);

	fixed.read(&header.zero);
	fixed.read(&header.width);
	fixed.read(&header.height);
	fixed.read(&header.numImages);
	if(header.zero != 0) {
		throw EXCEPTION("File \"%s\" first word != 0 (is %u)", file.c_str(), header.zero);
	}

	EDEBUG("File \"%s\" contains %u images, with dimensions %u x %u", file.c_str(), header.numImages, header.width, header.height);

	imgHeaders = new IMGHeader[header.numImages];
	for(uint16_t i = 0; i != header.numImages; i++) {
		readIMGHeader(i, fixed);
		imgHeaders[i].alloc();
	}
	for(uint16_t i = 0; i != header.numImages; i++) {
		readIMG(i, fixed);
	}
}

void SHPFile::readIMGHeader(uint16_t n, Utils::FixedRead& fixed) {
	IMGHeader* ih = &imgHeaders[n];
	fixed.read(&ih->x);
	fixed.read(&ih->y);
	fixed.read(&ih->w);
	fixed.read(&ih->h);
	fixed.read(&ih->compressionType);
	if(ih->compressionType > 3) {
		throw EXCEPTION("Image header %u - unknown compression type 0x%02X", n, ih->compressionType);
	}
	fixed.read(&ih->unknown);
	fixed.read(&ih->unknown2);
	fixed.read(&ih->transparent);
	fixed.read(&ih->zero);
	if(ih->zero != 0) {
		throw EXCEPTION("Image header %u - zero field == %u (not zero)", n, ih->zero);
	}
	fixed.read(&ih->offset);
}

void SHPFile::readIMG(uint16_t n, Utils::FixedRead& fixed) {
	fixed.seek(imgHeaders[n].offset);
	switch(imgHeaders[n].compressionType) {
		case 0:
		case 1:
			readIMGType1(n, fixed);
			break;
		case 2:
			readIMGType2(n, fixed);
			break;
		case 3:
			readIMGType3(n, fixed);
			break;
		default:
			throw EXCEPTION("Cannot decompress image compression type %u", imgHeaders[n].compressionType);
			break;
	}
}

/*
 * compressionType == 1
 * Just raw 8bpp image data
 */
void SHPFile::readIMGType1(uint16_t n, Utils::FixedRead& fixed) {
	unsigned int imgPos = 0;
	for(uint16_t y = 0; y != imgHeaders[n].h; y++) {
		for(uint16_t x = 0; x != imgHeaders[n].w; x++) {
			fixed.read(&imgHeaders[n].img[imgPos]);
			imgPos++;
		}
	}
}

/*
 * compressionType == 2
 * As compressionType == 3, but there is no RLE encoding of zero bytes.
 * i.e. just copy the data, but skip the 16bit scanline byte count at
 * the start of each scanline.
 */
void SHPFile::readIMGType2(uint16_t n, Utils::FixedRead& fixed) {
	uint16_t cBytes;
	unsigned int imgPos = 0;
	unsigned int x;
	for(uint16_t i = 0; i != imgHeaders[n].h; i++) {
		/* Read number of bytes in scanline */
		fixed.read(&cBytes);
		/* Take off the two bytes just read */
		cBytes -= 2;
		/* Keep track of the x coord (for sanity checking */
		x = 0;
		while(cBytes > 0) {
			if(x > imgHeaders[n].w) {
				throw EXCEPTION("Scanline %u - x value has slipped out of range (x == %u)", i, x);
			}
			/* Sanity check - ensure we can write another byte to the output buffer */
			if(imgPos >= (unsigned int)imgHeaders[n].w * (unsigned int)imgHeaders[n].h) {
				throw EXCEPTION("Scanline %u - cannot write next byte as it would overflow output buffer (x == %u)", i, x);
			}
			/* Just read the byte into the buffer */
			fixed.read(&imgHeaders[n].img[imgPos]);
			imgPos++;
			cBytes--;
			x++;
		}
	}
}

/*
 * compressionType == 3
 * each scan line starts with a 16 bit count, which is the number of
 * bytes in the scanline (including the 2 bytes for the count)
 * for each byte in the scanline {
 * 	if(byte == 0) {
 * 		c = next byte in scanline;
 * 		write c zeros;
 * 	} else {
 * 		write byte;
 * 	}
 * }
 */
void SHPFile::readIMGType3(uint16_t n, Utils::FixedRead& fixed) {
	uint16_t cBytes;
	uint8_t b;
	unsigned int imgPos = 0;
	unsigned int x;
	for(uint16_t i = 0; i != imgHeaders[n].h; i++) {
		/* Read number of bytes in scanline */
		fixed.read(&cBytes);
		/* Take off the two bytes just read */
		cBytes -= 2;
		/* Keep track of the x coord (for sanity checking */
		x = 0;
		while(cBytes > 0) {
			if(x > imgHeaders[n].w) {
				throw EXCEPTION("Scanline %u - x value has slipped out of range (x == %u)", i, x);
			}
			fixed.read(&b);
			cBytes--;
			if(b != 0) {
				/* Sanity check - ensure we can write another byte to the output buffer */
				if(imgPos >= (unsigned int)imgHeaders[n].w * (unsigned int)imgHeaders[n].h) {
					throw EXCEPTION("Scanline %u - cannot write byte 0x%02X as it would overflow output buffer (x == %u)", i, b, x);
				}
				/* If the byte is not zero then just copy it into the output */
				imgHeaders[n].img[imgPos++] = b;
				x++;
			} else {
				/* Sanity check - ensure we can read another byte from the scanline */
				if(cBytes == 0) {
					throw EXCEPTION("Scanline %u - ends halfway through RLE-zero byte", i);
				}
				/* If bytes == 0 then read count, then output that many 0 bytes */
				fixed.read(&b);
				cBytes--;
				/* Sanity check - ensure writing the requested number of bytes will not overflow the scanline */
				if((unsigned int)x + (unsigned int)b > (unsigned int)imgHeaders[n].w) {
					//ERROR("Scanline %u - RLE-encoded zero byte would overflow scanline (want to write %u zeros at x %u) writing %u instead", i, b, x, imgHeaders[n].w - x);
					b = imgHeaders[n].w - x;
				}
				for(uint8_t i = 0; i < b; i++) {
					/* Sanity check - ensure we can write another byte to the output buffer */
					if(imgPos >= (unsigned int)imgHeaders[n].w * (unsigned int)imgHeaders[n].h) {
						throw EXCEPTION("Scanline %u - cannot write RLE-encoded zero byte %u as it would overflow output buffer (x == %u)", i, b, x);
					}
					imgHeaders[n].img[imgPos++] = 0;
					x++;
				}
			}
		}
	}
}

SHPFile::~SHPFile() {
	delete[] imgHeaders;
}

void SHPFile::setCurrentImage(unsigned int n) {
	if(n >= header.numImages) {
		throw EXCEPTION("Could not set current image to %u, number of images == %u", n, header.numImages);
	}
	currentImage = n;
}

unsigned int SHPFile::numImages() {
	return header.numImages;
}

void SHPFile::getImageSize(unsigned int& x, unsigned int& y) {
	x = imgHeaders[currentImage].w;
	y = imgHeaders[currentImage].h;
}

uint8_t SHPFile::getPixel(unsigned int x, unsigned int y) {
	if(x >= imgHeaders[currentImage].w || y >= imgHeaders[currentImage].h) {
		throw EXCEPTION("Could not get pixel at (%u, %u) as image has size %u x %u", x, y, imgHeaders[currentImage].w, imgHeaders[currentImage].h);
	}
	return imgHeaders[currentImage].img[(y * imgHeaders[currentImage].w) + x];
}

void SHPFile::print() {
	printf("SHP contains %u frames, %u x %u pixels\n", header.numImages, header.width, header.height);
	for(unsigned int i = 0; i != header.numImages; i++) {
		printf("  Frame %u - (%u, %u) size [%u %u], compression = %u\n", i,
			imgHeaders[i].x, imgHeaders[i].y,
			imgHeaders[i].w, imgHeaders[i].h,
			imgHeaders[i].compressionType
		);
	}
}
