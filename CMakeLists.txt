cmake_minimum_required(VERSION 3.16)
project(hesia_core VERSION 0.1.0 LANGUAGES C)

option(HESIA_CORE_BUILD_SHARED "Build a shared library" ON)
option(HESIA_CORE_BUILD_TESTS "Build HESIA Core tests" ON)
option(HESIA_CORE_WITH_OPENSSL "Enable OpenSSL AES-GCM and RAND provider" ON)
option(HESIA_CORE_WITH_LIBOQS "Enable liboqs ML-KEM and ML-DSA provider" ON)
option(HESIA_CORE_VENDOR_DEPS "Prefer dependencies built under third_party/install" ON)

if(WIN32)
    set(HESIA_CORE_VENDOR_TRIPLE "windows-x86_64")
else()
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" HESIA_CORE_SYSTEM_NAME)
    set(HESIA_CORE_VENDOR_TRIPLE "${HESIA_CORE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
endif()
set(HESIA_CORE_DEPS_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/install/${HESIA_CORE_VENDOR_TRIPLE}"
    CACHE PATH "HESIA Core vendored dependency prefix")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

if(HESIA_CORE_BUILD_SHARED)
    add_library(hesia_core SHARED 
        src/hesia_core.c
        src/algorithms/ascon.c
    )
    target_compile_definitions(hesia_core PRIVATE HESIA_CORE_EXPORTS HESIA_CORE_BUILD_SHARED)
else()
    add_library(hesia_core STATIC 
        src/hesia_core.c
        src/algorithms/ascon.c
    )
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
    if(HESIA_CORE_VENDOR_DEPS)
        set(OPENSSL_ROOT_DIR "${HESIA_CORE_DEPS_ROOT}/openssl")
    endif()
    find_package(OpenSSL REQUIRED)
    target_compile_definitions(hesia_core PRIVATE HESIA_CORE_WITH_OPENSSL)
    target_link_libraries(hesia_core PRIVATE OpenSSL::Crypto)
endif()

if(HESIA_CORE_WITH_LIBOQS)
    set(HESIA_CORE_LIBOQS_PATHS
        "${HESIA_CORE_DEPS_ROOT}/liboqs"
        "$ENV{LIBOQS_ROOT}")
    find_path(LIBOQS_INCLUDE_DIR NAMES oqs/oqs.h PATHS ${HESIA_CORE_LIBOQS_PATHS} PATH_SUFFIXES include)
    find_library(LIBOQS_LIBRARY NAMES oqs liboqs PATHS ${HESIA_CORE_LIBOQS_PATHS} PATH_SUFFIXES lib build/lib)
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
    add_executable(hesia_ascon_kat tests/test_ascon_kat.c src/algorithms/ascon.c)
    target_include_directories(hesia_ascon_kat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME hesia_ascon_kat
        COMMAND hesia_ascon_kat ${CMAKE_CURRENT_SOURCE_DIR}/tests/kat/ascon128v12/LWC_AEAD_KAT_128_128.txt)
    if(HESIA_CORE_WITH_OPENSSL AND HESIA_CORE_WITH_LIBOQS)
        add_executable(hesia_core_provider_tests tests/test_providers.c)
        target_link_libraries(hesia_core_provider_tests PRIVATE hesia_core)
        add_test(NAME hesia_core_provider_tests COMMAND hesia_core_provider_tests)
    endif()
endif()

install(TARGETS hesia_core
    EXPORT hesia_coreTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)

# Tools
add_executable(hesia-cli tools/hesia-cli/main.c)
target_link_libraries(hesia-cli PRIVATE hesia_core)
install(TARGETS hesia-cli RUNTIME DESTINATION bin)
