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

#include "ioctl.h"
#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

enum {
	_INTERNAL_DEFINITION = 0x1,
}

#define _IOCTL_NOHOME -1
#define _IOCTL_CANT_MKDIR -2

#define _IOCTL_CANT_OPEN -1
#define _IOCTL_BAD_FILETYPE -2

/** Make directory for each name in a tree which doesn't exist */
/*  Behavior same as 'mkdir -p' command */
static int mkdir_tree(const char *path)
{

	int dir_fd = -1, old_dir_fd = AT_FDCWD;
	const int dir_sz = strlen(path);
	char* dir_name = malloc(dir_sz+1);
	memcpy(dir_name,path,dir_sz+1);

	char* path_rew = dir_name;

	char* slash_pos;
	char end = 0, retval = 0;

	if(path_rew[0] == '/') {
		slash_pos = strchr(path_rew+1,'/');		
		goto check_pos;
	}


	while(path_rew < dir_name + dir_sz) {
		slash_pos = strchr(path_rew,'/');

		if(slash_pos != path_rew) {

check_pos:
			if(slash_pos != NULL) {
				*slash_pos = '\0';
			}
			else {
				end = 1;
			}
		}
		else {
			goto rewind;
		}

		mkdirat(old_dir_fd, path_rew, 0777);
		dir_fd = openat(old_dir_fd, path_rew, O_RDONLY|O_DIRECTORY);
		if(dir_fd == -1) {
			retval = -1;
			break;
		}

		if(old_dir_fd > -1) close(old_dir_fd);
		old_dir_fd = dir_fd;

		if(end) break;

rewind:
		path_rew = slash_pos + 1;
	}

	if(dir_fd != -1) close(dir_fd);
	free(dir_name);
	return retval;

}

static struct _ioctl_cache_info {
	char where[PATH_MAX];
	int cachedir_fd;
	int file_fd;
} _cache_info;

int ioctl_add_to_cache(long request, const char *definition)
{
	int err;
	FILE *cachep = fopen(_cache_info.where, "a");
	if(cachep == NULL) {
		return -1;
	}
	err = fprintf(cachep, "%s=%li\n", definition, request);
	fclose(cachep);

	return err != -1;
}


long ioctl_lookup_cache(const char* definition)
{
	long request = -1;
	int err;
	char def[512];

	FILE *cachep = fopen(_cache_info.where, "a");
	if(cachep == NULL) {
		return -1;
	}
	while(1) {
		err = fscanf(cachep, "%[^=]=%li\n", def, &request);
		if(err < 2) continue;
		else if(err == EOF) break;
		if(strcmp(def, definition) == 0) {
			return request;
		}
	}
	return -1;
}


/* Super complicated method for finding #define X Y
 * Yeah. That's how chads would do it */
long ioctl_lookup_source(FILE *file, const char *definition)
{
	char fixed_format[512];
	//char def[512];
	long request;
	int err;
	
	err = snprintf(fixed_format, 512, "%s%s%s", "#define%*[ ]", definition, "%*[ ]%li");
	if(err < 512) return -1; 

	while(1) {
		err = fscanf(file, fixed_format, &request);
		if(err < 1) continue;
		else if(err == EOF) break;
		return request;
	}

	return -1;
}

long ioctl_find(DIR *treep, const char *definition, uint64_t timeval, size_t deepness)
{
	struct stat statbuf;
	struct dirent *direntp = NULL;
	int filefd = -1;
	long retval = -1;
	union {
		FILE *filep = NULL;
		DIR *dirp;
	};
	if(deepness = 0 || treep == NULL) return -1;

	while(direntp = readdir(treep)) {
		filefd = open(direntp->d_name, O_RDONLY);
		if(filefd == -1) {
			continue;
		}
		fstat(filefd, &statbuf);
		if(S_ISREG(statbuf.st_mode)) {
			filep = fdopen(filefd);
			retval = ioctl_lookup_source(filep, definition);
			fclose(filep);
			
			goto retval_check;
		}
		else
		if(S_ISDIR(statbuf.st_mode)) {
			deepness--;
			if(deepness > 0) {
				dirp = fdopendir(filefd);
				retval = ioctl_find(dirp, definition, 0, deepness);
				closedir(dirp);

				goto retval_check;
			}
		}
		close(filefd);
		continue;
retval_check:
		if(retval != -1)
			return retval;

	}


}

static int ioctl_prepare_cache(void)
{
	struct stat statbuf;
	int fd;
	int err = 0;

	_cache_info.cache_dir = -1;
	const char* home_env = getenv("HOME");
	if(home_env == NULL) return _IOCTL_NOHOME;
	snprintf(_cache_info.where,PATH_MAX,"%s/%s",home_env,CACHE_DIR);
	if(mkdir_recursive(_cache_info.where) == -1) 
		return _IOCTL_CANT_MKDIR;

	strcat(_cache_info.where,CACHE_FILE);
	fd = open(_cache_info.where, O_RDWR | O_CREAT, 0755);
	if(fd == -1) return _IOCTL_CANT_OPEN;
	fstat(fd, &statbuf);
	if(!S_ISREG(statbuf.st_mode)) err=_IOCTL_BAD_FILETYPE;
	close(fd);

	return err;
}


static void ioctl_find_expired_handler(int signo)
{
	fprintf(stderr, "Error!		Coudn't find REQUEST definition in %llu time\n", main_config.ioctl_find_timeval);
	exit(2);
}

void ioctl_start(void)
{
	char flags = 0;
	char cache_wr_flags = 0;

	unsigned long io_request;
	int prepare_error;

	prepare_error = ioctl_prepare_cache();
	switch(prepare_error) {
	case _IOCTL_CANT_OPEN:
		fprintf(stderr, "Warning!	Can't open a cache file for reading and writing in the same time.\n"
				"		There may be errors with writing or reading from/to the cache.\n"
				"		It's possible that, cache file: '%s' has bad permissions or doesn't exist.\n",
				_cache_info.where);
	case _IOCTL_BAD_TYPE:
		fprintf(stderr, "Warning!	Cache file is bad file type.\n"
				"		No reading or writing from it is possible.\n"
		       );
	}

	/* Hardly TODO more in 2019 2 aka 2020 !!! */
	if(sscanf(main_config.ioctl_request_s, "%lu", &io_request) < 1) {
		io_request = main_config.ioctl_flags & (IOCTL_RMCACHE|IOCTL_NONCACHE) ? -1 : ioctl_lookup_cache(main_config.ioctl_request_s);
		if(io_request == -1) {
			const char *find_tree = main_config.ioctl_find_directory ? main_config.ioctl_find_directory : CACHE_DEFAULT_TREE;
			io_request = ioctl_find(find_tree, main_config.ioctl_request_s, main_config.ioctl_find_timeval);
			if(io_request == -1) {
				fprintf(stderr, "Error! Cannot find REQUEST definition in cache nor %s tree\n", find_tree);
				exit(1);
			}
			if(prepare_error >= 0)
				ioctl_add_to_cache(main_config.ioctl_request_s, io_request);
		}
	}







}


