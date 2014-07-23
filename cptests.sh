#!/bin/bash

# Copyright (C) 2014 Daniel Santos <daniel.santos@pobox.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA

CFLAGS="-std=gnu11 -march=native -g3 -pipe -Wall -Wextra -Wcast-align -Wno-unused-parameter -O2 -DDEBUG"
#CROSS_COMPILE=/usr/bin/armv6j-hardfloat-linux-gnueabi-
#CROSS_COMPILE=/usr/bin/armv7a-hardfloat-linux-gnueabi-
#CFLAGS="-std=gnu11 -march=armv5te -g3 -pipe -Wall -Wextra -Wcast-align -Wno-unused-parameter -O2 -DNDEBUG"


die() {
	echo "ERROR: $*" >&2
	exit 1
}

build_tests() {
	#for size in 1 2 4 8 16 24 32 36 40 48 64 128 256 512 1024 2048 4096 8192; do
	#for ((size = 1; size <= 65536; size <<= 1)); do
	#for size in 1 2 4 8 16 24 32 36 40 48 64; do
	for size in 2048; do
		echo "size = ${size}"
		local out_file="build/copytest-${size}.o"
		${CROSS_COMPILE}gcc ${CFLAGS} "-DELEM_SIZE=${size}" -c copytest.c -o ${out_file} || die

		#for align in 1 2 4 8 16; do
		#	local out_file="build/copytest-${size}x${align}.o"
		#	CPPFLAGS="-DELEM_SIZE=${size} -DALIGN_SIZE=${align}"
		#	echo "align = ${align}"
		#	#echo "CPPFLAGS=${CPPFLAGS}"
		#	set -x
		#	${CROSS_COMPILE}gcc ${CFLAGS} ${CPPFLAGS} -c copytest.c -o build/copytest-${size}x${align}.o || die
		#	set +x
		#done
	done;
}

rm build/copytest*.o
build_tests

