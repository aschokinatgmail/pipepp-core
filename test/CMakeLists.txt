cmake_minimum_required(VERSION 3.20)

find_package(GTest REQUIRED)
include(GoogleTest)

# === Unit Tests ===
add_executable(pipepp-unit-tests
    unit/test_config.cpp
    unit/test_expected.cpp
    unit/test_error_code.cpp
    unit/test_fixed_string.cpp
    unit/test_message.cpp
    unit/test_small_function.cpp
    unit/test_stages.cpp
    unit/test_concepts.cpp
    unit/test_pipeline.cpp
    unit/test_pipeline_ops.cpp
    unit/test_any_source.cpp
    unit/test_phase4.cpp
    unit/test_uri.cpp
    unit/test_source_registry.cpp

    unit/test_thread_safety.cpp
    unit/test_pipeline_factory.cpp
    unit/test_spinlock.cpp
    unit/test_run_emit.cpp
    unit/test_capacity.cpp
    unit/test_pipeline_coverage.cpp
    unit/test_fault_injection.cpp
    unit/test_function_coverage.cpp
)
target_link_libraries(pipepp-unit-tests PRIVATE pipepp-core GTest::gtest_main)
target_include_directories(pipepp-unit-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(pipepp-unit-tests PRIVATE -fno-rtti -fno-exceptions)
target_link_libraries(pipepp-unit-tests PRIVATE -pthread)
gtest_discover_tests(pipepp-unit-tests TEST_LIST UNIT_TESTS)

# === Integration Tests ===
add_executable(pipepp-integration-tests
    integration/test_pipeline_flow.cpp
    integration/test_pipe_syntax.cpp
    integration/test_connect_to.cpp
)
target_link_libraries(pipepp-integration-tests PRIVATE pipepp-core GTest::gtest_main)
target_include_directories(pipepp-integration-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(pipepp-integration-tests PRIVATE -fno-rtti -fno-exceptions)
gtest_discover_tests(pipepp-integration-tests TEST_LIST INTEGRATION_TESTS)

# === System Tests ===
add_executable(pipepp-system-tests
    system/test_full_pipeline.cpp
    system/test_embedded_config.cpp
)
target_link_libraries(pipepp-system-tests PRIVATE pipepp-core GTest::gtest_main)
target_include_directories(pipepp-system-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(pipepp-system-tests PRIVATE -fno-rtti -fno-exceptions)
gtest_discover_tests(pipepp-system-tests TEST_LIST SYSTEM_TESTS)

# === Coverage (optional) ===
if(PIPEPP_CORE_ENABLE_COVERAGE)
    foreach(tgt pipepp-unit-tests pipepp-integration-tests pipepp-system-tests)
        target_compile_options(${tgt} PRIVATE --coverage -O0 -g)
        target_link_options(${tgt} PRIVATE --coverage)
    endforeach()
endif()