cmake_minimum_required(VERSION 3.24)

# adding windows version setting
add_definitions(-D_WIN32_WINNT=0x0600)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

# CPack settings
set(CPACK_PACKAGE_VENDOR "discnet")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "data delivery and discovery protocol.")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/smashmeta/discnet")
set(CPACK_PACKAGE_CONTACT "smashmeta")

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install")
set(CMAKE_TOOLCHAIN_FILE "C:/Users/Fredr/Desktop/void/programs/vcpkg/scripts/buildsystems/vcpkg.cmake")

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

add_compile_definitions(SPDLOG_USE_STD_FORMAT)

set(discnet_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include")

# Add a function to simplify the process
function(copy_runtime_dependencies target)
    if(WIN32 OR APPLE OR UNIX) # Applicable to all platforms that use dynamic libs
        add_custom_command(
            TARGET "${target}"
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                $<TARGET_RUNTIME_DLLS:${target}>
                $<TARGET_FILE_DIR:${target}>
            # COMMAND_EXPAND_LISTS is necessary to handle lists returned by generator expressions
            COMMAND_EXPAND_LISTS
            COMMENT "Copying runtime dependencies for ${target}..."
        )
    endif()
endfunction()

add_subdirectory(discnet)
add_subdirectory(discnet_test)
add_subdirectory(discnet_app)
add_subdirectory(discnet_sim)