cmake_minimum_required(VERSION 3.16)
project(zune-deploy-native)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-fdiagnostics-color=always")

# api.cpp uses std::filesystem
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options for andriod-file-transfer-linux
set(BUILD_MTPZ       ON  CACHE BOOL "" FORCE)
set(BUILD_FUSE       OFF CACHE BOOL "" FORCE)
set(BUILD_PYTHON     OFF CACHE BOOL "" FORCE)
set(BUILD_TAGLIB     OFF CACHE BOOL "" FORCE)
set(BUILD_QT_UI      OFF CACHE BOOL "" FORCE)

# macOS: static-link AFTL into the single dylib; Linux keeps the upstream shared mtp-ng
if (APPLE)
    set(BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)
    # openssl@3 is keg-only under Homebrew; hint find_package to it when available
    if (NOT OPENSSL_ROOT_DIR)
        find_program(BREW brew)
        if (BREW)
            execute_process(COMMAND ${BREW} --prefix openssl@3 OUTPUT_VARIABLE OPENSSL_ROOT_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
        endif()
    endif()
else()
    set(BUILD_SHARED_LIB ON  CACHE BOOL "" FORCE)
endif()

add_subdirectory(vendor/android-file-transfer-linux)

add_library(${PROJECT_NAME} SHARED
    src/api.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE vendor/android-file-transfer-linux/mtp)

if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE mtp-ng-static)
    # a static archive doesn't propagate mtp-ng-static's PRIVATE deps; link them here
    find_package(OpenSSL REQUIRED)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(CORE_LIBRARY CoreFoundation)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto ${IOKIT_LIBRARY} ${CORE_LIBRARY})
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE mtp-ng)
endif()
