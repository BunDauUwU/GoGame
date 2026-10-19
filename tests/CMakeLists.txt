find_package(Qt6 REQUIRED COMPONENTS Quick Sql Concurrent WebSockets)
qt_add_executable(soloAITest
    SoloAI.cpp
    ../src/AI/AIGame.cpp ../src/AI/MediumAI.cpp ../src/AI/HardAI.cpp
    ../src/Core/Game.cpp ../src/Core/Game.h ../src/Core/Enumutils.cpp
    ../src/Managers/DataManager.cpp
    ../src/Managers/AuthManager.cpp ../src/Managers/AuthManager.h
    ../src/Managers/AssetManager.cpp ../src/Managers/AssetManager.h
    ../src/Managers/MatchmakingManager.cpp ../src/Managers/MatchmakingManager.h
    ../src/Network/NetworkClient.cpp ../src/Network/NetworkClient.h
    ../src/Network/Protocol.cpp ../src/Rules/GoMatch.cpp)
target_link_libraries(soloAITest PRIVATE
    Qt6::Quick Qt6::Sql Qt6::Concurrent Qt6::WebSockets)
target_compile_definitions(soloAITest PRIVATE GOGAME_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME soloAI COMMAND soloAITest)
set_tests_properties(soloAI PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(hardAITest HardAI.cpp ../src/AI/HardAI.cpp ../src/Rules/GoMatch.cpp)
target_link_libraries(hardAITest PRIVATE Qt6::Core)
add_test(NAME hardAI COMMAND hardAITest)
set_tests_properties(hardAI PROPERTIES TIMEOUT 20)

# Include HardAI.cpp inside this test to inspect private engine invariants only.
qt_add_executable(hardCoreTest HardMCTS.cpp ../src/Rules/GoMatch.cpp)
target_link_libraries(hardCoreTest PRIVATE Qt6::Core)
add_test(NAME hardCore COMMAND hardCoreTest)
set_tests_properties(hardCore PROPERTIES TIMEOUT 20)
