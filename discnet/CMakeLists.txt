
# adding thirdparty dependencies
find_package(Boost REQUIRED COMPONENTS program_options log)
find_package(OpenSSL REQUIRED)
find_package(whatlog REQUIRED whatlog)

add_compile_definitions(DISCNET_DLL)

# setting up source files and header files groups
file(GLOB HEADERS_DEFAULT_GRP "${discnet_INCLUDE_DIRS}/discnet/*.hpp")
file(GLOB HEADERS_APPLICATION_GRP "${discnet_INCLUDE_DIRS}/discnet/application/*.hpp")
file(GLOB HEADERS_NETWORK_GRP "${discnet_INCLUDE_DIRS}/discnet/network/*.hpp")
file(GLOB HEADERS_NETWORK_MESSAGES_GRP "${discnet_INCLUDE_DIRS}/discnet/network/messages/*.hpp")
file(GLOB HEADERS_WINDOWS_GRP "${discnet_INCLUDE_DIRS}/discnet/windows/*.hpp")

file(GLOB SOURCE_DEFAULT_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB SOURCE_APPLICATION_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/application/*.cpp")
file(GLOB SOURCE_NETWORK_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/network/*.cpp")
file(GLOB SOURCE_NETWORK_MESSAGES_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/network/messages/*.cpp")
file(GLOB SOURCE_WINDOWS_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/*.cpp")

# adding file filters to visual studio project
source_group("header/" FILES ${HEADERS_DEFAULT_GRP})
source_group("header/application/" FILES ${HEADERS_APPLICATION_GRP})
source_group("header/network/" FILES ${HEADERS_NETWORK_GRP})
source_group("header/network/messages/" FILES ${HEADERS_NETWORK_MESSAGES_GRP})
source_group("header/windows/" FILES ${HEADERS_WINDOWS_GRP})
source_group("src/" FILES ${SOURCE_DEFAULT_GRP})
source_group("src/application/" FILES ${SOURCE_APPLICATION_GRP})
source_group("src/network/" FILES ${SOURCE_NETWORK_GRP})
source_group("src/network/messages/" FILES ${SOURCE_NETWORK_MESSAGES_GRP})
source_group("src/windows/" FILES ${SOURCE_WINDOWS_GRP})

# setting up executable
add_library(discnet SHARED
	${SOURCE_DEFAULT_GRP}
    ${SOURCE_APPLICATION_GRP}
    ${SOURCE_NETWORK_GRP}
    ${SOURCE_NETWORK_MESSAGES_GRP}
    ${SOURCE_WINDOWS_GRP}
    ${HEADERS_DEFAULT_GRP}
    ${HEADERS_NETWORK_GRP}
    ${HEADERS_NETWORK_MESSAGES_GRP}
    ${HEADERS_WINDOWS_GRP}
)

add_library(discnet::discnet_lib ALIAS discnet)

# include_directories(${discnet_INCLUDE_DIRS})
target_include_directories(discnet PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

# adding thirdparty library dependencies
target_link_libraries(discnet PUBLIC bcrypt)
target_link_libraries(discnet PRIVATE OpenSSL::Crypto)
target_link_libraries(discnet PRIVATE whatlog::whatlog)
target_link_libraries(discnet PRIVATE IPHLPAPI wbemuuid)
target_link_libraries(discnet PUBLIC Boost::log Boost::log_setup Boost::program_options)
