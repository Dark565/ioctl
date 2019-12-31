/* Program for calling ioctls from command line */

/*
  *Copyright 2019 Grzegorz Kociołek
 *
  *Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
  *to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  *and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
  *The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
  *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
  *WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdint.h>
#include "config.h"

#include <sys/sysmacros.h>

#define STR2(x) #x
#define STR(x) STR2(x)

static char *cmdline;

__attribute__((noreturn)) void usage(int exit_val)
{
	fprintf(stderr,
			"Usage: %s [-hvbcot] FILE REQUEST [INPUT]\n"
			"\n"
			"-h, --help			Show this help\n"
			"-v, --version			Show the program version\n"
			"\n"
			"-b, --block MAJOR:MINOR\n"
			"-c, --char MAJOR:MONOR		Create a temporary device file with specific MAJOR and MINOR instead of using FILE.\n"
			"				You require a CAP_MKNOD capability or being the root to create a block or char device file.\n"
			"				These two parameters are mutually exclusive.\n"
			"\n"
			"-o, --output BASE		Show data which ioctl(2) returned. Works only if input was a structure. BASE may be: raw, hex, oct, bin.\n"
			"\n"
			"-t, --timeval MSECONDS		Time in microseconds for ioctl(2) to return. If it will not, it will be interrupted and program will exit with status 2\n"
			"\n"
			"--define-directory DIR		Set a directory tree in which there are all definitions in C format. Default directory is '/usr/include'\n"
			"\n"
			"--no-cache			Don't cache definition data\n"
			"\n"
			"--rm-cache			Clear cache on the program startup\n"
			"--timeval-find MSECONDS	Time in microseconds for program to find a definition of REQUEST in a directory tree. If it will not, program exits with status 2\n"
			"\n"
			"\n",
		cmdline
			);

	if(exit_val == 0) {
		fputs(
				"Input format:\n"
				"<s> - Struct; requires description,\n"
				"<w> - Word; requires description,\n"
				"\n"
				"<f> - file, load raw number or structure definition from specified file or from standard input if -; for <s> and <w>,\n"
				"<l> - Aligned struct or buffer size; for <s>, <b>,\n"
				"<b[size]> - Buffer (string) of characters; size is optional; for <s>,\n"
				"<n[8/16/32/64]> - Number or character; for <s> and <w>\n"
				"<p<[definition]>> - Definition in this formatter will be placed somewhere else in the memory\n"
				"		and a long pointer to that data's address will be created in a place of invocation of <p> in a structure\n"
				"\n"
				"Notes:\n"
				"Note to formats in structs. You must add bit suffix.  It's because of variable sizes. For example: n32:, n64:, etc.\n"
				"Note to REQUEST format. In a case of names, finding may take some time, so if --no-cache is not selected, cache is created for the future usages.\n"
				"Anyway, if you want to use this tool with maximum effectivity, pass a number as a REQUEST.\n"
				"\n"
				"Difference between, take, '<s><n8>X' and '<w><n8>X' is that in the first case, structure will be created somewhere in memory and pointer to it will be passed as ioctl(2) argument,\n"
				"while in the second case a number zero-extended to long will be passed.\n"
				"\n"
				"If you want this program to ignore special treatment of '<>' characters in strings, format strings with ' quotes.\n"
				"\n"
				"Example usage of getting and printing a struct: 'ioctl -o hex /dev/fb0 FBIOGET_VSCREENINFO <s><l>160'\n"
				"Example usage without any input: 'ioctl /dev/sr0 CDROMEJECT'\n"
				"Example usage with an input as a single byte in a struct: 'ioctl /dev/pts/1 TIOCSTI <s><n8>H'\n"
				"\n"
				"Author is Grzegorz Kociołek, GPLv3 License\n\n",
			stderr
		);
	}

	exit(exit_val);
}

static char cmd_options[] = "-hvn:ot:";

static struct option cmd_loptions[] = {
	{"help", no_argument, 0, 'h'},
	{"version", no_argument, 0, 'v'},
	{"block", required_argument, 0, 'b'},
	{"char", required_argument, 0, 'c'},
	{"output", no_argument, 0, 'o'},
	{"timeval", required_argument, 0, 't'},
	{"define-directory", required_argument, 0, '1'},
	{"no-cache", no_argument, 0, '2'},
	{"rm-cache", no_argument, 0, '3'},
	{"find-timeval", required_argument, 0, '4'},
	{0,0,0,0}
};

static void useconds_from_optarg(uint64_t *useconds) {
	uint64_t useconds;
	if(sscanf(optarg, "%llu", useconds) < 1) {
		fprintf(stderr, "Argument %u: invalid timeval.\n", optin);
		usage(1);
	}
}

int main(int argc, char *argv)
{
	int option;
	memset(&main_config, 0, sizeof(main_config));
	cmdline = argv[0];
	const char **curr_ioctl_str = main_config.ioctl_strings;
	while((option = getopt_long(argc, argv, cmd_options, cmd_loptions, NULL)) != -1) {
		switch(option) {
			case 'h':
				usage(0);
			case 'v':
				fputs("Version: "STR(__IOCTL_VERSION)"\n",stderr);
				exit(0);
			case 'n':
			{
				unsigned major, minor;
				if(sscanf(optarg,"%u:%u",&major,&minor) < 2) {
					usage(1);
				}
				main_config.devid.id = makedev(major,minor);
				main_config.flags |= CONFIG_DEVID;
				break;
			}
			case 'o':
				main_config.flags |= CONFIG_OUTPUT;
				break;
			case 't':
				useconds_from_optarg(&main_config.ioctl_timeval);
				break;
			case '1':
				main_config.ioctl_find_directory = optarg;
				break;
			case '2':
				main_config.flags |= CONFIG_NONCACHE;
				break;
			case '3':
				main_config.flags |= CONFIG_RMCACHE;
				break;
			case '4':
				useconds_from_optarg(&main_config.ioctl_find_timeval);
				break;
			case 1:
			{
				if(curr_ioctl_str < main_config.ioctl_strings+3) {
					*curr_ioctl_str = optarg;
					curr_ioctl_str++;
				}
				else
					usage(1);
				break;
			}
			case '?':
				usage(1);
		}
	}

	if(main_config.ioctl_file_s == NULL || main_config.ioctl_request_s == NULL) {
		usage(1);
	}

	ioctl_start();

}
