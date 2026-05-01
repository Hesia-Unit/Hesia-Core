cmake_minimum_required(VERSION 3.16)
project(hesia_core VERSION 0.1.0 LANGUAGES C)

option(HESIA_CORE_BUILD_SHARED "Build a shared library" ON)
option(HESIA_CORE_BUILD_TESTS "Build HESIA Core tests" ON)
option(HESIA_CORE_WITH_OPENSSL "Enable OpenSSL AES-GCM and RAND provider" OFF)
option(HESIA_CORE_WITH_LIBOQS "Enable liboqs ML-KEM and ML-DSA provider" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

if(HESIA_CORE_BUILD_SHARED)
    add_library(hesia_core SHARED src/hesia_core.c)
    target_compile_definitions(hesia_core PRIVATE HESIA_CORE_EXPORTS HESIA_CORE_BUILD_SHARED)
else()
    add_library(hesia_core STATIC src/hesia_core.c)
endif()

target_include_directories(hesia_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(MSVC)
    target_compile_options(hesia_core PRIVATE /W4 /sdl)
else()
    target_compile_options(hesia_core PRIVATE -Wall -Wextra -Wpedantic -fstack-protector-strong)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_options(hesia_core PRIVATE -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack)
    endif()
endif()

if(HESIA_CORE_WITH_OPENSSL)
    find_package(OpenSSL REQUIRED)
    target_compile_definitions(hesia_core PRIVATE HESIA_CORE_WITH_OPENSSL)
    target_link_libraries(hesia_core PRIVATE OpenSSL::Crypto)
endif()

if(HESIA_CORE_WITH_LIBOQS)
    find_path(LIBOQS_INCLUDE_DIR NAMES oqs/oqs.h PATHS $ENV{LIBOQS_ROOT} PATH_SUFFIXES include)
    find_library(LIBOQS_LIBRARY NAMES oqs PATHS $ENV{LIBOQS_ROOT} PATH_SUFFIXES lib build/lib)
    if(NOT LIBOQS_INCLUDE_DIR OR NOT LIBOQS_LIBRARY)
        message(FATAL_ERROR "HESIA_CORE_WITH_LIBOQS=ON but liboqs was not found")
    endif()
    target_compile_definitions(hesia_core PRIVATE HESIA_CORE_WITH_LIBOQS)
    target_include_directories(hesia_core PRIVATE ${LIBOQS_INCLUDE_DIR})
    target_link_libraries(hesia_core PRIVATE ${LIBOQS_LIBRARY})
endif()

if(WIN32 AND NOT HESIA_CORE_WITH_OPENSSL)
    target_link_libraries(hesia_core PRIVATE bcrypt)
endif()

if(HESIA_CORE_BUILD_TESTS)
    enable_testing()
    add_executable(hesia_core_tests tests/test_core.c)
    target_link_libraries(hesia_core_tests PRIVATE hesia_core)
    add_test(NAME hesia_core_tests COMMAND hesia_core_tests)
endif()

install(TARGETS hesia_core
    EXPORT hesia_coreTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)
