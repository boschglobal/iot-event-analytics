#include_directories("${CMAKE_SOURCE_DIR}/include")

add_executable(perf_test_talent
    perf_test_talent.cpp
)

target_link_libraries(perf_test_talent
  PUBLIC
    iotea_sdk_cpp
)


add_executable(perf_tester
    perf_tester.cpp
)

target_link_libraries(perf_tester
  PUBLIC
    iotea_sdk_cpp
)
