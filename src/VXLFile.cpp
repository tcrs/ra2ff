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
#include "Exception.h"
#include "Utils.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

char const VXLFile::fileTypeText[] = "Voxel Animation";

VXLFile::VXLFile(std::string const& file) : limbHeaders(NULL), limbTailers(NULL), currentLimb(0) {
	FILE* f = fopen(file.c_str(), "rb");
	if(f == NULL) {
		throw EXCEPTION("Could not open \"%s\" (%s)", file.c_str(), strerror(errno));
	}
	Utils::FixedRead fixed(f);

	fixed.read(&header.fileType[0], 16);
	
	/* Ensure the file type header matches */
	for(unsigned int i = 0; i < 16; i++) {
		if(header.fileType[i] != fileTypeText[i]) {
			header.unknown = 0; /* Ensure the fileType string is null-terminated */
			throw EXCEPTION("fileType header field is not equal to \"%s\" (is \"%s\")", fileTypeText, header.fileType);
		}
	}

	fixed.read(&header.unknown);
	if(header.unknown != 1) {
		throw EXCEPTION("Field two of header (unknown) has value %u, should always be 1", header.unknown);
	}
	fixed.read(&header.numLimbs);
	fixed.read(&header.numLimbs2);
	if(header.numLimbs != header.numLimbs2) {
		throw EXCEPTION("Fields numLimbs and numLimbs2 in header should be equal, but have values %u and %u", header.numLimbs, header.numLimbs2);
	}

	fixed.read(&header.bodySize);
	fixed.read(&header.startPaletteRemap);
	fixed.read(&header.endPaletteRemap);

	//EDEBUG("File contains %u limbs, %u bytes of limb body data", header.numLimbs, header.bodySize);

	/* VXL files seem to have a full-depth palette */
	header.palette.setChannelDepth(8, 8, 8);
	header.palette.readPackedRGB(fixed);

	/* Allocate memory for limb headers, bodies & tailers */
	limbHeaders = new LimbHeader[header.numLimbs];
	limbBodies = new LimbBody[header.numLimbs];
	limbTailers = new LimbTailer[header.numLimbs];

	/* Read all limb headers */
	for(uint32_t i = 0; i != header.numLimbs; i++) {
		readLimbHeader(&limbHeaders[i], fixed);
	}

	/* Save file position after all limb headers (start of limb bodies) */
	long pos = fixed.pos();
	/* Skip the limb bodies for now (need the tailers first) */
	fixed.skip<uint8_t>(header.bodySize);
	/* Read all the limb tailers */
	for(uint32_t i = 0; i != header.numLimbs; i++) {
		readLimbTailer(&limbTailers[i], fixed);
	}
	/* Read all the limb bodies */
	for(unsigned int i = 0; i < header.numLimbs; i++) {
		fixed.seek(pos);
		readLimbBody(i, fixed);
	}
#ifndef NDEBUG
	std::ostringstream str;
	str << "Loaded ";
	str << header.numLimbs;
	str << " limbs from \"";
	str << file;
	str << "\" (";
	for(uint32_t i = 0; i < header.numLimbs; i++) {
		str << limbHeaders[i].name;
		if(i != header.numLimbs - 1) {
			str << ", ";
		}
	}
	str << ")";
	EDEBUG("%s", str.str().c_str());
#endif
}

/*
 * The FixedRead passed in MUST be currently seek'ed to the start of all the
 * limb body data
 */
void VXLFile::readLimbBody(uint32_t n, Utils::FixedRead& fixed) {
	long pos = fixed.pos();
	/* Skip to the start of the Span data */
	fixed.skip<uint8_t>(limbTailers[n].spanStartOff);
	/* Calculate the number of spans in the body */
	uint32_t nSpans = limbTailers[n].xSize * limbTailers[n].ySize;
	/* Allocate memory to hold the spans */
	limbBodies[n].alloc(nSpans);
	
	/* Read in the spanStart & spanEnd offsets */
	fixed.read(&limbBodies[n].spanStart[0], nSpans);
	fixed.read(&limbBodies[n].spanEnd[0], nSpans);

	/* Sanity check, based on limb tailer offsets */
	long tmp = fixed.pos() - pos;
	if(tmp != limbTailers[n].spanDataOff) {
		ERROR("Limb %u - Position in file appears to be incorrect (at %li, spanDataOff == %u)", n, tmp, limbTailers[n].spanDataOff);
	}
	LimbBody::Span::Voxel* tmpVox = new LimbBody::Span::Voxel[limbTailers[n].zSize];
	Utils::ScopedArray<LimbBody::Span::Voxel> tmpVox_free(tmpVox);
	pos = fixed.pos();
	for(uint32_t j = 0; j < nSpans; j++) {
		/* Skip any null spans */
		if(limbBodies[n].spanStart[j] == -1 || limbBodies[n].spanEnd[j] == -1) {
			//EDEBUG("Limb %u / span %u [%i, %i] - Skipping", n, j, limbBodies[n].spanStart[j], limbBodies[n].spanEnd[j]);
			continue;
		}
		if(fixed.pos() != pos + limbBodies[n].spanStart[j]) {
			//EDEBUG("Wanted to be at position %li am at %li, seeking to correct position", pos + limbBodies[n].spanStart[j], fixed.pos());
			fixed.seek(pos + limbBodies[n].spanStart[j]);
		}
		/* Decompress the voxels in this span, into the temporary voxel array */
		//EDEBUG("Limb %u / span %u - Decompressing a max of %u voxels", n, j, limbTailers[n].zSize);
		limbBodies[n].span[j].numVoxels = decompressVoxels(tmpVox, limbTailers[n].zSize, fixed);
		limbBodies[n].span[j].alloc();
		for(unsigned int k = 0; k != limbBodies[n].span[j].numVoxels; k++) {
			limbBodies[n].span[j].voxel[k] = tmpVox[k];
		}
		/* All voxel spans seem to end with 2 bytes of garbage, so eat that.  Some have more, but the seek
		 * at the top of the loop will take care of it */
		uint16_t unknown;
		fixed.read(&unknown);
	}
}

