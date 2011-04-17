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

#ifndef EXCEPTION_H__
#define EXCEPTION_H__

#include <stdarg.h>
#include <stdio.h>
#include <exception>

#define EXCEPTION(...)		Exception(__FILE__, __LINE__, __VA_ARGS__)
#define EWARN(fmt, ...)		fprintf(stderr, "WARN[%s:%i] " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)
#define WARN(fmt)		fprintf(stderr, "WARN[%s:%i] " fmt "\n", __FILE__, __LINE__)
#ifndef NDEBUG
#	define EDEBUG(fmt, ...)		fprintf(stdout, "DEBUG[%s:%i] " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)
#	define DEBUG(fmt)		fprintf(stdout, "DEBUG[%s:%i] " fmt "\n", __FILE__, __LINE__)
#	define GLCHECKERROR		do { \
						unsigned int err; \
						if((err = glGetError()) != GL_NO_ERROR) { \
							throw EXCEPTION("OpenGL error %u (%s)", err, gluErrorString(err)); \
						} \
					} while(0)
#else
#	define DEBUG(fmt)		do {} while(0)
#	define EDEBUG(fmt, ...)		do {} while(0)
#	define GLCHECKERROR		do {} while(0)
#endif
#	define ERROR(fmt, ...)		fprintf(stdout, "** ERROR[%s:%i] " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)

#define MAX_EXCEPTION_LEN 1024

class Exception : public std::exception {
protected:
	char buf[MAX_EXCEPTION_LEN];
public:
	virtual ~Exception() throw() { }

	Exception(char const* file, unsigned int line, char const* fmt, ...) {
		va_list vp;
		va_start(vp, fmt);
		int sz = snprintf(&buf[0], MAX_EXCEPTION_LEN, "%s:%i:\n\t", file, line);
		vsnprintf(&buf[sz], MAX_EXCEPTION_LEN - sz, fmt, vp);
		va_end(vp);
	}

	char const* what() const throw() {
		return &buf[0];
	}
};

#endif
