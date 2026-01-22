
# project definition
set(PROJECT_NAME "discnet_sim")
project(${PROJECT_NAME} VERSION ${DISCNET_VERSION} LANGUAGES CXX)

# Enable automatic handling of moc, uic, and rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# adding thirdparty dependencies
find_package(Boost REQUIRED COMPONENTS program_options log log_setup)
find_package(GTest CONFIG REQUIRED)
find_package(spdlog REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# setting up source files and header files groups
file(GLOB HEADERS_DEFAULT_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
file(GLOB SOURCE_DEFAULT_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# adding file filters to visual studio project
source_group("header/" FILES ${HEADERS_DEFAULT_GRP})
source_group("src/" FILES ${SOURCE_DEFAULT_GRP})

# setting up executable
add_executable(discnet_sim
    ${HEADERS_DEFAULT_GRP}
	${SOURCE_DEFAULT_GRP}
)

# adding thirdparty library dependencies
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(discnet_sim PRIVATE wsock32 ws2_32 IPHLPAPI wbemuuid bcrypt)
endif()
target_link_libraries(discnet_sim PUBLIC Boost::log Boost::log_setup Boost::program_options)
target_link_libraries(discnet_sim PRIVATE discnet::discnet_lib)
target_link_libraries(discnet_sim PRIVATE spdlog::spdlog_header_only)
target_link_libraries(discnet_sim PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)

install(TARGETS discnet_sim
        DESTINATION bin)