uint8_t VXLFile::decompressVoxels(LimbBody::Span::Voxel* voxel, uint8_t zSz, Utils::FixedRead& fixed) {
	unsigned int z = 0;
	uint8_t skip, nv, nv2, numZ;
	for(unsigned int i = 0; i != zSz; i++) {
		voxel[i].used = false;		/* Mark all as unused */
	}
	while(z != zSz) {
		fixed.read(&skip);
		//EDEBUG("Z %u - Skip %u", z, skip);
		z += skip;
		if(z >= zSz) {
			break;
		}
		fixed.read(&nv);
		//EDEBUG("Z %u - Read %u", z, nv);
		if(z + nv > zSz) {
			throw EXCEPTION("Z %u - Cannot write %u voxels (zSz == %u)", nv, zSz);
		}
		for(unsigned int i = 0; i != nv; i++) {
			fixed.read(&voxel[z].colour);
			fixed.read(&voxel[z].normal);
			voxel[z].used = true;
			z++;
		}
		numZ = z;
		fixed.read(&nv2);
		if(nv != nv2) {
			throw EXCEPTION("Error when decompressing voxels - head & tail voxel counts do not match (head == %u, tail == %u", nv, nv2);
		}
	}
	return numZ;
}

void VXLFile::readLimbHeader(LimbHeader* lh, Utils::FixedRead& fixed) {
	fixed.read(&lh->name[0], 16);
	bool zterm = false;
	for(unsigned int i = 0; i < 16; i++) {
		if(lh->name[i] == '\0') {
			zterm = true;
		}
	}
	if(!zterm) {
		throw EXCEPTION("Limb header name is not null-terminated");
	}
	fixed.read(&lh->number);
	fixed.read(&lh->unknown);
	fixed.read(&lh->unknown2);
	//EDEBUG("LimbHeader = { .name = \"%s\", .number = %u, .unknown = %u, .unknown2 = %u }", lh->name, lh->number, lh->unknown, lh->unknown2);
}

void VXLFile::readLimbTailer(LimbTailer* lt, Utils::FixedRead& fixed) {
	fixed.read(&lt->spanStartOff);
	fixed.read(&lt->spanEndOff);
	fixed.read(&lt->spanDataOff);
	fixed.read(&lt->scale);
	fixed.read(&lt->transform[0][0], 3 * 4);
	fixed.read(&lt->minBounds[0], 3);
	fixed.read(&lt->maxBounds[0], 3);
	fixed.read(&lt->xSize);
	fixed.read(&lt->ySize);
	fixed.read(&lt->zSize);
	fixed.read(&lt->normalType);
	if(lt->normalType != 2 && lt->normalType != 4) {
		throw EXCEPTION("Unknown normal encoding %u", lt->normalType);
	}
	//EDEBUG("LimbTailer = { .spanStartOff = %u, .spanEndOff = %u, .spanDataOff = %u, ..., .xSize = %u, .ySize = %u, .zSize = %u, .normalType = %u }",
	//	lt->spanStartOff, lt->spanEndOff, lt->spanDataOff, lt->xSize, lt->ySize, lt->zSize, lt->normalType);
}

VXLFile::~VXLFile() {
	delete[] limbHeaders;
	delete[] limbTailers;
	delete[] limbBodies;
}

bool VXLFile::getVoxel(uint8_t x, uint8_t y, uint8_t z, LimbBody::Span::Voxel* vx) {
	if(x >= limbTailers[currentLimb].xSize) {
		throw EXCEPTION("x coord %u is out of range  (%u voxels in x direction)", x, limbTailers[currentLimb].xSize);
	}
	if(y >= limbTailers[currentLimb].ySize) {
		throw EXCEPTION("y coord %u is out of range  (%u voxels in y direction)", y, limbTailers[currentLimb].ySize);
	}
	if(z >= limbTailers[currentLimb].zSize) {
		throw EXCEPTION("z coord %u is out of range  (%u voxels in z direction)", z, limbTailers[currentLimb].zSize);
	}
	unsigned int sn = (y * limbTailers[currentLimb].xSize) + x;
	if(limbBodies[currentLimb].spanStart[sn] == -1 || limbBodies[currentLimb].spanEnd[sn] == -1) {
		return false;
	}
	if(z >= limbBodies[currentLimb].span[sn].numVoxels) {
		return false;
	}
	LimbBody::Span::Voxel* tmp = &limbBodies[currentLimb].span[sn].voxel[z];
	if(tmp->used) {
		*vx = *tmp;
		return true;
	} else {
		return false;
	}
}

