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

#include "Display.h"
#include "Exception.h"
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

Display* Display::instance = NULL;
int Display::activeImplementation = IMPL_GL;
unsigned int Display::resX = 800;
unsigned int Display::resY = 600;

Display* Display::Instance() {
	if(instance == NULL) {
		instance = new Display(activeImplementation);
	}
	return instance;
}

Display::Display(int impl) : screen(NULL), implementation(impl) {
	unsigned int flags = 0;
	if(implementation == IMPL_DUMMY) {
		return;
	}
	if(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) == -1) {
		throw EXCEPTION("Could not initialise SDL (%s)", SDL_GetError());
	}

	if(implementation == IMPL_GL) {
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
		flags |= SDL_OPENGL;
	}

	/* Deal with resolution issues! */
	screen = SDL_SetVideoMode(resX, resY, 0, flags);
	if(screen == NULL) {
		throw EXCEPTION("SDL_SetVideoMode failed (\"%s\")", SDL_GetError());
	}

	if(implementation == IMPL_GL) {
		setup2DI();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glShadeModel(GL_SMOOTH);
		glEnable(GL_COLOR_MATERIAL);
		//glEnable(GL_LIGHTING);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glClearColor(1, 1, 1, 1);
		GLCHECKERROR;
		DEBUG("OpenGL Initialised");
	} else {
		DEBUG("SDL initialised");
	}
}

Display::~Display() {
	if(implementation != IMPL_DUMMY) {
		SDL_Quit();
	}
}

void Display::setup3DI() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glEnable(GL_LIGHTING);
	glViewport(0, 0, resX, resY);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, (float)resX / (float)resY, 1, 500);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 3, 5, 0, 0, 0, 0, 1, 0);
}

void Display::setup2DI() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
}

SDL_Surface* Display::getScreenI() {
	return screen;
}

void Display::flipI() {
	if(implementation == IMPL_GL) {
		SDL_GL_SwapBuffers();
	} else if(implementation == IMPL_SDL) {
		SDL_Flip(screen);
	}
}

void Display::clearI() {
	if(implementation == IMPL_GL) {
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	} else if(implementation == IMPL_SDL) {
		SDL_FillRect(screen, NULL, SDL_MapRGBA(screen->format, cColR, cColG, cColB, cColA));
	}
}

void Display::setBackgroundColourI(float r, float g, float b, float a) {
	if(implementation == IMPL_GL) {
		glClearColor(r, g, b, a);
	} else if(implementation == IMPL_SDL) {
		cColR = r * 255;
		cColG = g * 255;
		cColB = b * 255;
		cColA = a * 255;
	}
}

/*void Display::setWindowI(Window const& lep) {
	window = lep;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(window.x, window.w, window.y + window.h, window.y, 0, 1);
	glMatrixMode(GL_MODELVIEW);
}*/

/*void Display::clipToWindowI(LeptonWindow& leptons) {
	if(leptons.x < lepWindow.x) {
		leptons.x = lepWindow.x;
	}
	if(leptons.x + leptons.w > lepWindow.x + lepWindow.w) {
		leptons.w = lepWindow.w - (leptons.x - lepWindow.x);
	}
	if(leptons.y < lepWindow.y) {
		leptons.y = lepWindow.y;
	}
	if(leptons.y + leptons.h > lepWindow.y + lepWindow.h) {
		leptons.h = lepWindow.h - (leptons.y - lepWindow.y);
	}
}*/
