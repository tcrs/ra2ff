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

#include "VXLFile.h"
#include "HVAFile.h"
#include <sstream>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "SDLUtils.h"
#include "Display.h"
#include "VoxelRenderer.h"
#include "Input.h"

class VoxelExplorer : public Input {
protected:
	VXLFile vxl;
	HVAFile* hva;
	VoxelRenderer renderer;
	bool doWireframe, doSolid, doNormals, doColours, doLighting, animRun;
	float rot[3], rotRate[3];
	float pos[3], posRate[3];
	float frame, frameRate;
	uint32_t firstFrameTime;
	uint64_t frameCount;
public:
	VoxelExplorer(std::string const& file, std::string const& hvaFile = "") :
			vxl(file), hva(NULL), renderer(vxl),
			doWireframe(false), doSolid(true), doNormals(true), doColours(true), doLighting(true), animRun(false),
			frame(0), frameRate(6),
			firstFrameTime(0), frameCount(0)
	{
		if(hvaFile != "") {
			hva = new HVAFile(hvaFile);
			renderer.setHVA(hva);
		}
		rot[0] = 305;
		rot[1] = 0;
		rot[2] = 46;
		pos[0] = 0;
		pos[1] = 0;
		pos[2] = 10;
		rotRate[0] = rotRate[1] = rotRate[2] = 0;
		posRate[0] = posRate[1] = posRate[2] = 0;
		DEBUG("Initialising OpenGL");
		Display::setup3D();
		Display::setBackgroundColour(1, 0, 1, 1);
		Display::clear();
		framePeriod = 0;
		VoxelRenderer::setupLighting();
	}
	virtual ~VoxelExplorer() { }

	virtual int main() {
		if(firstFrameTime == 0) {
			firstFrameTime = SDL_GetTicks();
		}
		frameCount++;
		uint32_t thisFrameTime = SDL_GetTicks();
		float fp = ((float)thisFrameTime - (float)firstFrameTime) / (1000 * (float)frameCount);
		if(!(frameCount % 10)) {
			EDEBUG("Frame period == %f, fps == %f", fp, 1 / fp);
		}
		rot[0] += rotRate[0] * fp;
		rot[1] += rotRate[1] * fp;
		rot[2] += rotRate[2] * fp;
		pos[0] += posRate[0] * fp;
		pos[1] += posRate[1] * fp;
		pos[2] += posRate[2] * fp;
		if(hva && animRun) {
			frame += frameRate * fp;
			if(frame >= (float)hva->numFrames()) {
				frame = 0;
			}
			renderer.frame = static_cast<unsigned int>(frame);
		}

		glLoadIdentity();
		gluLookAt(0, 0, -10, 0, 0, 0, 0, 1, 0);
		glTranslatef(pos[0], pos[1], pos[2]);
		glRotatef(rot[0], 1, 0, 0);
		glRotatef(rot[1], 0, 1, 0);
		glRotatef(rot[2], 0, 0, 1);
		glScalef(0.1, 0.1, 0.1);

		Display::clear();
		if(doLighting) {
			glEnable(GL_LIGHTING);
			glEnable(GL_NORMALIZE);
		} else {
			glDisable(GL_LIGHTING);
		}
		if(doSolid) {
			if(doWireframe) {
				renderer.pitch = 0.05;
			}
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glColor4f(1, 1, 1, 1);
			renderer.render(doColours, doNormals);
			renderer.pitch = 0;
		}
		if(doWireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glColor4f(1, 1, 1, 0.5);
			renderer.pitch = 0;
			renderer.render(false, false);
			glColor4f(1, 1, 1, 1);
		}
		Display::flip();
		return 0;
	}
	virtual void inputKeyDown(SDLKey key, SDLMod mod) {
		switch(key) {
			case SDLK_SPACE:
				animRun = true;
			break;
			case SDLK_w:
				posRate[2] = -6;
			break;
			case SDLK_s:
				posRate[2] = 6;
			break;
			case SDLK_a:
				rotRate[1] = 50;
			break;
			case SDLK_d:
				rotRate[1] = -50;
			break;
			case SDLK_UP:
				rotRate[0] = 50;
			break;
			case SDLK_DOWN:
				rotRate[0] = -50;
			break;
			case SDLK_LEFT:
				rotRate[2] = -50;
			break;
			case SDLK_RIGHT:
				rotRate[2] = 50;
			break;
			default:
			break;
		}
	}
	virtual void inputKeyUp(SDLKey key, SDLMod mod) {
		switch(key) {
			case SDLK_SPACE:
				animRun = false;
			break;
			case SDLK_1:
				doWireframe = !doWireframe;
			break;
			case SDLK_2:
				doSolid = !doSolid;
			break;
			case SDLK_3:
				doNormals = !doNormals;
			break;
			case SDLK_4:
				doColours = !doColours;
			break;
			case SDLK_5:
				doLighting = !doLighting;
			break;
			case SDLK_w:
			case SDLK_s:
				posRate[2] = 0;
			break;
			case SDLK_a:
			case SDLK_d:
				rotRate[1] = 0;
			break;
			case SDLK_UP:
			case SDLK_DOWN:
				rotRate[0] = 0;
			break;
			case SDLK_LEFT:
			case SDLK_RIGHT:
				rotRate[2] = 0;
			break;
			default:
			break;
		}
	}
	virtual void inputMouseMove(uint8_t button, unsigned int, unsigned int, int xrel, int yrel) {
		if(button & SDL_BUTTON(1)) {
		} else if(button & SDL_BUTTON(2)) {
		}
	}
};

int main(int argc, char** argv) {
	if(argc < 2) {
		fprintf(stderr, "Usage: (bin) <vxl-file> [<hva-file>]\n");
		return 1;
	}

	VoxelExplorer ex(argv[1], argc > 2 ? argv[2] : "");
	ex.run();
	return 0;
}
