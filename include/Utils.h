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

#ifndef UTILS_H__
#define UTILS_H__

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <sstream>
#include <vector>
#include "Exception.h"

namespace Utils {
	struct ScopedFile {
		FILE* fptr;
		ScopedFile(FILE* f) : fptr(f) { }
		~ScopedFile() {
			if(fptr != NULL) {
				fclose(fptr);
			}
		}
	};
	class FixedRead {
	protected:
		FILE* f;

		void fread_x(void* buf, size_t sz, size_t n) {
			if(fread(buf, sz, n, f) != n) {
				throw EXCEPTION("Could not read %u items of size %u from stream (%s)%s", n, sz, strerror(errno), feof(f) ? " (End of file)" : "");
			}
		}
	public:
		FixedRead(FILE* fp) : f(fp) { }
		~FixedRead() { if(f) fclose(f); }

		template<typename T>
		void read(T* ptr, size_t n = 1) {
			fread_x(static_cast<void*>(ptr), sizeof(T), n);
		}

		template<typename T>
		void skip(size_t n = 1) {
			if(fseek(f, sizeof(T) * n, SEEK_CUR) == -1) {
				throw EXCEPTION("Could not seek %u bytes in stream (%s)", sizeof(T) * n, strerror(errno));
			}
		}

		long pos() {
			return ftell(f);
		}

		void seek(long pos) {
			if(fseek(f, pos, SEEK_SET) == -1) {
				throw EXCEPTION("Could not seek to position %i in stream (%s)", pos, strerror(errno));
			}
		}
	};
	template<typename T>
	struct ScopedArray {
		T* ptr;
		ScopedArray(T* a) : ptr(a) { }
		~ScopedArray() {
			delete[] ptr;
		}
	};
	template<typename T>
	void split(std::string const& in, std::vector<T>& out, char delim) {
		std::istringstream str(in);
		T tmp;
		while(str) {
			str >> tmp;
			out.push_back(tmp);
			str.ignore(in.length(), delim);
		}
	}
	template<typename T>
	std::vector<T> split(std::string const& in, char delim) {
		std::vector<T> tmp;
		split(in, tmp, delim);
		return tmp;
	}
	template<typename T>
	std::string toString(T v) {
		std::ostringstream tmp;
		tmp << v;
		return tmp.str();
	}
}

#endif
