#cmake_minimum_required(VERSION 2.8)
#project(qsort_demo)

add_executable(qsort_test qsort_test.c glibc_qsort.c)
#add_executable(cmetaprog ct_strlen.c)
add_executable(static_strlen static_strlen.c)
add_executable(cptests cptests.c)

#install(TARGETS cmetaprog RUNTIME DESTINATION bin)
