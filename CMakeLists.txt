cmake_minimum_required(VERSION 3.20)
project(pipepp-core VERSION 0.1.0 LANGUAGES CXX)

add_library(pipepp-core INTERFACE)
target_include_directories(pipepp-core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(pipepp-core INTERFACE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(pipepp-core INTERFACE -fno-rtti -fno-exceptions)
elseif(MSVC)
    target_compile_options(pipepp-core INTERFACE /GR- /EHs-c-)
endif()

add_library(pipepp::core ALIAS pipepp-core)

include(GNUInstallDirs)
install(TARGETS pipepp-core
    EXPORT pipepp-core-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/pipepp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT pipepp-core-targets
    FILE pipepp-core-targets.cmake
    NAMESPACE pipepp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pipepp-core
)

if(PIPEPP_CORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()