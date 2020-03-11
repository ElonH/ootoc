
include(GoogleTest)

file(GLOB files "src/test-*.cxx")
foreach(file ${files})
    get_filename_component(fileName "${file}" NAME_WLE)
    add_executable("${fileName}" "${file}")

    target_link_libraries("${fileName}" PRIVATE gtest_main ootoc)
    gtest_discover_tests(
        "${fileName}"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
endforeach()

add_executable(main-test src/main-test.cxx)
target_link_libraries(main-test PRIVATE ootoc)
add_test(
    NAME mainTest 
    COMMAND main-test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)  
