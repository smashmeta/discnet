
# project definition
set(PROJECT_NAME "discnet_app")
project(${PROJECT_NAME} VERSION ${DISCNET_VERSION} LANGUAGES CXX)

# adding thirdparty dependencies
find_package(Boost REQUIRED COMPONENTS program_options log log_setup)
find_package(GTest CONFIG REQUIRED)
find_package(spdlog REQUIRED)

# setting up source files and header files groups
file(GLOB HEADERS_DEFAULT_GRP "${discnet_INCLUDE_DIRS}/discnet_app/*.hpp")
file(GLOB SOURCE_DEFAULT_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# adding file filters to visual studio project
source_group("header/" FILES ${HEADERS_DEFAULT_GRP})
source_group("src/" FILES ${SOURCE_DEFAULT_GRP})

# setting up executable
add_executable(discnet_app
    ${HEADERS_DEFAULT_GRP}
	${SOURCE_DEFAULT_GRP}
)

# adding thirdparty library dependencies
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(discnet_app PRIVATE wsock32 ws2_32 IPHLPAPI wbemuuid bcrypt)
endif()
target_link_libraries(discnet_app PUBLIC Boost::log Boost::log_setup Boost::program_options)
target_link_libraries(discnet_app PRIVATE discnet::discnet_lib)
target_link_libraries(discnet_app PRIVATE spdlog::spdlog)
