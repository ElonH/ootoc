set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
add_library(ootoc ootoc.cpp)
target_link_libraries(ootoc PUBLIC yaml-cpp Tar httplib Curl Threads::Threads)

add_executable(ootocCLI main.cpp)
target_link_libraries(ootocCLI PRIVATE ootoc)
