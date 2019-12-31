/*
 * Copyright 2019 Grzegorz Kociołek
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#define CACHE_DIR ".cache/ioctl" /* Begins in HOME */
#define CACHE_FILE "cache"
#define CACHE_DEFAULT_TREE "/usr/include"

enum _IOCTL_CONFIG_FLAGS {
	CONFIG_DEVID = 0x1,  	/* Create a node file from major and minor and use ioctl() on it, then remove it on exit. 
			  	   Requires appropriate permissions (as root has) for making up device files. */

	CONFIG_OUTPUT = 0x2, 	/* If input/output argument is specified and was a struct,
				   print bytes of that struct in specific base on exit. */

	CONFIG_NONCACHE = 0x4,	/* Don't cache any REQUEST definitions. */
	CONFIG_RMCACHE = 0x8	/* Clear the cache on the start */
};

typedef struct {

	char flags;
	const char* ioctl_find_directory;
	const char* ioctl_file_s;
	const char* ioctl_request_s;

	union {
		const char* ioctl_input_output_s;

		struct {
			const char* base;
		} output;
	}

	uint64_t ioctl_timeval; /* in microseconds */
	uint64_t ioctl_find_timeval;

	struct {
		dev_t id;
	} devid;

} ioctl_config;

extern ioctl_config main_config;

#endif
