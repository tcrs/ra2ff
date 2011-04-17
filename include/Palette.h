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

#ifndef PALETTE_H__
#define PALETTE_H__

#include <stdint.h>
#include <string>
#include "Utils.h"

class Palette {
protected:
	uint8_t palette[256][3];
	uint8_t channelDepth[3];
public:

	Palette() { }
	Palette(Utils::FixedRead&);
	Palette(std::string const&);
	~Palette() { }
	
	void getRGB(uint8_t c, uint8_t& r, uint8_t& g, uint8_t& b) const;
	void getRGB(uint8_t, uint8_t&, uint8_t&, uint8_t&);
	void getRGB(uint8_t c, float& r, float& g, float& b) const;
	void getRGB(uint8_t c, float& r, float& g, float& b);

	void readPackedRGB(Utils::FixedRead&);
	void readPackedRGB(std::string const&);

	void setChannelDepth(uint8_t = 6, uint8_t = 6, uint8_t = 6);
};

#endif
