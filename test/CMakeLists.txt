
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
