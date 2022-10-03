
# adding thirdparty dependencies
find_package(fmt CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(whatlog REQUIRED whatlog)


# setting up source files and header files groups
file(GLOB HEADERS_DEFAULT_GRP "${discnet_INCLUDE_DIRS}/discnet_lib/*.hpp")
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
add_library(discnet_lib
    ${HEADERS_DEFAULT_GRP}
	${SOURCE_DEFAULT_GRP}
)

# adding thirdparty library dependencies
target_link_libraries(discnet_lib PRIVATE ${Boost_LIBRARIES})
target_link_libraries(discnet_lib PRIVATE fmt::fmt)
target_link_libraries(discnet_lib PRIVATE ${whatlog_LIBRARIES})