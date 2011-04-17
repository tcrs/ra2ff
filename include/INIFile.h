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

#ifndef INIFILE_H__
#define INIFILE_H__

#include <map>
#include <string>
#include <stdio.h>
#include "Exception.h"

class INIFile {
protected:
	typedef std::map<std::string, std::string> Section;
	typedef std::map<std::string, Section> SectionMap;
	SectionMap sections;
	SectionMap::iterator currentSection;
	bool parseIniLine(char const* line);
	bool trim(std::string& str, std::string const& trimchars);
public:
	typedef SectionMap::const_iterator section_iterator;
	typedef Section::const_iterator key_iterator;

	INIFile();
	INIFile(std::string const& fn);
	bool sectionExists(std::string const& section) const;
	void setCurrentSection(std::string const& section);
	std::string getCurrentSectionName() const;
	bool keyExists(std::string const& key) const;
	std::string getKey(std::string const& key) const;
	void setKey(std::string const& key, std::string const& val);
	void eraseSection();
	void eraseKey(std::string const& key);
	void write(std::string const& fn) const;
	void write(FILE* fp) const;
	void read(std::string const& fn);
	void read(FILE* fp);

	section_iterator sectionsBegin();
	section_iterator sectionsEnd();

	key_iterator keysBegin();
	key_iterator keysEnd();
};

#endif
