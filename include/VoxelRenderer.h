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

#ifndef VOXELRENDERER_H__
#define VOXELRENDERER_H__

#include "VXLFile.h"
#include "HVAFile.h"
#include <SDL/SDL_opengl.h>

class VoxelRenderer {
public:
	static float lightPos[4];
	static float lightSpec[4];
	static float lightLight[4];
	static float lightAmb[4];
protected:
	void renderSection(bool, bool);
	void renderVoxel(float, float, float, float);

	VXLFile& vxl;
	HVAFile* hva;
public:
	uint32_t frame;
	float pitch;

	VoxelRenderer(VXLFile& v, HVAFile* h = NULL) : vxl(v), hva(h), frame(0), pitch(0) { }
	~VoxelRenderer() { }

	void render(bool = true, bool = true);
	void setHVA(HVAFile*);
	static void setupLighting(int = GL_LIGHT0);
};

#endif
