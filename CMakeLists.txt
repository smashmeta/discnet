cmake_minimum_required(VERSION 3.20)

# CPack settings
set(CPACK_PACKAGE_VENDOR "discnet")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "data delivery and discovery protocol.")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/smashmeta/discnet")
set(CPACK_PACKAGE_CONTACT "smashmeta")

set(CMAKE_TOOLCHAIN_FILE "C:/Users/fredrhe/Desktop/void/programs/vcpkg/scripts/buildsystems/vcpkg.cmake")

set(DISCNET_VERSION 1.0.0)
project(discnet VERSION ${DISCNET_VERSION} LANGUAGES CXX)
add_compile_definitions(DISCNET_DLL)
set(CMAKE_CXX_STANDARD 20)

set(discnet_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include")

add_subdirectory(discnet_lib)
add_subdirectory(discnet_test)