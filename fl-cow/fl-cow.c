/*
 *  FL-COW by Davide Libenzi (File Links Copy On Write)
 *  Copyright (C) 2003..2009  Davide Libenzi
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Davide Libenzi <davidel@xmailserver.org>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _ATFILE_SOURCE

#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <regex.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <dlfcn.h>


#ifdef ENABLE_DEBUG
#define DPRINTF(format, args...) fprintf(stderr, format, ## args)
#else
#define DPRINTF(format, args...) do { } while (0)
#endif

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1L)
#endif

#define REAL_LIBC RTLD_NEXT
#define FLCOW_MAXPATH 1024

#if defined(__APPLE__) || defined(__MACOSX__)
#define AT_FDCWD -100
#define openat(dfd, name, flags, args...) open(name, flags, ## args)
#define FLCOW_MAP_FUNC(sym) sym
#define FLCOW_MAP_ALIAS(sym, type) /* Aliases are unnecessary under OSX */
#else
#define FLCOW_MAP_FUNC(sym) flcow_##sym
#define FLCOW_MAP_ALIAS(sym, type) FLCOW_ ##type##_ALIAS(sym, sym)
#endif

#define FLCOW_OPEN_ALIAS(asym, rsym) int asym(char const *, int, ...)	\
	__attribute__ ((alias("flcow_" #rsym)))
#define FLCOW_MAP_OPEN(sym)						\
	int FLCOW_MAP_FUNC(sym)(char const *name, int flags, ...) {	\
		static int (*func_open)(char const *, int, ...) = NULL; \
		va_list args;						\
		mode_t mode;						\
		if (!func_open &&					\
		    !(func_open = (int (*)(char const *, int, ...))	\
		      dlsym(REAL_LIBC, #sym))) {			\
			fprintf(stderr, "missing symbol: %s\n", #sym);	\
			exit(1);					\
		}							\
		va_start(args, flags);					\
		mode = (mode_t) va_arg(args, int);			\
		va_end(args);						\
		DPRINTF("[%s](%s, 0x%x, 0x%x)\n", __FUNCTION__, name,	\
			flags, mode);					\
		return do_generic_open(name, flags, mode, func_open);	\
	}								\
	FLCOW_MAP_ALIAS(sym, OPEN)


#if defined(__APPLE__) || defined(__MACOSX__)
#define FLCOW_MAP_OPENAT(sym) /* There is no openat in OSX */
#else
#define FLCOW_OPENAT_ALIAS(asym, rsym) int asym(int, char const *, int, ...) \
	__attribute__ ((alias("flcow_" #rsym)))
#define FLCOW_MAP_OPENAT(sym)						\
	int FLCOW_MAP_FUNC(sym)(int dirfd, char const *name, int flags, ...) {	\
		static int (*func_open)(int, char const *, int, ...) = NULL; \
		va_list args;						\
		mode_t mode;						\
		if (!func_open &&					\
		    !(func_open = (int (*)(int, char const *, int, ...)) \
		      dlsym(REAL_LIBC, #sym))) {			\
			fprintf(stderr, "missing symbol: %s\n", #sym);	\
			exit(1);					\
		}							\
		va_start(args, flags);					\
		mode = (mode_t) va_arg(args, int);			\
		va_end(args);						\
		DPRINTF("[%s](%s, 0x%x, 0x%x)\n", __FUNCTION__, name,	\
			flags, mode);					\
		return do_generic_openat(dirfd, name, flags, mode, func_open); \
	}								\
	FLCOW_MAP_ALIAS(sym, OPENAT)
#endif

#define FLCOW_FOPEN_ALIAS(asym, rsym) FILE *asym(char const *, char const *) \
	__attribute__ ((alias("flcow_" #rsym)))
#define FLCOW_MAP_FOPEN(sym)						\
	FILE * FLCOW_MAP_FUNC(sym)(char const *name, char const *mode) { \
		static FILE *(*func_open)(char const *, char const *) = NULL; \
		if (!func_open &&					\
		    !(func_open = (FILE *(*)(char const *, char const *)) \
		      dlsym(REAL_LIBC, #sym))) {			\
			fprintf(stderr, "missing symbol: %s\n", #sym);	\
			exit(1);					\
		}							\
		DPRINTF("[%s](%s, '%s')\n", __FUNCTION__, name, mode);	\
		return do_generic_fopen(name, mode, func_open);		\
	}								\
	FLCOW_MAP_ALIAS(sym, FOPEN)

#define FLCOW_FREOPEN_ALIAS(asym, rsym) FILE *asym(char const *, char const *, FILE *) \
	__attribute__ ((alias("flcow_" #rsym)))
#define FLCOW_MAP_FREOPEN(sym)						\
	FILE * FLCOW_MAP_FUNC(sym)(char const *name, char const *mode, FILE *file) { \
		static FILE *(*func_open)(char const *, char const *, FILE *) = NULL; \
		if (!func_open &&					\
		    !(func_open = (FILE *(*)(char const *, char const *, FILE *)) \
		      dlsym(REAL_LIBC, #sym))) {			\
			fprintf(stderr, "missing symbol: %s\n", #sym);	\
			exit(1);					\
		}							\
		DPRINTF("[%s](%s, '%s')\n", __FUNCTION__, name, mode);	\
		return do_generic_freopen(name, mode, file, func_open);	\
		}							\
	FLCOW_MAP_ALIAS(sym, FREOPEN)

#define MIN(a,b) ((b) < (a) ? (b) : (a))



static char path_pattern[FLCOW_MAXPATH] = "";
static char exclude_pattern[FLCOW_MAXPATH] = "";
static regex_t path_regex;
static regex_t exclude_regex;

void update_regex(const char* pattern, char* previous, regex_t* regex)
{
    if(pattern == 0)
    {
        strncpy(previous, "", FLCOW_MAXPATH);
        regcomp(regex, "", REG_NOSUB);
    }
    else if(strcmp(pattern, previous) != 0)
    {
        strncpy(previous, pattern, FLCOW_MAXPATH);
        regcomp(regex, pattern, REG_NOSUB);
    }
}

void update_regexes()
{
    const char* path = getenv("FLCOW_PATH");
    update_regex(path, path_pattern, &path_regex);
    const char* exclude = getenv("FLCOW_EXCLUDE");
    update_regex(exclude, exclude_pattern, &exclude_regex);
}

int regex_match(const char* pattern, const regex_t* regex, const char* text)
{
    DPRINTF("match \"%s\" with pattern \"%s\" : %d\n", text, pattern, regexec(regex, text, 1, 0, 0) == 0);
    return ((strlen(pattern) > 0) && regexec(regex, text, 1, 0, 0) == 0);
}

static int cow_name(char const *name) {
	char normalized_path[FLCOW_MAXPATH];
	realpath(name, normalized_path);
	update_regexes();
	if(regex_match(path_pattern, &path_regex, normalized_path) && !regex_match(exclude_pattern, &exclude_regex, normalized_path))
	{
	    return 1;
	}
	return 0;
}

static int do_cow_name(int dirfd, char const *name) {
	char temp_path[FLCOW_MAXPATH];
	int input_fd, temp_fd;
	struct stat64 input_stat, temp_stat;

	if ((input_fd = openat(dirfd, name, O_RDONLY, 0)) == -1)
		return -1;
	if (fstat64(input_fd, &input_stat)) {
		close(input_fd);
		return -1;
	}
	snprintf(temp_path, FLCOW_MAXPATH, "%s,,+++", name);
	if ((temp_fd = open(temp_path, O_CREAT | O_EXCL | O_WRONLY, input_stat.st_mode)) == -1) {
		close(input_fd);
		return -1;
	}
	off64_t file_size = input_stat.st_size;
	off64_t offset = 0;
	while(offset != file_size)
	{
		void* input_buffer;
		size_t buffer_size = (size_t)MIN((off64_t)(file_size - offset), (off64_t)(SIZE_MAX/8));
		if((input_buffer = mmap64(NULL, buffer_size, PROT_READ, MAP_PRIVATE, input_fd, offset)) == MAP_FAILED)
		{
			close(temp_fd);
			unlink(temp_path);
			close(input_fd);
			return -1;
		}
		size_t write_size = write(temp_fd, input_buffer, buffer_size);
		munmap(input_buffer, buffer_size);
		offset = offset + write_size;
	}
	close(input_fd);
	
	if (fstat64(temp_fd, &temp_stat) == -1 || temp_stat.st_size != input_stat.st_size) {
		close(temp_fd);
		unlink(temp_path);
		return -1;
	}
	fchown(temp_fd, input_stat.st_uid, input_stat.st_gid);
	close(temp_fd);

	if (unlink(name) == -1) {
		unlink(temp_path);
		return -1;
	}
	rename(temp_path, name);

	return 0;
}

static int is_cow_open(char const *name, int flags) {
	struct stat stb;

	return (flags & O_RDWR || flags & O_WRONLY) && cow_name(name) &&
		!stat(name, &stb) && S_ISREG(stb.st_mode) && stb.st_nlink > 1;
}

static int do_cow_open(int dirfd, char const *name, int flags) {
	int cowres = 0;

	if (is_cow_open(name, flags)) {
		DPRINTF("COW-ing %s\n", name);
		cowres = (do_cow_name(dirfd, name) < 0) ? -1: 1;
		if (cowres < 0)
			DPRINTF("COW-ing %s failed !\n", name);
		else
			DPRINTF("COW-ing %s succeed\n", name);
	}

	return cowres;
}

static int do_generic_open(char const *name, int flags, mode_t mode,
			   int (*open_proc)(char const *, int, ...)) {
	do_cow_open(AT_FDCWD, name, flags);
	DPRINTF("calling libc open[%p](%s, 0x%x, 0x%x)\n", open_proc, name, flags, mode);

	return open_proc(name, flags, mode);
}

static int do_generic_openat(int dirfd, char const *name, int flags, mode_t mode,
			     int (*open_proc)(int, char const *, int, ...)) {
	do_cow_open(dirfd, name, flags);
	DPRINTF("calling libc openat[%p](%d, %s, 0x%x, 0x%x)\n", dirfd,
		open_proc, name, flags, mode);

	return open_proc(dirfd, name, flags, mode);
}

static int is_cow_fopen(char const *name, char const *mode) {
	struct stat stb;

	return (strchr("wa", *mode) != NULL || strchr(mode, '+') != NULL) &&
		cow_name(name) && !stat(name, &stb) &&
		S_ISREG(stb.st_mode) && stb.st_nlink > 1;
}

static int do_cow_fopen(char const *name, char const *mode) {
	int cowres = 0;

	if (is_cow_fopen(name, mode)) {
		DPRINTF("COW-ing %s\n", name);
		cowres = (do_cow_name(AT_FDCWD, name) < 0) ? -1: 1;
		if (cowres < 0)
			DPRINTF("COW-ing %s failed !\n", name);
		else
			DPRINTF("COW-ing %s succeed\n", name);
	}

	return cowres;
}

static FILE *do_generic_fopen(char const *name, char const *mode,
			      FILE *(*open_proc)(char const *, char const *)) {
	do_cow_fopen(name, mode);
	DPRINTF("calling libc fopen[%p](%s, '%s')\n", open_proc, name, mode);

	return open_proc(name, mode);
}

static FILE *do_generic_freopen(char const *name, char const *mode, FILE *file,
				FILE *(*open_proc)(char const *, char const *, FILE *)) {
	do_cow_fopen(name, mode);
	DPRINTF("calling libc freopen[%p](%s, '%s', %p)\n", open_proc, name, mode, file);

	return open_proc(name, mode, file);
}

FLCOW_MAP_OPEN(open);
FLCOW_MAP_OPEN(open64);
FLCOW_MAP_OPENAT(openat);
FLCOW_MAP_OPENAT(openat64);
FLCOW_MAP_FOPEN(fopen);
FLCOW_MAP_FOPEN(fopen64);
FLCOW_MAP_FREOPEN(freopen);
FLCOW_MAP_FREOPEN(freopen64);

