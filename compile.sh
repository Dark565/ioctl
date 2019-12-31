# An handly script for compiling small programs when professional build system would eat some time without any greater sense

# Copyright 2019 Grzegorz Kociołek
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

cd src

SRC=( "main.c" "config.c" "ioctl.c" "convert.c" )
BUILD_DIR="../build"
GCC_FLAGS="-O2 -D__IOCTL_VERSION=$(<../VERSION)"
LINKER_FLAGS=""
TARGET="ioctl"


cd "$(dirname "$0")"

die() {
	echo "$1" >&2
	exit 1
}

help() {
	echo "Usage: $0 [clean][build]"
}

build() {

	mkdir -p "$BUILD_DIR"

	for obj in "${SRC[@]}"; do
		obj_file="${BUILD_DIR}/${obj/%.c/.o}"
		[ -f "${obj_file}" ] && continue

		echo "Compiling $((++b_ind))/${#SRC[@]}: ${obj_file}"
		gcc -c ${GCC_FLAGS} "${obj}" -o "${obj_file}" || die "Cannot compile '${obj} file'"
	done

	cd "$BUILD_DIR"
	echo "Linking target"
	gcc $LINKER_FLAGS "${SRC[@]/%.c/.o}" -o "${TARGET}" || die "Cannot link '${TARGET}' object"
	echo "Success!"

}

if [ -n "$1" ]; then
	while [ -n "$1" ]; do
		case "$1" in
			"help" )
				help
				;;
			"clean" )
				rm -r ${BUILD_DIR}
				;;
			"build")
				build
				;;
			* )
				echo "No appropriate command." >&2
				help >&2
				exit 1
				;;
		esac
		shift
	done
else 
	build
fi
