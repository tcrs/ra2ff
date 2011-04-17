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

#ifndef HVAFILE_H__
#define HVAFILE_H__

#include "Utils.h"
#include <stdint.h>
#include <string>

class HVAFile {
public:
	struct Header {
		uint8_t fileName[16];
		uint32_t numFrames;
		uint32_t numSections;
	};
	struct Section {
		uint8_t name[16];
		typedef float TMatrix[3][4];
		TMatrix* matrices;

		Section() : matrices(NULL) { }
		~Section() { delete[] matrices; }
		void alloc(uint32_t n) {
			matrices = new TMatrix[n];
		}
	};
protected:
	Header header;
	Section* sections;

	uint32_t currentSection;
public:
	HVAFile(std::string const&);
	~HVAFile();

	void loadGLMatrix(uint32_t, float*);
	void setCurrentSection(std::string const&);
	uint32_t numFrames();

	void print();
};

#endif