Palette const& VXLFile::getPalette() {
	return header.palette;
}

void VXLFile::getXYZNormal(uint8_t n, float& x, float& y, float& z) {
	if(limbTailers[currentLimb].normalType == 2) {
		if(n >= TS_NUM_NORMALS) {
			n = TS_NUM_NORMALS - 1;
		}
		x = TSNormals[n][0];
		y = TSNormals[n][1];
		z = TSNormals[n][2];
	} else if(limbTailers[currentLimb].normalType == 4) {
		if(n >= RA2_NUM_NORMALS) {
			n = RA2_NUM_NORMALS - 1;
		}
		x = RA2Normals[n][0];
		y = RA2Normals[n][1];
		z = RA2Normals[n][2];
	} else {
		throw EXCEPTION("Normal encoding scheme %u not supported", limbTailers[currentLimb].normalType);
	}
}

void VXLFile::getSize(uint8_t& x, uint8_t& y, uint8_t& z) {
	x = limbTailers[currentLimb].xSize;
	y = limbTailers[currentLimb].ySize;
	z = limbTailers[currentLimb].zSize;
}

void VXLFile::getBounds(float* min, float* max) {
	min[0] = limbTailers[currentLimb].minBounds[0];
	min[1] = limbTailers[currentLimb].minBounds[1];
	min[2] = limbTailers[currentLimb].minBounds[2];

	max[0] = limbTailers[currentLimb].maxBounds[0];
	max[1] = limbTailers[currentLimb].maxBounds[1];
	max[2] = limbTailers[currentLimb].maxBounds[2];
}

void VXLFile::getTotalBounds(float* min, float* max) {
	for(unsigned int i = 0; i != header.numLimbs; i++) {
		if(i == 0) {
			min[0] = limbTailers[i].minBounds[0];
			min[1] = limbTailers[i].minBounds[1];
			min[2] = limbTailers[i].minBounds[2];

			max[0] = limbTailers[i].maxBounds[0];
			max[1] = limbTailers[i].maxBounds[1];
			max[2] = limbTailers[i].maxBounds[2];
		} else {
			if(min[0] > limbTailers[i].minBounds[0])
				min[0] = limbTailers[i].minBounds[0];
			if(min[1] > limbTailers[i].minBounds[1])
				min[1] = limbTailers[i].minBounds[1];
			if(min[2] > limbTailers[i].minBounds[2])
				min[2] = limbTailers[i].minBounds[2];

			if(max[0] < limbTailers[i].maxBounds[0])
				max[0] = limbTailers[i].maxBounds[0];
			if(max[1] < limbTailers[i].maxBounds[1])
				max[1] = limbTailers[i].maxBounds[1];
			if(max[2] < limbTailers[i].maxBounds[2])
				max[2] = limbTailers[i].maxBounds[2];
		}
	}
}

float VXLFile::getScale() {
	return limbTailers[currentLimb].scale;
}

void VXLFile::loadGLMatrix(float* m) {
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
	m[0]  = limbTailers[currentLimb].transform[0][0];
	m[1]  = limbTailers[currentLimb].transform[1][0];
	m[2]  = limbTailers[currentLimb].transform[2][0];
	m[3]  = 0;
	m[4]  = limbTailers[currentLimb].transform[0][1];
	m[5]  = limbTailers[currentLimb].transform[1][1];
	m[6]  = limbTailers[currentLimb].transform[2][1];
	m[7]  = 0;
	m[8]  = limbTailers[currentLimb].transform[0][2];
	m[9]  = limbTailers[currentLimb].transform[1][2];
	m[10] = limbTailers[currentLimb].transform[2][2];
	m[11] = 0;
	m[12] = limbTailers[currentLimb].transform[0][3];
	m[13] = limbTailers[currentLimb].transform[1][3];
	m[14] = limbTailers[currentLimb].transform[2][3];
	m[15] = 1;
}

uint32_t VXLFile::getNumLimbs() {
	return header.numLimbs;
}

std::string VXLFile::limbName() {
	return reinterpret_cast<char const*>(limbHeaders[currentLimb].name);
}

void VXLFile::setCurrentLimb(uint32_t l) {
	if(l >= header.numLimbs) {
		throw EXCEPTION("Limb %u is too large (numLimbs == %u)", l, header.numLimbs);
	}
	currentLimb = l;
}

void VXLFile::setCurrentLimb(std::string const& name) {
	for(uint32_t i = 0; i != header.numLimbs; i++) {
		if(strcmp(reinterpret_cast<char const*>(&limbHeaders[i].name[0]), name.c_str()) == 0) {
			currentLimb = i;
			return;
		}
	}
	throw EXCEPTION("Could not find limb with name \"%s\"", name.c_str());
}

