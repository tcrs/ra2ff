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

#ifndef VXLFILE_H__
#define VXLFILE_H__

#include <stdint.h>
#include <string>
#include "Utils.h"
#include "Palette.h"

#define RA2_NUM_NORMALS		244
#define TS_NUM_NORMALS		36

class VXLFile {
public:
	static char const fileTypeText[];
	static float const RA2Normals[RA2_NUM_NORMALS][3];
	static float const TSNormals[TS_NUM_NORMALS][3];
	struct Header {
		uint8_t fileType[16];		/* "Voxel Animation" */
		uint32_t unknown;		/* == 1 */
		uint32_t numLimbs;		/* Number of limbs/bodies/tailers */
		uint32_t numLimbs2;		/* == numLimbs */
		uint32_t bodySize;		/* Total size in bytes of all limb bodies */
		uint8_t startPaletteRemap;	/* (?) Palette remapping for player colours (?) */
		uint8_t endPaletteRemap;		
		Palette palette;		/* RGB colour palette */
	};
	struct LimbHeader {
		uint8_t name[16];		/* Limb name (Zero terminated) */
		uint32_t number;		/* Limb number */
		uint32_t unknown;		/* == 1 */
		uint32_t unknown2;		/* == 0 or == 2 (Documentation is contradictory) */
	};
	struct LimbBody {
		int32_t* spanStart;
		int32_t* spanEnd;
		struct Span {
			uint8_t numVoxels;
			struct Voxel {
				uint8_t colour;
				uint8_t normal;
				bool used;
				Voxel() : used(true) { }
			} *voxel;
			Span() : voxel(NULL) { }
			~Span() { delete[] voxel; }
			void alloc() {
				delete[] voxel;
				voxel = new Voxel[numVoxels];
			}
		} *span;
		LimbBody() : spanStart(NULL), spanEnd(NULL), span(NULL) { }
		~LimbBody() {
			delete[] spanStart;
			delete[] spanEnd;
			delete[] span;
		}
		void alloc(unsigned int n) {
			delete[] spanStart;
			spanStart = new int32_t[n];
			delete[] spanEnd;
			spanEnd = new int32_t[n];
			delete[] span;
			span = new Span[n];
		}
	};
	struct LimbTailer {
		uint32_t spanStartOff;		/* Offset into body section to span start list */
		uint32_t spanEndOff;		/* Offset into body section to span end list */
		uint32_t spanDataOff;		/* Offset into body section to span data */
		float scale;			/* Scale factor for the section always seems to be 0.083333 */
		float transform[3][4];		/* Transformation matrix */
		float minBounds[3];		/* Voxel bounding box */
		float maxBounds[3];
		uint8_t xSize;			/* Width of voxel limb */
		uint8_t ySize;			/* Breadth of voxel limb */
		uint8_t zSize;			/* Height of voxel limb */
		uint8_t normalType;		/* 2 == TS Normals, 4 == RA2 Normals */
	};

protected:
	Header header;
	LimbHeader* limbHeaders;
	LimbBody* limbBodies;
	LimbTailer* limbTailers;

	uint32_t currentLimb;

	void readPalette(Utils::FixedRead&);
	void readLimbHeader(LimbHeader*, Utils::FixedRead&);
	void readLimbBody(uint32_t, Utils::FixedRead&);
	void readLimbTailer(LimbTailer*, Utils::FixedRead&);
	uint8_t decompressVoxels(LimbBody::Span::Voxel*, uint8_t, Utils::FixedRead&);
public:
	VXLFile(std::string const&);
	~VXLFile();

	bool getVoxel(uint8_t, uint8_t, uint8_t, LimbBody::Span::Voxel*);
	Palette const& getPalette();
	void getXYZNormal(uint8_t, float&, float&, float&);
	void getSize(uint8_t&, uint8_t&, uint8_t&);
	std::string limbName();
	void getBounds(float*, float*);
	void getTotalBounds(float*, float*);
	float getScale();
	void loadGLMatrix(float*);

	uint32_t getNumLimbs();

	void setCurrentLimb(uint32_t);
	void setCurrentLimb(std::string const&);

	void writeLimbToNRRD(std::string const&);

	void print();
};

#endif
