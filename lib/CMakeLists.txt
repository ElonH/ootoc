find_path(TAR_INCLUDE_DIR libtar.h
    DOC "tar include directory")
if(NOT TAR_INCLUDE_DIR)
    message(FATAL_ERROR "Looking for libtar.h - not found")
else()
    message (STATUS "Looking for libtar.h - found")
endif()

find_library(TAR_LIBRARY tar
    DOC "tar library location")
if(NOT TAR_LIBRARY)
    message(FATAL_ERROR "libtar library - not found")
endif()

add_library(Tar INTERFACE)
target_include_directories(Tar INTERFACE ${TAR_INCLUDE_DIR})
target_link_libraries(Tar INTERFACE ${TAR_LIBRARY})

find_package(CURL REQUIRED)
if(CURL_FOUND)
    add_library(Curl INTERFACE)
    target_include_directories(Curl INTERFACE ${CURL_INCLUDE_DIRS})
    target_link_libraries(Curl INTERFACE "${CURL_LIBRARIES}")
endif()
