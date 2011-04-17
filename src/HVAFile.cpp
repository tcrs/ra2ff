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

#include "HVAFile.h"

HVAFile::HVAFile(std::string const& file) : sections(NULL), currentSection(0) {
	FILE* f = fopen(file.c_str(), "rb");
	if(f == NULL) {
		throw EXCEPTION("Could not open \"%s\" (%s)", file.c_str(), strerror(errno));
	}
	Utils::FixedRead fixed(f);

	fixed.read(&header.fileName[0], 16);
	fixed.read(&header.numFrames);
	fixed.read(&header.numSections);

	EDEBUG("File \"%s\" contains %u frames, and manipulates %u sections", file.c_str(), header.numFrames, header.numSections);

	sections = new Section[header.numSections];

	/* Read in section names */
	for(uint32_t i = 0; i != header.numSections; i++) {
		fixed.read(&sections[i].name[0], 16);
		sections[i].alloc(header.numFrames);
	}

	/* Read in matrices.  They are stored packed:
	 * <Frame0/Section0>...<Frame0/SectionN><Frame1/Section0>...<Frame1/SectionN>...<FrameN/SectionN>
	 */
	for(uint32_t i = 0; i != header.numFrames; i++) {
		for(uint32_t j = 0; j != header.numSections; j++) {
			fixed.read(&sections[j].matrices[i]);
		}
	}
}

HVAFile::~HVAFile() {
	delete[] sections;
}

void HVAFile::setCurrentSection(std::string const& name) {
	for(uint32_t i = 0; i != header.numSections; i++) {
		if(strcmp(reinterpret_cast<char const*>(&sections[i].name[0]), name.c_str()) == 0) {
			currentSection = i;
			return;
		}
	}
	throw EXCEPTION("Could not find section with name \"%s\"", name.c_str());
}

uint32_t HVAFile::numFrames() {
	return header.numFrames;
}

void HVAFile::loadGLMatrix(uint32_t frame, float* m) {
	/* OpenGL matrices are laid out thus:
	 * [ 0] [ 4] [ 8] [12]
	 * [ 1] [ 5] [ 9] [13]
	 * [ 2] [ 6] [10] [14]
	 * [ 3] [ 7] [11] [15]
	 *
	 * We need to load the matrix with
	 * [0][0] [0][1] [0][2] [0][3]
	 * [1][0] [1][1] [1][2] [1][3]
	 * [2][0] [2][1] [2][2] [2][3]
	 *    0      0      0      1
	 */
	if(frame >= header.numFrames) {
		throw EXCEPTION("Frame %u is out of range (there are only %u frames)", frame, header.numFrames);
	}
	Section::TMatrix* tm = &sections[currentSection].matrices[frame];
	m[0]  = (*tm)[0][0];
	m[1]  = (*tm)[1][0];
	m[2]  = (*tm)[2][0];
	m[3]  = 0;
	m[4]  = (*tm)[0][1];
	m[5]  = (*tm)[1][1];
	m[6]  = (*tm)[2][1];
	m[7]  = 0;
	m[8]  = (*tm)[0][2];
	m[9]  = (*tm)[1][2];
	m[10] = (*tm)[2][2];
	m[11] = 0;
	m[12] = (*tm)[0][3];
	m[13] = (*tm)[1][3];
	m[14] = (*tm)[2][3];
	m[15] = 1;
}

void HVAFile::print() {
	printf("Contains %u frames for %u sections\n", header.numFrames, header.numSections);
	for(uint32_t i = 0; i != header.numSections; i++) {
		printf("Section %s\n", (char const*)sections[i].name);
		for(uint32_t j = 0; j != header.numFrames; j++) {
			printf("            |% 8.03f  % 8.03f  % 8.03f  % 8.03f|       _  |% 8.03f|\n",
				sections[i].matrices[j][0][0], sections[i].matrices[j][0][1], sections[i].matrices[j][0][2], sections[i].matrices[j][0][3], sections[i].matrices[j][0][3] * 0.083333); 
			printf("  Frame %03u:|% 8.03f  % 8.03f  % 8.03f  % 8.03f| x 0.083  |% 8.03f|\n",
				j, sections[i].matrices[j][1][0], sections[i].matrices[j][1][1], sections[i].matrices[j][1][2], sections[i].matrices[j][1][3], sections[i].matrices[j][1][3] * 0.083333);
			printf("            |% 8.03f  % 8.03f  % 8.03f  % 8.03f|          |% 8.03f|\n",
				sections[i].matrices[j][2][0], sections[i].matrices[j][2][1], sections[i].matrices[j][2][2], sections[i].matrices[j][2][3], sections[i].matrices[j][2][3] * 0.083333);
			printf("             --------------------------------------\n");
		}
	}
}
