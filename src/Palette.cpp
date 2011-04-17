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

#include "Palette.h"

Palette::Palette(Utils::FixedRead& fixed) {
	setChannelDepth();
	readPackedRGB(fixed);
}

Palette::Palette(std::string const& file) {
	setChannelDepth();
	readPackedRGB(file);
}

void Palette::readPackedRGB(std::string const& file) {
	FILE* f = fopen(file.c_str(), "rb");
	if(f == NULL) {
		throw EXCEPTION("Could not open \"%s\" (%s)", file.c_str(), strerror(errno));
	}
	Utils::FixedRead fixed(f);
	readPackedRGB(fixed);
}

void Palette::readPackedRGB(Utils::FixedRead& fixed) {
	for(unsigned int i = 0; i != 256; i++) {
		fixed.read(&palette[i][0], 3);
	}
}

/* Each colour channel is only 6 bits deep, so shift the 6 bits
 * into the most significant bits of the colour channel
 */

void Palette::getRGB(uint8_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
	r = palette[c][0] << (8 - channelDepth[0]);
	g = palette[c][1] << (8 - channelDepth[1]);
	b = palette[c][2] << (8 - channelDepth[2]);
}

void Palette::getRGB(uint8_t c, uint8_t& r, uint8_t& g, uint8_t& b) const {
	r = palette[c][0] << (8 - channelDepth[0]);
	g = palette[c][1] << (8 - channelDepth[1]);
	b = palette[c][2] << (8 - channelDepth[2]);
}

void Palette::getRGB(uint8_t c, float& r, float& g, float& b) {
	r = (float)(palette[c][0] << (8 - channelDepth[0])) / (float)255;
	g = (float)(palette[c][1] << (8 - channelDepth[1])) / (float)255;
	b = (float)(palette[c][2] << (8 - channelDepth[2])) / (float)255;
}

void Palette::getRGB(uint8_t c, float& r, float& g, float& b) const {
	r = (float)(palette[c][0] << (8 - channelDepth[0])) / (float)255;
	g = (float)(palette[c][1] << (8 - channelDepth[1])) / (float)255;
	b = (float)(palette[c][2] << (8 - channelDepth[2])) / (float)255;
}

void Palette::setChannelDepth(uint8_t r, uint8_t g, uint8_t b) {
	channelDepth[0] = r;
	channelDepth[1] = g;
	channelDepth[2] = b;
}
