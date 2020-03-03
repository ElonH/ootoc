cmake_minimum_required(VERSION 3.15.0)
project(ootoc VERSION 0.1.0)

# include(CTest)
# enable_testing()
add_subdirectory(lib)

add_executable(ootoc main.cpp)
target_link_libraries(ootoc PRIVATE Tar)
# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)
