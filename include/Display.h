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

#ifndef DISPLAY_H__
#define DISPLAY_H__

#include <stdint.h>

struct SDL_Surface;

#define IMPL_DUMMY	0
#define IMPL_SDL	1
#define IMPL_GL		2

class Display {
public:
	struct Window {
		unsigned int x, y;
		unsigned int w, h;
		Window(unsigned int X = 0, unsigned int Y = 0, unsigned int W = 1024, unsigned int H = 1024) 
			: x(X), y(Y), w(W), h(H) { }
	};
	static int activeImplementation;
	static unsigned int resX;
	static unsigned int resY;
protected:
	static Display* instance;

	SDL_Surface* screen;
	int implementation;
	uint8_t cColR, cColG, cColB, cColA;

	static Display* Instance();
	Display(int);
	~Display();

	SDL_Surface* getScreenI();
	void flipI();
	void clearI();
	void setBackgroundColourI(float, float, float, float);
	void setup2DI();
	void setup3DI();
public:
	static SDL_Surface* getScreen() {
		return Instance()->getScreenI();
	}
	static void flip() {
		Instance()->flipI();
	}
	static void clear() {
		Instance()->clearI();
	}
	static void setBackgroundColour(float r, float g, float b, float a) {
		Instance()->setBackgroundColourI(r, g, b, a);
	}
	static void init() {
		Instance();
	}
	static void quit() {
		delete Instance();
	}
	static int getImplementation() {
		return Instance()->implementation;
	}
	static void setup2D() {
		Instance()->setup2DI();
	}
	static void setup3D() {
		Instance()->setup3DI();
	}
};

#endif
