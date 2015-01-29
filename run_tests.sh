#!/bin/bash

# run_tests.sh - a stupid script that bypasses cmake, builds and runs qsort_test
#		 with varying parameters
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

CFLAGS="-std=gnu11 -march=native -g3 -pipe -Wall -Wextra -Wcast-align -Wno-unused-parameter -O2 -DNDEBUG"

die() {
	echo "ERROR: $*" >&2
	exit 1
}

build() {
	rm -f CMakeFiles/qsort_test.dir/{glibc_qsort.c,qsort_test.o} qsort_test

	set -x
	gcc ${CFLAGS} ${CPPFLAGS} -o CMakeFiles/qsort_test.dir/glibc_qsort.o -c ../glibc_qsort.c || die
	gcc ${CFLAGS} ${CPPFLAGS} -o CMakeFiles/qsort_test.dir/qsort_test.o -c ../qsort_test.c || die
	gcc ${CFLAGS} ${CPPFLAGS} CMakeFiles/qsort_test.dir/{glibc_qsort,qsort_test}.o -o qsort_test -rdynamic || die
	set +x
}

run_tests() {
	for total_size in 16384 32768 65536 262144 1048576 4194304; do
		echo "total_size = ${total_size}"
		for size in 1 2 4 8 16 24 32 36 40 48 64 128 256 512 1024 2048 4096 8192; do
			typeset -i num_elems
			echo "size = ${size}"

			num_elems=$((total_size / size))

			# If less than 16 elements, then skip the test
			if ((num_elems < 16)); then
				continue;
			fi
			#num_elems=8

			for align in 1 2 4 8 16 32; do
				typeset -i test_count=512
				echo "align = ${align}"
				# skip alignments that won't work
				((size < align)) && continue;
				((size % align)) && continue;


				((total_size <= 16384)) && test_count=$((test_count * 4))
				((total_size <= 32768)) && test_count=$((test_count * 2))
				((total_size <= 65536)) && test_count=$((test_count * 2))

				# small elements are slower (per total data)
				((size == 1)) && test_count=$((test_count / 4))
				((size == 2)) && test_count=$((test_count / 2))
				((size > 16)) && test_count=$((test_count * 2))
				((size > 64)) && test_count=$((test_count * 2))
				((size > 128)) && test_count=$((test_count * 2))
				((size > 256)) && test_count=$((test_count * 2))
				#((size > 1024)) && test_count=$((test_count / 2))
				((size > 2048)) && test_count=$((test_count / 2))

				CPPFLAGS="-DELEM_SIZE=${size} -DALIGN_SIZE=${align} -DNUM_ELEMS=${num_elems} -DTEST_COUNT=${test_count}"
				echo "CPPFLAGS=${CPPFLAGS}"
				echo "TEST: elem_size=${size}, align=${align}, num_elems=${num_elems}, test_count=${test_count}, total_size=${total_size}"
				CPPFLAGS="${CPPFLAGS=}" build || die

				set -x
				./qsort_test 2>&1 || die
				set +x
			done
		done;
	done
}

mkdir -p build/CMakeFiles/qsort_test.dir
rm build/out.log
pushd build || die
run_tests 2>&1 | tee -a out.log
popd
