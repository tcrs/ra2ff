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

#ifndef SDLUTILS_H__
#define SDLUTILS_H__

#include <SDL/SDL.h>
#include <assert.h>

namespace SDL {
	struct ScopedSurfaceLock {
		SDL_Surface* surf;
		ScopedSurfaceLock(SDL_Surface* s) : surf(NULL) {
			if(SDL_MUSTLOCK(s)) {
				if(SDL_LockSurface(s) == -1) {
					throw EXCEPTION("Could not lock surface %p", s);
				} else {
					surf = s;
				}
			}
		}
		~ScopedSurfaceLock() {
			if(surf) {
				SDL_UnlockSurface(surf);
			}
		}
	};
	struct ScopedSurface {
		SDL_Surface* surf;
		ScopedSurface(SDL_Surface* s) : surf(s) { }
		~ScopedSurface() {
			if(surf) {
				SDL_FreeSurface(surf);
			}
		}
	};
	struct ScopedAlphaChange {
		SDL_Surface* surf;
		uint32_t alphaFlag;
		uint8_t alphaValue;
		ScopedAlphaChange(SDL_Surface* s, uint32_t aflag, uint8_t aval) : surf(s) {
			assert(surf);
			alphaFlag = (surf->flags & SDL_SRCALPHA) | (surf->flags & SDL_RLEACCEL);
			alphaValue = surf->format->alpha;
			SDL_SetAlpha(surf, aflag, aval);
		}
		~ScopedAlphaChange() {
			SDL_SetAlpha(surf, alphaFlag, alphaValue);
		}
	};
}

#endif