void VXLFile::writeLimbToNRRD(std::string const& file) {
	static uint8_t const emptyVoxel[5] = { 0x00, 0x00, 0x00, 0x00, 0x00, };
	FILE* f = fopen(file.c_str(), "wb");
	if(f == NULL) {
		throw EXCEPTION("Could not open \"%s\" (%s)", file.c_str(), strerror(errno));
	}
	Utils::ScopedFile f_close(f);
	
	fprintf(f, "NRRD0001\n");
	fprintf(f, "type: uint8_t\n");
	fprintf(f, "dimension: 4\n");
	fprintf(f, "# Dimensions are used/R/G/B/normal, x, y, z\n");
	fprintf(f, "sizes: 5 %u %u %u\n", limbTailers[currentLimb].xSize, limbTailers[currentLimb].ySize, limbTailers[currentLimb].zSize);
	fprintf(f, "encoding: raw\n");
	fprintf(f, "endian: little\n");
	uint8_t packedVoxel[5];
	LimbBody::Span::Voxel vx;
	for(uint32_t z = 0; z != limbTailers[currentLimb].zSize; z++) {
		for(uint32_t y = 0; y != limbTailers[currentLimb].ySize; y++) {
			for(uint32_t x = 0; x != limbTailers[currentLimb].xSize; x++) {
				if(getVoxel(x, y, z, &vx)) {
					packedVoxel[0] = 0x01;
					header.palette.getRGB(vx.colour, packedVoxel[1], packedVoxel[2], packedVoxel[3]);
					packedVoxel[4] = vx.normal;
					fwrite(&packedVoxel[0], 1, 5, f);
				} else {
					fwrite(&emptyVoxel[0], 1, 5, f);
				}
			}
		}
	}
}

void VXLFile::print() {
	printf("Contains %u limbs\n", header.numLimbs);
	for(uint32_t i = 0; i != header.numLimbs; i++) {
		printf("Limb %u - \"%s\" (number %u) %u x %u x %u\n", i, limbHeaders[i].name, limbHeaders[i].number, limbTailers[i].xSize, limbTailers[i].ySize, limbTailers[i].zSize);
		printf("  Bounding (%f, %f, %f) -> (%f, %f, %f)\n",
			limbTailers[i].minBounds[0], limbTailers[i].minBounds[1], limbTailers[i].minBounds[2],
			limbTailers[i].maxBounds[0], limbTailers[i].maxBounds[1], limbTailers[i].maxBounds[2]
		);
		printf("            |% 8.03f  % 8.03f  % 8.03f  % 8.03f|\n", limbTailers[i].transform[0][0], limbTailers[i].transform[0][1], limbTailers[i].transform[0][2], limbTailers[i].transform[0][3]); 
		printf("  Transform:|% 8.03f  % 8.03f  % 8.03f  % 8.03f| scale = %f\n", limbTailers[i].transform[1][0], limbTailers[i].transform[1][1], limbTailers[i].transform[1][2], limbTailers[i].transform[1][3], limbTailers[i].scale);
		printf("            |% 8.03f  % 8.03f  % 8.03f  % 8.03f|\n", limbTailers[i].transform[2][0], limbTailers[i].transform[2][1], limbTailers[i].transform[2][2], limbTailers[i].transform[2][3]);
	}
}

/*
 * This table of normals was taken from the OS Voxel Tools SVN code repository on 2008-08-08.
 * 
 * Thanks to: Banshee, Stucuk, Will, Koen and Plasmadroid.
 *
 * Get the latest versions of OS Voxel Tools, OS HVA Builder & other modding tools from:
 * Project Perfect Mod (http://www.ppmsite.com/)
 * CnC Source (http://www.cnc-source.com/)
 */
