cmake_minimum_required(VERSION 3.15.0)
project(ootoc VERSION 0.1.0)

option(OOTOC_TEST "" ON)
include_directories(src)

include_directories(lib)
add_subdirectory(lib)
add_subdirectory(src)

if(OOTOC_TEST)
    enable_testing()
    add_subdirectory(test)
endif()

install(TARGETS ootoc Curl Tar EXPORT ootocConfig)
install(TARGETS ootocCLI)
install(EXPORT ootocConfig DESTINATION share/ootoc/cmake)
export(TARGETS ootoc Curl Tar FILE ootocConfig.cmake)
