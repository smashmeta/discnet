
# adding thirdparty dependencies
find_package(fmt CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options log)
find_package(GTest CONFIG REQUIRED)
find_package(whatlog REQUIRED whatlog)

# setting up source files and header files groups
file(GLOB HEADERS_DEFAULT_GRP "${discnet_INCLUDE_DIRS}/discnet_app/*.hpp")
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
add_executable(discnet_app
    ${HEADERS_DEFAULT_GRP}
	${SOURCE_DEFAULT_GRP}
)

# adding thirdparty library dependencies
target_link_libraries(discnet_app PRIVATE wsock32 ws2_32 IPHLPAPI wbemuuid bcrypt)
target_link_libraries(discnet_app PRIVATE Boost::log Boost::log_setup Boost::program_options)
target_link_libraries(discnet_app PRIVATE fmt::fmt)
target_link_libraries(discnet_app PRIVATE discnet)
target_link_libraries(discnet_app PRIVATE whatlog::whatlog)

############# force copy of dependencies to build folder #############

add_custom_command( 
	TARGET discnet_app POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_RUNTIME_DLLS:discnet_app>
        $<TARGET_FILE_DIR:discnet_app>
        COMMAND_EXPAND_LISTS
)