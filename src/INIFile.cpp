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

#include "INIFile.h"

#include <string.h>

INIFile::INIFile() {
	currentSection = sections.end();
}

INIFile::INIFile(std::string const& fn) {
	currentSection = sections.end();
	read(fn);
}

bool INIFile::sectionExists(std::string const& section) const {
	return sections.find(section) != sections.end();
}

void INIFile::setCurrentSection(std::string const& section) {
	std::pair<SectionMap::iterator, bool> tmp = sections.insert(std::make_pair(section, Section()));
	currentSection = tmp.first;
}

std::string INIFile::getCurrentSectionName() const {
	if(currentSection == sections.end()) {
		throw EXCEPTION("No current section");
	}
	return currentSection->first;
}

bool INIFile::keyExists(std::string const& key) const {
	if(currentSection == sections.end()) {
		throw EXCEPTION("No current section");
	}
	return currentSection->second.find(key) != currentSection->second.end();
}

std::string INIFile::getKey(std::string const& key) const {
	if(currentSection == sections.end()) {
		throw EXCEPTION("No current section");
	}
	Section::iterator it = currentSection->second.find(key);
	if(it == currentSection->second.end()) {
		throw EXCEPTION("Key not found");
	}
	return it->second;
}

void INIFile::setKey(std::string const& key, std::string const& val) {
	if(currentSection == sections.end()) {
		throw EXCEPTION("No current section");
	}
	currentSection->second[key] = val;
}

void INIFile::eraseSection() {
	if(currentSection == sections.end()) {
		throw EXCEPTION("No current section");
	}
	sections.erase(currentSection);
	currentSection = sections.end();
}

void INIFile::eraseKey(std::string const& key) {
	if(currentSection == sections.end()) {
		throw EXCEPTION("No current section");
	}
	Section::iterator it = currentSection->second.find(key);
	if(it != currentSection->second.end()) {
		currentSection->second.erase(it);
	}
}

void INIFile::write(std::string const& fn) const {
	FILE* f = fopen(fn.c_str(), "w");
	if(f == NULL) {
		throw EXCEPTION("Could not open file");
	}
	write(f);
	fclose(f);
}

void INIFile::write(FILE* f) const {
	for(SectionMap::const_iterator it = sections.begin(); it != sections.end(); it++) {
		fprintf(f, "[%s]\n", it->first.c_str());
		for(Section::const_iterator jt = it->second.begin(); jt != it->second.end(); jt++) {
			fprintf(f, "%s = %s\n", jt->first.c_str(), jt->second.c_str());
		}
	}
}

bool INIFile::parseIniLine(char const* line) {
	std::string key, val;
	char* pos;
	if(line[0] == '\0') { // Empty line
	} else if(line[0] == '[') { // Section
		pos = strchr(const_cast<char*>(line + 1), ']');
		if(pos == NULL) {
			return true;
		}
		key.assign(line + 1, pos - line - 1);
		//EDEBUG("Found section %s", key.c_str());
		std::pair<SectionMap::iterator, bool> tmp = sections.insert(std::make_pair(key, Section()));
		currentSection = tmp.first;
	} else if(line[0] == ';' || line[0] == '#') { // Comment
	} else {
		if(currentSection == sections.end()) {
			throw EXCEPTION("No current section, input == \"%s\"", line);
		}
		pos = strchr(const_cast<char*>(line), '=');
		if(pos == NULL) {
			return true;
		}
		key.assign(line, pos - line);
		if(trim(key, " \n")) {
			throw EXCEPTION("Empty key");
		}
		val.assign(pos + 1);
		if(trim(val, " \t")) {
			currentSection->second.insert(make_pair(key, ""));
		} else {
			currentSection->second.insert(make_pair(key, val));
		}
	}
	return false;
}

void INIFile::read(std::string const& fn) {
	FILE* f = fopen(fn.c_str(), "r");
	if(f == NULL) {
		throw EXCEPTION("Could not open file");
	}
	read(f);
	fclose(f);
}

void INIFile::read(FILE* f) {
	char buf[1024];
	char line[1024];
	char *bufp;
	char* linep = &line[0];
	char* linee = &line[sizeof(line) - 1];
	bool stripws = true;
	ssize_t cur, sz;
	while((sz = fread(buf, 1, sizeof(buf), f))) {
		bufp = buf;
		for(cur = 0; cur < sz && linep <= linee; cur++, bufp++) {
			if(stripws && (*bufp == ' ' || *bufp == '\t')) {
				continue;
			}
			if(*bufp == '\n' || *bufp == '\r') {
				*linep = '\0';
				parseIniLine(line);
				linep = line;
				stripws = true;
			} else {
				stripws = false;
				*linep = *bufp;
				linep++;
			}
		}
		if(linep > linee) {
			throw EXCEPTION("Line too long");
		}
	}
}

bool INIFile::trim(std::string& str, std::string const& trimchars) {
	size_t start = str.find_first_not_of(trimchars);
	if(start != std::string::npos) {
		str = str.substr(start, str.find_last_not_of(trimchars) - start + 1);
		return false;
	} else {
		return true;
	}
}

INIFile::section_iterator INIFile::sectionsBegin() {
	return sections.begin();
}

INIFile::section_iterator INIFile::sectionsEnd() {
	return sections.end();
}

INIFile::key_iterator INIFile::keysBegin() {
	if(currentSection == sections.end()) {
		throw EXCEPTION("No current section");
	}
	return currentSection->second.begin();
}

INIFile::key_iterator INIFile::keysEnd() {
	if(currentSection == sections.end()) {
		throw EXCEPTION("No current section");
	}
	return currentSection->second.end();
}
