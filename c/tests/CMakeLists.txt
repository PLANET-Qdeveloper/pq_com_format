add_executable(test_pq_com_format test_pq_com_format.c)
target_include_directories(test_pq_com_format PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/lib/include>)
target_link_libraries(test_pq_com_format pq_com_format)
add_test(NAME pq_com_format_test COMMAND test_pq_com_format)
