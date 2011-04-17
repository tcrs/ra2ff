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

#ifndef SDLSPRITE_H__
#define SDLSPRITE_H__

#include "Sprite.h"
#include "Exception.h"

class SDLSprite : public Sprite {
protected:
	SDL_Surface** frames;
	unsigned int numFrames;
public:
	SDLSprite(SDL_Surface*, unsigned int, unsigned int, bool);
	virtual ~SDLSprite();

	virtual void drawFrame(unsigned int n, float x, float y, float w = 1, float h = 1);
	virtual void drawFrameSegment(unsigned int n, float x, float y, float w, float h, float srcX, float srcY, float srcW, float srcH);
};

#endif
