cmake_minimum_required(VERSION 3.20)

# adding windows version setting
add_definitions(-D_WIN32_WINNT=0x0600)

# CPack settings
set(CPACK_PACKAGE_VENDOR "discnet")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "data delivery and discovery protocol.")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/smashmeta/discnet")
set(CPACK_PACKAGE_CONTACT "smashmeta")

set(CMAKE_TOOLCHAIN_FILE "C:/Users/smashcomp/Desktop/void/libraries/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(whatlog_DIR "C:/Users/smashcomp/Desktop/void/projects/whatlog/install/cmake")

set(DISCNET_VERSION 1.0.0)
project(discnet VERSION ${DISCNET_VERSION} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

if (MSVC)
	add_compile_options(/W4)
	add_compile_definitions(_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
else()
	add_compile_options(-Wall -pedantic)
endif()

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

set(discnet_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include")

add_subdirectory(discnet_lib)
add_subdirectory(discnet_test)