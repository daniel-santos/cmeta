#cmake_minimum_required(VERSION 2.8)
#project(qsort_demo)

#set(CMAKE_C_FLAGS=-std=gnu11 -march=native -g3 -pipe -Wall -Wextra -Wcast-align -Wno-unused-parameter)
#CMAKE_C_COMPILER:FILEPATH=/usr/bin/gcc
#add_compile_options(CMAKE_C_FLAGS:STRING="-std=gnu11 -march=native -g3 -pipe -Wall -Wextra -Wcast-align -Wno-unused-parameter")
#add_compile_options(CMAKE_C_FLAGS_RELEASE:STRING=-O2 -DNDEBUG)
#add_compile_options(CMAKE_C_FLAGS_RELWITHDEBINFO:STRING=-O2)
#add_compile_options(CMAKE_C_FLAGS_DEBUG:STRING=)
#add_compile_options(-std=gnu11)

add_executable(qsort_test qsort_test.c glibc_qsort.c)
#add_executable(cmetaprog ct_strlen.c)
add_executable(static_strlen static_strlen.c)
add_executable(cptests cptests.c)

#install(TARGETS cmetaprog RUNTIME DESTINATION bin)