float const VXLFile::RA2Normals[RA2_NUM_NORMALS][3] = {
	{ 0.526578009128571, -0.359620988368988, -0.770317018032074, },
	{ 0.150481998920441, 0.43598398566246, 0.887283980846405, },
	{ 0.414195001125336, 0.738255023956299, -0.532374024391174, },
	{ 0.0751520022749901, 0.916248977184296, -0.393498003482819, },
	{ -0.316148996353149, 0.930736005306244, -0.183792993426323, },
	{ -0.773819029331207, 0.623333990573883, -0.112510003149509, },
	{ -0.900842010974884, 0.428537011146545, -0.0695680007338524, },
	{ -0.998942017555237, -0.010971000418067, 0.0446650013327599, },
	{ -0.979761004447937, -0.157670006155968, -0.123323999345303, },
	{ -0.911274015903473, -0.362370997667313, -0.195620000362396, },
	{ -0.624068975448608, -0.720941007137299, -0.301301002502441, },
	{ -0.310173004865646, -0.809345006942749, -0.498751997947693, },
	{ 0.146613001823425, -0.815819025039673, -0.559414029121399, },
	{ -0.716516017913818, -0.694356024265289, -0.0668879970908165, },
	{ 0.503971993923187, -0.114202000200748, -0.856136977672577, },
	{ 0.455491006374359, 0.872627019882202, -0.176210999488831, },
	{ -0.00500999996438622, -0.114372998476028, -0.993425011634827, },
	{ -0.104675002396107, -0.32770100235939, -0.938965022563934, },
	{ 0.560411989688873, 0.752588987350464, -0.345755994319916, },
	{ -0.0605759993195534, 0.821627974510193, -0.566796004772186, },
	{ -0.302341014146805, 0.797007024288178, -0.52284699678421, },
	{ -0.671543002128601, 0.670740008354187, -0.314862996339798, },
	{ -0.778401017189026, -0.128356993198395, 0.614504992961884, },
	{ -0.924049973487854, 0.278382003307343, -0.261985003948212, },
	{ -0.699773013591766, -0.550490975379944, -0.455278009176254, },
	{ -0.568247973918915, -0.517189025878906, -0.640007972717285, },
	{ 0.0540979988873005, -0.932864010334015, -0.356142997741699, },
	{ 0.758382022380829, 0.572893023490906, -0.31088799238205, },
	{ 0.00362000009045005, 0.305025994777679, -0.952337026596069, },
	{ -0.0608499981462956, -0.986886024475098, -0.149510994553566, },
	{ 0.635230004787445, 0.0454780012369156, -0.770982980728149, },
	{ 0.521704971790314, 0.241309002041817, -0.818287014961243, },
	{ 0.269403994083405, 0.635424971580505, -0.723640978336334, },
	{ 0.0456760004162788, 0.672753989696503, -0.73845499753952, },
	{ -0.180510997772217, 0.674656987190247, -0.715718984603882, },
	{ -0.397130995988846, 0.636640012264252, -0.661041975021362, },
	{ -0.552003979682922, 0.472514986991882, -0.687038004398346, },
	{ -0.772170007228851, 0.0830899998545647, -0.629960000514984, },
	{ -0.669818997383118, -0.119533002376556, -0.732840001583099, },
	{ -0.540454983711243, -0.318444013595581, -0.77878201007843, },
	{ -0.386135011911392, -0.522789001464844, -0.759993970394135, },
	{ -0.26146599650383, -0.688566982746124, -0.676394999027252, },
	{ -0.0194119997322559, -0.696102976799011, -0.717679977416992, },
	{ 0.303568989038467, -0.481844007968903, -0.821992993354797, },
	{ 0.681939005851746, -0.195129007101059, -0.704900026321411, },
	{ -0.244889006018639, -0.116562001407146, -0.962518990039825, },
	{ 0.800759017467499, -0.0229790005832911, -0.598546028137207, },
	{ -0.370274990797043, 0.0955839976668358, -0.923991024494171, },
	{ -0.330671012401581, -0.326577991247177, -0.885439991950989, },
	{ -0.163220003247261, -0.527579009532928, -0.833679020404816, },
	{ 0.126389995217323, -0.313145995140076, -0.941256999969482, },
	{ 0.349548012018204, -0.272226005792618, -0.896498024463654, },
	{ 0.239917993545532, -0.0858250036835671, -0.966992020606995, },
	{ 0.390845000743866, 0.0815370008349419, -0.916837990283966, },
	{ 0.2552669942379, 0.268696993589401, -0.928785026073456, },
	{ 0.146245002746582, 0.480437994003296, -0.864749014377594, },
	{ -0.326016008853912, 0.478455990552902, -0.815348982810974, },
	{ -0.46968200802803, -0.112519003450871, -0.875635981559753, },
	{ 0.818440020084381, -0.258520007133484, -0.513150990009308, },
	{ -0.474317997694015, 0.292237997055054, -0.830433011054993, },
	{ 0.778943002223969, 0.395841985940933, -0.486371010541916, },
	{ 0.624094009399414, 0.39377298951149, -0.674870014190674, },
	{ 0.740885972976685, 0.203833997249603, -0.639953017234802, },
	{ 0.480217009782791, 0.565768003463745, -0.670297026634216, },
	{ 0.380930006504059, 0.424535006284714, -0.821377992630005, },
	{ -0.0934220030903816, 0.501124024391174, -0.860318005084991, },
	{ -0.236485004425049, 0.296198010444641, -0.925387024879456, },
	{ -0.131531000137329, 0.0939590036869049, -0.986849009990692, },
	{ -0.823562026023865, 0.29577699303627, -0.484005987644196, },
	{ 0.611065983772278, -0.624303996562958, -0.486663997173309, },
	{ 0.0694959983229637, -0.520330011844635, -0.851132988929748, },
	{ 0.226521998643875, -0.664879024028778, -0.711775004863739, },
	{ 0.471307992935181, -0.568903982639313, -0.673956990242004, },
	{ 0.38842499256134, -0.74262398481369, -0.545560002326965, },
	{ 0.783675014972687, -0.480729013681412, -0.393384993076324, },
	{ 0.962393999099731, 0.135675996541977, -0.235348999500275, },
	{ 0.876607000827789, 0.172033995389938, -0.449405997991562, },
	{ 0.633405029773712, 0.589793026447296, -0.500940978527069, },
	{ 0.182275995612144, 0.800657987594605, -0.570720970630646, },
	{ 0.177002996206284, 0.764133989810944, 0.620297014713287, },
	{ -0.544016003608704, 0.675514996051788, -0.497720986604691, },
	{ -0.679296970367432, 0.286466985940933, -0.675642013549805, },
	{ -0.590390980243683, 0.0913690030574799, -0.801928997039795, },
	{ -0.824360013008118, -0.133123993873596, -0.550189018249512, },
	{ -0.715794026851654, -0.334542006254196, -0.612960994243622, },
	{ 0.174285992980003, -0.8924840092659, 0.416049003601074, },
	{ -0.0825280025601387, -0.837122976779938, -0.54075300693512, },
	{ 0.283331006765366, -0.88087397813797, -0.379189014434814, },
	{ 0.675134003162384, -0.42662701010704, -0.601817011833191, },
	{ 0.843720018863678, -0.512335002422333, -0.16015599668026, },
	{ 0.977303981781006, -0.0985559970140457, -0.187519997358322, },
	{ 0.84629499912262, 0.52267199754715, -0.102946996688843, },
	{ 0.677141010761261, 0.721324980258942, -0.145501002669334, },
	{ 0.320964992046356, 0.870891988277435, -0.372193992137909, },
	{ -0.178977996110916, 0.911532998085022, -0.37023600935936, },
	{ -0.447169005870819, 0.826700985431671, -0.341473996639252, },
	{ -0.703203022480011, 0.496327996253967, -0.50908100605011, },
	{ -0.977181017398834, 0.0635629966855049, -0.202674001455307, },
	{ -0.878170013427734, -0.412937998771667, 0.241455003619194, },
	{ -0.835830986499786, -0.358550012111664, -0.415728002786636, },
	{ -0.499173998832703, -0.693432986736298, -0.519591987133026, },
	{ -0.188788995146751, -0.923753023147583, -0.333225011825562, },
	{ 0.19225400686264, -0.969361007213593, -0.152896001935005, },
	{ 0.515940010547638, -0.783906996250153, -0.345391988754272, },
	{ 0.90592497587204, -0.300951987504959, -0.297870993614197, },
	{ 0.991111993789673, -0.127746000885963, 0.0371069982647896, },
	{ 0.995135009288788, 0.0984240025281906, -0.0043830000795424, },
	{ 0.760123014450073, 0.646277010440826, 0.0673670023679733, },
	{ 0.205220997333527, 0.95958000421524, -0.192590996623039, },
	{ -0.0427500009536743, 0.979512989521027, -0.196790993213654, },
	{ -0.438017010688782, 0.898926973342895, 0.00849200040102005, },
	{ -0.821994006633759, 0.480785012245178, -0.305238991975784, },
	{ -0.899917006492615, 0.0817100033164024, -0.428337007761002, },
	{ -0.926612019538879, -0.144618004560471, -0.347095996141434, },
	{ -0.79365998506546, -0.557792007923126, -0.242838993668556, },
	{ -0.431349992752075, -0.847778975963593, -0.308557987213135, },
	{ -0.00549199990928173, -0.964999973773956, 0.262192994356155, },
	{ 0.587904989719391, -0.804026007652283, -0.0889400020241737, },
	{ 0.699492990970612, -0.667685985565186, -0.254765003919601, },
	{ 0.889303028583527, 0.35979500412941, -0.282290995121002, },
	{ 0.780972003936768, 0.197036996483803, 0.592671990394592, },
	{ 0.520120978355408, 0.506695985794067, 0.687556982040405, },
	{ 0.403894990682602, 0.693961024284363, 0.59605997800827, },
	{ -0.154982998967171, 0.899236023426056, 0.409090012311935, },
	{ -0.65733802318573, 0.537168025970459, 0.528542995452881, },
	{ -0.746195018291473, 0.334091007709503, 0.57582700252533, },
	{ -0.624952018260956, -0.0491439998149872, 0.77911502122879, },
	{ 0.318141013383865, -0.254714995622635, 0.913185000419617, },
	{ -0.555896997451782, 0.405294001102447, 0.725751996040344, },
	{ -0.794434010982513, 0.0994059965014458, 0.599160015583038, },
	{ -0.64036101102829, -0.689463019371033, 0.3384949862957, },
	{ -0.126712992787361, -0.734094977378845, 0.667119979858398, },
	{ 0.105457000434399, -0.780816972255707, 0.615795016288757, },
	{ 0.407992988824844, -0.480915993452072, 0.776054978370666, },
	{ 0.69513601064682, -0.545120000839233, 0.468647003173828, },
	{ 0.973191022872925, -0.00648899981752038, 0.229908004403114, },
	{ 0.946893990039825, 0.31750899553299, -0.0507990010082722, },
	{ 0.563583016395569, 0.825612008571625, 0.0271829999983311, },
	{ 0.325773000717163, 0.945423007011414, 0.00694900006055832, },
	{ -0.171820998191834, 0.985096991062164, -0.00781499966979027, },
	{ -0.670440971851349, 0.739938974380493, 0.0547689981758594, },
	{ -0.822980999946594, 0.554961979389191, 0.121321998536587, },
	{ -0.96619302034378, 0.117857001721859, 0.229306995868683, },
	{ -0.953769028186798, -0.294703990221024, 0.0589450001716614, },
	{ -0.864386975765228, -0.50272798538208, -0.0100149996578693, },
	{ -0.530609011650085, -0.842006027698517, -0.0973659977316856, },
	{ -0.16261799633503, -0.984075009822845, 0.071772001683712, },
	{ 0.081446997821331, -0.996011018753052, 0.0364390015602112, },
	{ 0.745984017848968, -0.665962994098663, 0.000761999981477857, },
	{ 0.942057013511658, -0.329268991947174, -0.0641060024499893, },
	{ 0.939701974391937, -0.2810899913311, 0.19480299949646, },
	{ 0.771214008331299, 0.550670027732849, 0.319362998008728, },
	{ 0.641348004341126, 0.730690002441406, 0.234020993113518, },
	{ 0.0806820020079613, 0.996690988540649, 0.00987899955362082, },
	{ -0.0467250011861324, 0.976643025875092, 0.209725007414818, },
	{ -0.531076014041901, 0.821000993251801, 0.209562003612518, },
	{ -0.695815026760101, 0.65599000453949, 0.292434990406036, },
	{ -0.97612202167511, 0.21670900285244, -0.0149130001664162, },
	{ -0.961660981178284, -0.144128993153572, 0.233313992619514, },
	{ -0.77208399772644, -0.613646984100342, 0.165298998355865, },
	{ -0.449600011110306, -0.836059987545013, 0.314426004886627, },
	{ -0.392699986696243, -0.914615988731384, 0.0962470024824142, },
	{ 0.390588998794556, -0.919470012187958, 0.0448900014162064, },
	{ 0.582529008388519, -0.799197971820831, 0.148127004504204, },
	{ 0.866430997848511, -0.489811986684799, 0.0968639999628067, },
	{ 0.904586970806122, 0.11149799823761, 0.411449998617172, },
	{ 0.953536987304687, 0.232329994440079, 0.191806003451347, },
	{ 0.497310996055603, 0.770802974700928, 0.398176997900009, },
	{ 0.194066002964973, 0.956319987773895, 0.218611001968384, },
	{ 0.422876000404358, 0.882275998592377, 0.206797003746033, },
	{ -0.373796999454498, 0.849565982818604, 0.372173994779587, },
	{ -0.534497022628784, 0.714022994041443, 0.452199995517731, },
	{ -0.881826996803284, 0.237159997224808, 0.407597988843918, },
	{ -0.904947996139526, -0.0140690002590418, 0.425289005041122, },
	{ -0.751827001571655, -0.512817025184631, 0.414458006620407, },
	{ -0.50101500749588, -0.697916984558105, 0.511758029460907, },
	{ -0.235190004110336, -0.925922989845276, 0.295554995536804, },
	{ 0.228982999920845, -0.953939974308014, 0.193819001317024, },
	{ 0.734025001525879, -0.634898006916046, 0.241062000393867, },
	{ 0.913752973079681, -0.0632530003786087, -0.401315987110138, },
	{ 0.905735015869141, -0.161486998200417, 0.391874998807907, },
	{ 0.858929991722107, 0.342445999383926, 0.380748987197876, },
	{ 0.624486029148102, 0.60758101940155, 0.490776985883713, },
	{ 0.289263993501663, 0.857478976249695, 0.425507992506027, },
	{ 0.0699680000543594, 0.902168989181519, 0.425671011209488, },
	{ -0.28617998957634, 0.940699994564056, 0.182164996862411, },
	{ -0.574012994766235, 0.805118978023529, -0.149308994412422, },
	{ 0.111258000135422, 0.0997179970145225, -0.988776028156281, },
	{ -0.305393010377884, -0.944227993488312, -0.123159997165203, },
	{ -0.601166009902954, -0.78957599401474, 0.123162999749184, },
	{ -0.290645003318787, -0.812139987945557, 0.505918979644775, },
	{ -0.064920000731945, -0.877162992954254, 0.475784987211227, },
	{ 0.408300995826721, -0.862215995788574, 0.299789011478424, },
	{ 0.566097021102905, -0.725566029548645, 0.391263991594315, },
	{ 0.839363992214203, -0.427386999130249, 0.335869014263153, },
	{ 0.818899989128113, -0.0413050018250942, 0.572448015213013, },
	{ 0.719784021377564, 0.414997011423111, 0.556496977806091, },
	{ 0.881744027137756, 0.450269997119904, 0.140659004449844, },
	{ 0.40182301402092, -0.898220002651215, -0.178151994943619, },
	{ -0.0540199987590313, 0.791343986988068, 0.608980000019074, },
	{ -0.293774008750916, 0.763993978500366, 0.574464976787567, },
	{ -0.450798004865646, 0.610346972942352, 0.651350975036621, },
	{ -0.638221025466919, 0.186693996191025, 0.746873021125793, },
	{ -0.872870028018951, -0.257126986980438, 0.414707988500595, },
	{ -0.587257027626038, -0.521709978580475, 0.618827998638153, },
	{ -0.353657990694046, -0.641973972320557, 0.680290997028351, },
	{ 0.0416489988565445, -0.611272990703583, 0.79032301902771, },
	{ 0.348342001438141, -0.779182970523834, 0.521086990833282, },
	{ 0.499166995286942, -0.622440993785858, 0.602825999259949, },
	{ 0.790018975734711, -0.3038310110569, 0.53250002861023, },
	{ 0.660117983818054, 0.0607330016791821, 0.748701989650726, },
	{ 0.604920983314514, 0.29416099190712, 0.739960014820099, },
	{ 0.38569700717926, 0.379346013069153, 0.841032028198242, },
	{ 0.239693000912666, 0.207875996828079, 0.948332011699677, },
	{ 0.012622999958694, 0.258531987667084, 0.965919971466065, },
	{ -0.100556999444962, 0.457147002220154, 0.883687973022461, },
	{ 0.0469669997692108, 0.628588020801544, 0.776319026947021, },
	{ -0.430391013622284, -0.445405006408691, 0.785097002983093, },
	{ -0.434291005134583, -0.196227997541428, 0.879139006137848, },
	{ -0.256637006998062, -0.33686700463295, 0.905902028083801, },
	{ -0.131372004747391, -0.158910006284714, 0.978514015674591, },
	{ 0.102379001677036, -0.208766996860504, 0.972591996192932, },
	{ 0.195686995983124, -0.450129002332687, 0.871258020401001, },
	{ 0.627318978309631, -0.42314800620079, 0.653770983219147, },
	{ 0.687439024448395, -0.171582996845245, 0.70568197965622, },
	{ 0.275920003652573, -0.021254999563098, 0.960946023464203, },
	{ 0.459367007017136, 0.157465994358063, 0.874177992343903, },
	{ 0.285394996404648, 0.583184003829956, 0.760555982589722, },
	{ -0.812174022197723, 0.460303008556366, 0.358460992574692, },
	{ -0.189068004488945, 0.641223013401032, 0.743698000907898, },
	{ -0.338874995708466, 0.476480007171631, 0.811251997947693, },
	{ -0.920993983745575, 0.347185999155045, 0.176726996898651, },
	{ 0.0406389981508255, 0.024465000256896, 0.998874008655548, },
	{ -0.739131987094879, -0.353747010231018, 0.573189973831177, },
	{ -0.603511989116669, -0.286615014076233, 0.744059979915619, },
	{ -0.188675999641418, -0.547058999538422, 0.815554022789001, },
	{ -0.0260450001806021, -0.397819995880127, 0.917093992233276, },
	{ 0.267897009849548, -0.649040997028351, 0.712023019790649, },
	{ 0.518245995044708, -0.28489100933075, 0.806385993957519, },
	{ 0.493450999259949, -0.0665329992771149, 0.867224991321564, },
	{ -0.328188002109528, 0.140250995755196, 0.934143006801605, },
	{ -0.328188002109528, 0.140250995755196, 0.934143006801605, },
	{ -0.328188002109528, 0.140250995755196, 0.934143006801605, },
	{ -0.328188002109528, 0.140250995755196, 0.934143006801605, },
};

