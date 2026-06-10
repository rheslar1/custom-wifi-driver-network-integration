cmake_minimum_required(VERSION 3.16)
project(custom_wifi_driver_network_integration LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_library(wifi_integration_core
  src/WifiIntegration.cpp
)
target_include_directories(wifi_integration_core PUBLIC include)
target_compile_options(wifi_integration_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE wifi_integration_core)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)

add_executable(wifi_integration_tests tests/WifiIntegrationTests.cpp)
target_link_libraries(wifi_integration_tests PRIVATE wifi_integration_core)
target_compile_options(wifi_integration_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME wifi_integration_tests COMMAND wifi_integration_tests)
