
SET(TAR_BIN "${CMAKE_CURRENT_BINARY_DIR}/libtar")
SET(TAR_SRC "${CMAKE_CURRENT_SOURCE_DIR}/libtar")
SET(TAR_INCLUDE "${TAR_BIN}/include")
SET(TAR_LIB "${TAR_BIN}/lib/libtar.a")
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

set_target_properties(Tar PROPERTIES IMPORTED_LOCATION ${TAR_LIB})
set_target_properties(Tar PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${TAR_INCLUDE})

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
