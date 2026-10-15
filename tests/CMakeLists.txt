find_package(Qt6 REQUIRED COMPONENTS Quick Sql Concurrent WebSockets)
qt_add_executable(soloAITest
    SoloAI.cpp
    ../src/AI/AIGame.cpp ../src/AI/MediumAI.cpp
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
