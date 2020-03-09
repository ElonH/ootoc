
SET(TAR_BIN "${CMAKE_CURRENT_BINARY_DIR}/libtar")
SET(TAR_SRC "${CMAKE_CURRENT_SOURCE_DIR}/libtar")
SET(TAR_INCLUDE "${TAR_BIN}/include" CACHE PATH "libtar include directory")
SET(TAR_LIB "${TAR_BIN}/lib/libtar.a" CACHE FILEPATH "libtar library")

option(OOTOC_BUILDIN_LIBTAR "Using build-in libtar, otherwise find libtar in system" ON)
if(OOTOC_BUILDIN_LIBTAR)
    include(ExternalProject)
    ExternalProject_Add(
        libtar
        PREFIX ${TAR_BIN}
        SOURCE_DIR ${TAR_SRC}
        CONFIGURE_COMMAND cd ${TAR_SRC} && autoreconf --force --install
        COMMAND ${TAR_SRC}/configure --srcdir=${TAR_SRC} --prefix=${TAR_BIN} --enable-static=yes --disable-shared
        BUILD_COMMAND $(MAKE)
        INSTALL_COMMAND $(MAKE) install
        BUILD_BYPRODUCTS ${TAR_LIB}
        BUILD_ALWAYS TRUE
    )
    add_library(Tar STATIC IMPORTED GLOBAL)

    add_dependencies(Tar libtar)
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${TAR_INCLUDE}")
    set_target_properties(Tar PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${TAR_INCLUDE})
    set_target_properties(Tar PROPERTIES IMPORTED_LOCATION ${TAR_LIB})
else()
    if("${TAR_INCLUDE}" STREQUAL "${TAR_BIN}/include")
        UNSET(TAR_INCLUDE CACHE)
    endif()
    find_path(TAR_INCLUDE libtar.h DOC "libtar include directory")
    if(TAR_INCLUDE)
        message("Looking for libtar.h found in ${TAR_INCLUDE}")
    else()
        message(FATAL_ERROR, "Looking for libtar.h not found")
    endif()

    if("${TAR_LIB}" STREQUAL "${TAR_BIN}/lib/libtar.a")
        UNSET(TAR_LIB CACHE)
    endif()
    find_library(TAR_LIB tar DOC "libtar library")
    if(TAR_LIB)
        message("Looking for libtar found in ${TAR_LIB}")
    else()
        message(FATAL_ERROR, "Looking for libtar not found")
    endif()

    add_library(Tar INTERFACE)
    target_include_directories(Tar INTERFACE ${TAR_INCLUDE})
    target_link_libraries(Tar INTERFACE "${TAR_LIB}")
endif()

find_package(CURL REQUIRED)
if(CURL_FOUND)
    add_library(Curl INTERFACE)
    target_include_directories(Curl INTERFACE ${CURL_INCLUDE_DIRS})
    target_link_libraries(Curl INTERFACE "${CURL_LIBRARIES}")
endif()

if(OOTOC_TEST)
    # google test
    set(INSTALL_GTEST OFF CACHE BOOL "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)")
    add_subdirectory(googletest)
endif()

find_package(yaml-cpp 0.6.0 REQUIRED)

# OPTION(OOTOC_BUILDIN_HTTPLIB "Using build-in httplib, otherwise find libtar in system" ON)
# if(OOTOC_BUILDIN_HTTPLIB)
add_subdirectory(httplib)
# else()
#     find_package(httplib REQUIRED)
# endif()

add_subdirectory(CLI11)
