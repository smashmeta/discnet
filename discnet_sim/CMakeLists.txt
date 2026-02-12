
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
find_package(spdlog CONFIG REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

# setting up source files and header files groups
file(GLOB HEADERS_DEFAULT_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h" )
file(GLOB HEADERS_SIM_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/simulator/*.h")
file(GLOB HEADERS_UI_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.h")
file(GLOB HEADERS_UI_WIDGETS_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/widgets/*.h")

file(GLOB QRC_DEFAULT_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/*.qrc")
file(GLOB SOURCE_DEFAULT_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB SOURCE_SIM_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/simulator/*.cpp")
file(GLOB SOURCE_UI_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cpp")
file(GLOB SOURCE_UI_WIDGETS_GRP "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/widgets/*.cpp")

# adding file filters to visual studio project
source_group("header/" FILES ${HEADERS_DEFAULT_GRP})
source_group("header/sim/" FILES ${HEADERS_SIM_GRP})
source_group("header/ui/" FILES ${HEADERS_UI_GRP})
source_group("header/ui/widgets/" FILES ${HEADERS_UI_WIDGETS_GRP})
source_group("src/" FILES ${SOURCE_DEFAULT_GRP})
source_group("src/sim/" FILES ${SOURCE_SIM_GRP})
source_group("src/ui/" FILES ${SOURCE_UI_GRP})
source_group("src/ui/widgets/" FILES ${SOURCE_UI_WIDGETS_GRP})

# setting up executable
add_executable(discnet_sim
    ${HEADERS_DEFAULT_GRP}
    ${HEADERS_SIM_GRP}
    ${HEADERS_UI_GRP}
    ${HEADERS_UI_WIDGETS_GRP}
	${QRC_DEFAULT_GRP}
    ${SOURCE_DEFAULT_GRP}
    ${SOURCE_SIM_GRP}
    ${SOURCE_UI_GRP}
    ${SOURCE_UI_WIDGETS_GRP}
)

target_include_directories(discnet_sim PRIVATE src)

# adding thirdparty library dependencies
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(discnet_sim PUBLIC wsock32 ws2_32 IPHLPAPI wbemuuid bcrypt)
endif()
target_link_libraries(discnet_sim PUBLIC OpenSSL::Crypto)
target_link_libraries(discnet_sim PUBLIC Boost::log Boost::log_setup Boost::program_options)
target_link_libraries(discnet_sim PUBLIC discnet::discnet_lib)
target_link_libraries(discnet_sim PRIVATE spdlog::spdlog)
target_link_libraries(discnet_sim PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets)
target_link_libraries(discnet_sim PRIVATE nlohmann_json::nlohmann_json)

copy_runtime_dependencies(discnet_sim)