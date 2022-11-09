
# adding thirdparty dependencies
find_package(fmt CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options log)
find_package(GTest CONFIG REQUIRED)
find_package(whatlog REQUIRED whatlog)

# enabling and adding google tests
enable_testing()
add_test(project_test_cases discnet)

# setting up source files and header files groups
file(GLOB HEADERS_DEFAULT_GRP "${discnet_INCLUDE_DIRS}/discnet_test/*.hpp")
file(GLOB SOURCE_DEFAULT_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# adding file filters to visual studio project
source_group("header/" FILES ${HEADERS_DEFAULT_GRP})
source_group("src/" FILES ${SOURCE_DEFAULT_GRP})

# adding thirdparty include directories
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${fmt_INCLUDE_DIRS})
include_directories(${discnet_INCLUDE_DIRS})
include_directories(${whatlog_INCLUDE_DIRS})

# setting up executable
add_executable(discnet_test
    ${HEADERS_DEFAULT_GRP}
	${SOURCE_DEFAULT_GRP}
)

# adding thirdparty library dependencies
target_link_libraries(discnet_test PRIVATE wsock32 ws2_32 IPHLPAPI wbemuuid bcrypt)
target_link_libraries(discnet_test PRIVATE Boost::log)
target_link_libraries(discnet_test PRIVATE fmt::fmt)
target_link_libraries(discnet_test PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)
target_link_libraries(discnet_test PRIVATE discnet)
target_link_libraries(discnet_test PRIVATE whatlog::whatlog)

############# force copy of dependencies to build folder #############

add_custom_command( 
	TARGET discnet_test POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_RUNTIME_DLLS:discnet_test>
        $<TARGET_FILE_DIR:discnet_test>
        COMMAND_EXPAND_LISTS
)