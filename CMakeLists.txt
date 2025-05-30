cmake_minimum_required(VERSION 3.10)
project(js-style-co-routine LANGUAGES CXX)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

install(
    DIRECTORY include/
    DESTINATION include/${PROJECT_NAME}
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
)

install(
    EXPORT ${PROJECT_NAME}Targets
    DESTINATION lib/cmake/${PROJECT_NAME}
)
