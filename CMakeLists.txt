cmake_minimum_required(VERSION 3.15.0)
project(ootoc VERSION 0.1.0)

option(OOTOC_TEST "" ON)
include_directories(src)

add_subdirectory(lib)
add_subdirectory(src)

if(OOTOC_TEST)
    enable_testing()
    add_subdirectory(test)
endif()

# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)
