add_executable(test_rect test_rect.cpp
		${CMAKE_SOURCE_DIR}/library/strings.cpp
		${CMAKE_SOURCE_DIR}/framework/logger.cpp)
target_link_libraries(test_rect ${FRAMEWORK_LIBRARIES})
add_test(NAME test_rect COMMAND ${EXECUTABLE_OUTPUT_PATH}/test_rect)
