add_library(ootoc ootoc.cpp)
target_link_libraries(ootoc PUBLIC Tar Curl)

add_executable(ootocCLI main.cpp)
target_link_libraries(ootocCLI PRIVATE ootoc)
