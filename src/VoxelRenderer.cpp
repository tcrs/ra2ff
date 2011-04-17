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

#include "VoxelRenderer.h"
#include "Exception.h"
#include <SDL/SDL_opengl.h>

float VoxelRenderer::lightPos[4] = { 5, 0, 10, 0, };
float VoxelRenderer::lightSpec[4] = { 1, 0.5, 0, 0, };
float VoxelRenderer::lightLight[4] = { 0.5, 0.5, 0.5, 1, };
float VoxelRenderer::lightAmb[4] = { 0.75, 0.75, 0.75, 1, };

void VoxelRenderer::setupLighting(int light) {
	glLightfv(light, GL_POSITION, lightPos);
	glLightfv(light, GL_SPECULAR, lightSpec);
	glLightfv(light, GL_AMBIENT, lightAmb);
	glLightfv(light, GL_DIFFUSE, lightLight);
	glEnable(light);
	glEnable(GL_LIGHTING);
}

void VoxelRenderer::setHVA(HVAFile* h) {
	hva = h;
}

void printMatrix(float* m) {
	/* OpenGL matrices are layed out thus:
	 * [ 0] [ 4] [ 8] [12]
	 * [ 1] [ 5] [ 9] [13]
	 * [ 2] [ 6] [10] [14]
	 * [ 3] [ 7] [11] [15]
	 */
	printf("|% 8.04f  % 8.04f  % 8.04f  % 8.04f|\n", m[0], m[4], m[8], m[12]);
	printf("|% 8.04f  % 8.04f  % 8.04f  % 8.04f|\n", m[1], m[5], m[9], m[13]);
	printf("|% 8.04f  % 8.04f  % 8.04f  % 8.04f|\n", m[2], m[6], m[10], m[14]);
	printf("|% 8.04f  % 8.04f  % 8.04f  % 8.04f|\n", m[3], m[7], m[11], m[15]);
}

void VoxelRenderer::render(bool coloured, bool normals) {
	if(hva && frame >= hva->numFrames()) {
		frame = 0;
	}
	for(uint32_t i = 0; i != vxl.getNumLimbs(); i++) {
		vxl.setCurrentLimb(i);
		if(hva) {
			hva->setCurrentSection(vxl.limbName());
		}
		renderSection(coloured, normals);
	}
}

void VoxelRenderer::renderSection(bool coloured, bool normals) {
	glPushMatrix();
	uint8_t xs, ys, zs;
	float min[3], max[3];
	float transform[16];
	float sectionScale[3];

	vxl.getSize(xs, ys, zs);
	vxl.getBounds(min, max);

	/* Calculate the screen units / voxel ratio for scaling */
	max[0] -= min[0];
	max[1] -= min[1];
	max[2] -= min[2];
	sectionScale[0] = max[0] / (float)xs;
	sectionScale[1] = max[1] / (float)ys;
	sectionScale[2] = max[2] / (float)zs;

	/* Load transformation matrix */
	if(hva) {
		hva->loadGLMatrix(frame, transform);
		/* The HVA transformation matrices have to be scaled */
		transform[12] *= vxl.getScale() * sectionScale[0];
		transform[13] *= vxl.getScale() * sectionScale[1];
		transform[14] *= vxl.getScale() * sectionScale[2];
	} else {
		vxl.loadGLMatrix(transform);
	}

	/* Apply the transform for this frame */
	glMultMatrixf(transform);

	/* Translate to the bottom left of the section's bounding box */
	glTranslatef(min[0], min[1], min[2]);


	VXLFile::LimbBody::Span::Voxel vx;
	glBegin(GL_QUADS);
	for(unsigned int x = 0; x != xs; x++) {
		for(unsigned int y = 0; y != ys; y++) {
			for(unsigned int z = 0; z != zs; z++) {
				if(vxl.getVoxel(x, y, z, &vx)) {
					if(coloured) {
						float colour[3];
						vxl.getPalette().getRGB(vx.colour, colour[0], colour[1], colour[2]);
						glColor3fv(colour);
					}
					if(normals) {
						float normal[3];
						vxl.getXYZNormal(vx.normal, normal[0], normal[1], normal[2]);
						glNormal3fv(normal);
					}

					renderVoxel((float)x * sectionScale[0], (float)y * sectionScale[1], (float)z * sectionScale[2], (1 - pitch) / 2);
				}
			}
		}
	}
	glEnd();
	glPopMatrix();
}

void VoxelRenderer::renderVoxel(float cx, float cy, float cz, float r) {
	float left = cx - r;
	float right = cx + r;
	float base = cy - r;
	float top = cy + r;
	float front = cz - r;
	float back = cz + r;
	/* Base */
	glVertex3f(left, base, front);
	glVertex3f(right, base, front);
	glVertex3f(right, base, back);
	glVertex3f(left, base, back);

	/* Back */
	glVertex3f(left, base, back);
	glVertex3f(right, base, back);
	glVertex3f(right, top, back);
	glVertex3f(left, top, back);

	/* Top */
	glVertex3f(left, top, front);
	glVertex3f(right, top, front);
	glVertex3f(right, top, back);
	glVertex3f(left, top, back);

	/* Right */
	glVertex3f(right, base, front);
	glVertex3f(right, base, back);
	glVertex3f(right, top, back);
	glVertex3f(right, top, front);

	/* Front */
	glVertex3f(left, base, front);
	glVertex3f(right, base, front);
	glVertex3f(right, top, front);
	glVertex3f(left, top, front);

	/* Left */
	glVertex3f(left, base, front);
	glVertex3f(left, base, back);
	glVertex3f(left, top, back);
	glVertex3f(left, top, front);
}
