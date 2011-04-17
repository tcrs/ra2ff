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

#include "MapReader.h"
#include "LZODecompress.h"
#include <vector>
#include <stdio.h>

int const MapReader::LZOPack = 1;
int const MapReader::F80Pack = 2;

/* This code is a reimplementation of the algorithm written by
 * Olaf van der Spek in the XCC code (map_ts_encoder.cpp, function
 * preview_decode4)
 */
size_t MapReader::decode4(uint16_t* buf, size_t bufLen, uint8_t* out, size_t outLen, Palette& pal) {
	std::vector<uint64_t> colLookup;
	uint64_t cp = 0;
	uint8_t rgb[3];
	for(unsigned int i = 0; i != 256; i++) {
		pal.getRGB(i, rgb[0], rgb[1], rgb[2]);
		for(unsigned int j = 0; j != 3; j++) {
			cp |= rgb[j];
			cp <<= 8;
		}
		if(i % 2) {
			colLookup.push_back(cp >> 8);
			cp = 0;
		}
	}
	size_t bufPos = 0;
	uint16_t numColours = buf[bufPos++];
	if(bufPos + (numColours * 6) >= bufLen) {
		throw EXCEPTION("Reading %u additional colour pairs would overrun input data");
	}
	EDEBUG("Reading %u additional colour pairs", numColours);
	for(unsigned int i = 0; i != numColours; i++) {
		for(unsigned int j = 0; j != 6; j++) {
			cp |= buf[bufPos++];
			cp <<= 8;
		}
		colLookup.push_back(cp >> 8);
		cp = 0;
	}
	if(bufPos >= bufLen) {
		throw EXCEPTION("Overran data when reading additional palette data");
	}
	size_t outPos = 0;
	uint16_t lookup;
	while(bufPos != bufLen) {
		lookup = buf[bufPos++];
		if(lookup >= colLookup.size()) {
			ERROR("Position %lu - Cannot lookup colour pair %u as there are only %lu pairs in table",
				bufPos - 1, lookup, colLookup.size());
			cp = 0;
		} else {
			cp = colLookup[lookup];
		}
		if(outPos + 6 >= outLen) {
			throw EXCEPTION("Overran output buffer when decoding");
		}
		out[outPos++] = cp >> 40 & 0xFF;
		out[outPos++] = cp >> 32 & 0xFF;
		out[outPos++] = cp >> 24 & 0xFF;
		out[outPos++] = cp >> 16 & 0xFF;
		out[outPos++] = cp >> 8 & 0xFF;
		out[outPos++] = cp & 0xFF;
	}
	return outPos;
}

void MapReader::readIsoMapPack(uint8_t* isoData, size_t len) {
	if(len % 11 != 0) {
		ERROR("%s", "isoData is not a multiple of 11 bytes long");
	}
	numEntries = len / 11;
	entry = new Entry[numEntries];
	uint8_t* cur = isoData;
	for(uint32_t i = 0; i != numEntries; i++) {
		entry[i].x = *reinterpret_cast<uint16_t const*>(cur);
		cur += 2;
		entry[i].y = *reinterpret_cast<uint16_t const*>(cur);
		cur += 2;
		entry[i].tile = *reinterpret_cast<uint16_t const*>(cur);
		cur += 2;
		entry[i].zero1[0] = *cur++;
		entry[i].zero1[1] = *cur++;
		entry[i].subTile = *cur++;
		entry[i].z = *cur++;
		entry[i].zero2 = *cur++;
	}
}

static uint16_t read16(uint8_t const* buf) {
	return *reinterpret_cast<uint16_t const*>(buf);
}

