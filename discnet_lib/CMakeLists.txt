
# adding thirdparty dependencies
find_package(fmt CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options log)
find_package(whatlog REQUIRED whatlog)


# setting up source files and header files groups
file(GLOB HEADERS_DEFAULT_GRP "${discnet_INCLUDE_DIRS}/discnet_lib/*.hpp")
file(GLOB HEADERS_NETWORK_GRP "${discnet_INCLUDE_DIRS}/discnet_lib/network/*.hpp")
file(GLOB HEADERS_NETWORK_MESSAGES_GRP "${discnet_INCLUDE_DIRS}/discnet_lib/network/messages/*.hpp")
file(GLOB HEADERS_WINDOWS_GRP "${discnet_INCLUDE_DIRS}/discnet_lib/windows/*.hpp")

file(GLOB SOURCE_DEFAULT_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB SOURCE_NETWORK_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/network/*.cpp")
file(GLOB SOURCE_NETWORK_MESSAGES_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/network/messages/*.cpp")
file(GLOB SOURCE_WINDOWS_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/*.cpp")

# adding file filters to visual studio project
source_group("header/" FILES ${HEADERS_DEFAULT_GRP})
source_group("header/network/" FILES ${HEADERS_NETWORK_GRP})
source_group("header/network/messages" FILES ${HEADERS_NETWORK_MESSAGES_GRP})
source_group("header/windows/" FILES ${HEADERS_WINDOWS_GRP})
source_group("src/" FILES ${SOURCE_DEFAULT_GRP})
source_group("src/network/" FILES ${SOURCE_NETWORK_GRP})
source_group("src/network/messages" FILES ${SOURCE_NETWORK_MESSAGES_GRP})
source_group("src/windows/" FILES ${SOURCE_WINDOWS_GRP})

# adding thirdparty include directories
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${fmt_INCLUDE_DIRS})
include_directories(${discnet_INCLUDE_DIRS})
include_directories(${whatlog_INCLUDE_DIRS})

# setting up executable
add_library(discnet_lib
	${SOURCE_DEFAULT_GRP}
    ${SOURCE_NETWORK_GRP}
    ${SOURCE_NETWORK_MESSAGES_GRP}
    ${SOURCE_WINDOWS_GRP}
    ${HEADERS_DEFAULT_GRP}
    ${HEADERS_NETWORK_GRP}
    ${HEADERS_NETWORK_MESSAGES_GRP}
    ${HEADERS_WINDOWS_GRP}
)

# adding thirdparty library dependencies
target_link_libraries(discnet_lib PRIVATE fmt::fmt)
target_link_libraries(discnet_lib PRIVATE whatlog::whatlog)