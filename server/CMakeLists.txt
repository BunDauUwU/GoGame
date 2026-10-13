cmake_minimum_required(VERSION 3.16)
project(GoGameServer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Network WebSockets)

add_library(goGameServer STATIC Server.cpp Server.h GoMatch.cpp GoMatch.h ByoYomiClock.h)
target_link_libraries(goGameServer PUBLIC Qt6::Core Qt6::Network Qt6::WebSockets)
target_include_directories(goGameServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_executable(appserver main.cpp)
target_link_libraries(appserver PRIVATE goGameServer)

include(GNUInstallDirs)
install(TARGETS appserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CTest)
if(BUILD_TESTING)
    qt_add_executable(clockTest tests/Clock.cpp)
    target_link_libraries(clockTest PRIVATE goGameServer)
    add_test(NAME byoYomiClock COMMAND clockTest)
    qt_add_executable(playersTest tests/Players.cpp
        ../src/Core/Game.cpp ../src/Core/Game.h
        ../src/Core/Enumutils.cpp ../src/Network/Protocol.cpp
        ../src/Network/NetworkClient.cpp ../src/Network/NetworkClient.h)
    set_target_properties(playersTest PROPERTIES AUTOMOC ON)
    target_link_libraries(playersTest PRIVATE goGameServer)
    add_test(NAME players COMMAND playersTest)
    set_tests_properties(players PROPERTIES TIMEOUT 30)
    qt_add_executable(goCommandsTest tests/GoCommands.cpp)
    target_link_libraries(goCommandsTest PRIVATE goGameServer)
    add_test(NAME goCommands COMMAND goCommandsTest)
    set_tests_properties(goCommands PROPERTIES TIMEOUT 30)
    qt_add_executable(serverIntegrationTest tests/Integration.cpp
        ../src/Network/NetworkClient.cpp ../src/Network/NetworkClient.h)
    set_target_properties(serverIntegrationTest PROPERTIES AUTOMOC ON)
    target_link_libraries(serverIntegrationTest PRIVATE goGameServer)
    add_test(NAME serverIntegration COMMAND serverIntegrationTest $<TARGET_FILE:appserver>)
    set_tests_properties(serverIntegration PROPERTIES TIMEOUT 30)
endif()
