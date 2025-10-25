
# adding thirdparty dependencies
find_package(Boost REQUIRED COMPONENTS program_options log)
find_package(GTest CONFIG REQUIRED)
find_package(spdlog REQUIRED)

# enabling and adding google tests
enable_testing()
add_test(project_test_cases discnet)

# setting up source files and header files groups
file(GLOB HEADERS_DEFAULT_GRP "${discnet_INCLUDE_DIRS}/discnet_test/*.hpp")
file(GLOB SOURCE_DEFAULT_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# adding file filters to visual studio project
source_group("header/" FILES ${HEADERS_DEFAULT_GRP})
source_group("src/" FILES ${SOURCE_DEFAULT_GRP})

# setting up executable
add_executable(discnet_test
    ${HEADERS_DEFAULT_GRP}
	${SOURCE_DEFAULT_GRP}
)

# adding thirdparty library dependencies
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(discnet_test PRIVATE wsock32 ws2_32 IPHLPAPI wbemuuid bcrypt)
endif()
target_link_libraries(discnet_test PRIVATE Boost::log)
target_link_libraries(discnet_test PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)
target_link_libraries(discnet_test PRIVATE discnet::discnet_lib)
target_link_libraries(discnet_test PRIVATE spdlog::spdlog_header_only)

# copying test resource files
configure_file(
        "${CMAKE_SOURCE_DIR}/resources/test/sha.file"
        "${CMAKE_BINARY_DIR}/discnet_test/sha.file"
        COPYONLY
    )