float const VXLFile::TSNormals[TS_NUM_NORMALS][3] = {
	{ 0.671213984489441, 0.198492005467415, -0.714193999767303, },
	{ 0.269643008708954, 0.584393978118897, -0.765359997749329, },
	{ -0.0405460000038147, 0.0969879999756813, -0.994458973407745, },
	{ -0.572427988052368, -0.0919139981269836, -0.814786970615387, },
	{ -0.171400994062424, -0.572709977626801, -0.801639020442963, },
	{ 0.362556993961334, -0.30299898982048, -0.881331026554108, },
	{ 0.810347020626068, -0.348971992731094, -0.470697999000549, },
	{ 0.103961996734142, 0.938672006130218, -0.328767001628876, },
	{ -0.32404699921608, 0.587669014930725, -0.741375982761383, },
	{ -0.800864994525909, 0.340460985898972, -0.492646992206574, },
	{ -0.665498018264771, -0.590147018432617, -0.456988990306854, },
	{ 0.314767003059387, -0.803001999855042, -0.506072998046875, },
	{ 0.972629010677338, 0.151076003909111, -0.176550000905991, },
	{ 0.680290997028351, 0.684235990047455, -0.262726992368698, },
	{ -0.520079016685486, 0.827777028083801, -0.210482999682426, },
	{ -0.961643993854523, -0.179001003503799, -0.207846999168396, },
	{ -0.262713998556137, -0.937451004981995, -0.228401005268097, },
	{ 0.219706997275352, -0.971301019191742, 0.0911249965429306, },
	{ 0.923807978630066, -0.229975000023842, 0.306086987257004, },
	{ -0.0824889987707138, 0.970659971237183, 0.225866004824638, },
	{ -0.591798007488251, 0.696789979934692, 0.405288994312286, },
	{ -0.925296008586884, 0.36660099029541, 0.0971110016107559, },
	{ -0.705051004886627, -0.687775015830994, 0.172828003764153, },
	{ 0.732400000095367, -0.680366992950439, -0.0263049993664026, },
	{ 0.855162024497986, 0.37458199262619, 0.358310997486114, },
	{ 0.473006010055542, 0.836480021476746, 0.276704996824265, },
	{ -0.0976170003414154, 0.654111981391907, 0.750072002410889, },
	{ -0.904124021530151, -0.153724998235703, 0.398658007383347, },
	{ -0.211915999650955, -0.858089983463287, 0.467732012271881, },
	{ 0.500226974487305, -0.67440801858902, 0.543090999126434, },
	{ 0.584538996219635, -0.110248997807503, 0.8038409948349, },
	{ 0.437373012304306, 0.454643994569778, 0.775888979434967, },
	{ -0.0424409992992878, 0.0833180025219917, 0.995618999004364, },
	{ -0.596251010894775, 0.220131993293762, 0.772028028964996, },
	{ -0.50645500421524, -0.396977007389069, 0.765448987483978, },
	{ 0.0705690011382103, -0.478473991155624, 0.875262022018433, },
};
