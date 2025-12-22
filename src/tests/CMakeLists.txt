enable_testing()
set(TEST_ASSETS ${CMAKE_SOURCE_DIR}/test.obj ${CMAKE_SOURCE_DIR}/cube.mtl)
message(STATUS "Root CMake directory: ${CMAKE_SOURCE_DIR}")
add_executable(obj_loader_test ObjLoaderTest.cpp)
target_link_libraries(obj_loader_test 
	PRIVATE 
		obj_loader_lib
		GTest::gtest_main
)



add_custom_target(obj_loader_test_assets)
foreach(asset IN LISTS TEST_ASSETS)
    add_custom_command(
        TARGET obj_loader_test_assets
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${asset}
		${CMAKE_CURRENT_BINARY_DIR}/assets/
	 COMMENT "Copying ${asset} to test folder"
    )
endforeach()

add_dependencies(obj_loader_test obj_loader_test_assets)



include(GoogleTest)

gtest_discover_tests(obj_loader_test)
