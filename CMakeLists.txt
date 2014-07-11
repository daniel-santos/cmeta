#cmake_minimum_required(VERSION 2.8)
#project(sqort_demo)

add_executable(sqort_demo qsort.c /usr/src/debug/sys-libs/glibc-2.17/glibc-2.17/stdlib/qsort.c)
#add_executable(cmetaprog ct_strlen.c)
add_executable(static_strlen static_strlen.c)
add_executable(cptests cptests.c)

#install(TARGETS cmetaprog RUNTIME DESTINATION bin)