size_t MapReader::decode80(uint8_t const* in, uint8_t* out, size_t inLen, size_t outLen) {
	size_t inpos = 0, outpos = 0;

	uint8_t cmd, byte;
	size_t count, pos;
	uint8_t action;
	while(inpos < inLen) {
		cmd = in[inpos++];
		if(cmd == 0xFF) { // copy count16 from abs pos16
			count = read16(&in[inpos]);
			inpos += 2;
			pos = read16(&in[inpos]);
			inpos += 2;
			action = 0;
			EDEBUG("in %lu / %lu, out %lu / %lu [CMD %02X c16a16 - COPY %lu from %lu]",
				inpos, inLen, outpos, outLen,
				cmd, count, pos
			);
		} else if(cmd == 0xFE) { // write count16 pixel8's
			count = read16(&in[inpos]);
			inpos += 2;
			byte = in[inpos++];
			action = 2;
			EDEBUG("in %lu / %lu, out %lu / %lu [CMD %02X c16w8 - WRITE %lu 0x%02X]",
				inpos, inLen, outpos, outLen,
				cmd, count, byte
			);
		} else if((cmd & 0xC0) == 0xC0) { // copy (count6 + 3) from abs pos16
			count = cmd & 0x3F;
			count += 3;
			pos = read16(&in[inpos]);
			inpos += 2;
			action = 0;
			EDEBUG("in %lu / %lu, out %lu / %lu [CMD %02X c6a16 - COPY %lu from %lu]",
				inpos, inLen, outpos, outLen,
				cmd, count, pos
			);
		} else if((cmd & 0xC0) == 0x80) { // copy count6 bytes from source
			count = cmd & 0x3F;
			action = 1;
			if(count == 0) {
				EDEBUG("in %lu / %lu, out %lu / %lu [CMD %02X terminate]",
					inpos, inLen, outpos, outLen,
					cmd
				);
				break;
			} else {
				EDEBUG("in %lu / %lu, out %lu / %lu [CMD %02X c6s - COPY %lu from source]",
					inpos, inLen, outpos, outLen,
					cmd, count
				);
			}
		} else if((cmd & 0x80) == 0x00) { // copy (count3 + 3) from rel pos12
			count = (cmd & 0x70) >> 4;
			count += 3;
			pos = (cmd & 0x0F) << 4;
			pos |= in[inpos++];
			pos = outpos - pos;
			action = 0;
			EDEBUG("in %lu / %lu, out %lu / %lu [CMD %02X c3r12 - COPY %lu from %lu]",
				inpos, inLen, outpos, outLen,
				cmd, count, pos
			);
		} else {
			throw EXCEPTION("Unknown command 0x%02X", cmd);
		}
		if(action == 0) {
			if(pos >= outLen || pos + count >= outLen) {
				throw EXCEPTION("Cannot copy %lu bytes from %lu, only %lu bytes in buffer",
					count, pos, outLen
				);
			}
			if(outpos >= outLen || outpos + count > outLen) {
				throw EXCEPTION("Cannot write %lu bytes to output, (position = %lu, length = %lu)",
					count, outpos, outLen
				);
			}
			for(size_t i = pos; i != pos + count; i++) {
				out[outpos++] = out[i];
			}
		} else if(action == 1) {
			if(inpos >= inLen || inpos + count > inLen) {
				throw EXCEPTION("Cannot copy %lu bytes from source, would overflow", count);
			}
			if(outpos >= outLen || outpos + count > outLen) {
				throw EXCEPTION("Cannot write %lu bytes to output, (position = %lu, length = %lu)",
					count, outpos, outLen
				);
			}
			memcpy(&out[outpos], &in[inpos], count);
			inpos += count;
			outpos += count;
		} else if(action == 2) {
			if(outpos >= outLen || outpos + count > outLen) {
				throw EXCEPTION("Cannot write %lu bytes to output, (position = %lu, length = %lu)",
					count, outpos, outLen
				);
			}
			memset(&out[outpos], byte, count);
			outpos += count;
		} else {
			throw EXCEPTION("Unknown action %u", action);
		}
	}
	return outpos;
}

uint8_t* MapReader::unpack(uint8_t const* in, size_t inLen, size_t& outLen, int format) {
	typedef std::vector<PackSectionInfo> SectionVec;

	PackSectionInfo section;
	SectionVec sections;

	outLen = 0;
	uint8_t const* cur = in;
	uint8_t const* cure = in + inLen;
	while(cur < cure) {
		if(cure - cur < 4) {
			throw EXCEPTION("Ran out of input data when reading packs");
		}
		section.packedLen = *reinterpret_cast<uint16_t const*>(cur);
		cur += 2;
		section.unpackedLen = *reinterpret_cast<uint16_t const*>(cur);
		cur += 2;
		EDEBUG("Pack length %u/%u", section.packedLen, section.unpackedLen);
		if(cure - cur < section.packedLen) {
			throw EXCEPTION("Not enough input data remaining to read %u bytes of packed data (%li bytes left)",
				section.packedLen, cure - cur
			);
		}
		if(section.packedLen > section.unpackedLen) {
			throw EXCEPTION("packedLen > unpackedLen");
		}
		section.packedData = cur;
		cur += section.packedLen;
		outLen += section.unpackedLen;
		sections.push_back(section);
	}
	uint8_t* out = new uint8_t[outLen];
	size_t sz;
	uint8_t* curOut = out;
	for(SectionVec::iterator it = sections.begin(); it != sections.end(); it++) {
		if(format == LZOPack) {
			sz = LZODecompress::decompress(it->packedData, curOut, it->packedLen, it->unpackedLen);
		} else if(format == F80Pack) {
			sz = decode80(it->packedData, curOut, it->packedLen, it->unpackedLen);
		} else {
			throw EXCEPTION("Unknown pack compression format %i", format);
		}
		curOut += sz;
		if(sz != it->unpackedLen) {
			delete[] out;
			throw EXCEPTION("Wanted %u bytes of data, only %lu unpacked", it->unpackedLen, sz);
		}
	}
	return out;
}

void MapReader::print() {
	printf("There are %u entries\n", numEntries);
	for(uint32_t i = 0; i != numEntries; i++) {
		printf("Entry %u - position (%i, %i, %i), tile %i / %i\n",
			i, entry[i].x, entry[i].y, entry[i].z,
			entry[i].tile, entry[i].subTile
		);
	}
}
