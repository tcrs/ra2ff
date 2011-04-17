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

#include "Input.h"
#include "Display.h"
#include "Exception.h"

Input::Input() : frameDelta(0), framePeriod(150) {
	Display::init();
}

Input::~Input() {
	Display::quit();
}

void Input::run() {
	SDL_Event ev;
	unsigned int start;
	while(1) {
		start = SDL_GetTicks();
		if(main()) {
			break;
		}
		do {
			while(SDL_PollEvent(&ev)) {
				dispatch(&ev);
			};
			frameDelta = SDL_GetTicks() - start;
		} while(frameDelta < framePeriod);
	}
}

void Input::dispatch(SDL_Event* ev) {
	switch(ev->type) {
		case SDL_KEYDOWN:
			inputKeyDown(ev->key.keysym.sym, ev->key.keysym.mod);
		break;
		case SDL_KEYUP:
			inputKeyUp(ev->key.keysym.sym, ev->key.keysym.mod);
		break;
		case SDL_MOUSEMOTION:
			inputMouseMove(ev->motion.state, ev->motion.x, ev->motion.y, ev->motion.xrel, ev->motion.yrel);
		break;
		case SDL_MOUSEBUTTONDOWN:
			inputMouseDown(ev->button.button, ev->button.x, ev->button.y);
		break;
		case SDL_MOUSEBUTTONUP:
			inputMouseUp(ev->button.button, ev->button.x, ev->button.y);
		break;
		case SDL_QUIT:
			inputQuit();
		break;
	}
}

void Input::inputQuit() {
	Display::quit();
	exit(EXIT_SUCCESS);
}
