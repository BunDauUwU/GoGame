find_package(Qt6 REQUIRED COMPONENTS Quick Sql Concurrent WebSockets)
qt_add_executable(soloAITest
    SoloAI.cpp
    ../src/AI/AIGame.cpp ../src/AI/EasyAI.cpp ../src/AI/MediumAI.cpp ../src/AI/HardAI.cpp
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

qt_add_executable(hardParallelTest HardParallel.cpp ../src/AI/HardAI.cpp ../src/Rules/GoMatch.cpp)
target_link_libraries(hardParallelTest PRIVATE Qt6::Core)
add_test(NAME hardParallel COMMAND hardParallelTest)
set_tests_properties(hardParallel PROPERTIES TIMEOUT 20)

if(GOGAME_ENABLE_KATAGO)
    qt_add_executable(kataGoTest KataGo.cpp ../src/AI/KataGoEvaluator.cpp ../src/AI/HardAI.cpp ../src/Rules/GoMatch.cpp)
    target_compile_definitions(kataGoTest PRIVATE GOGAME_KATAGO)
    target_link_libraries(kataGoTest PRIVATE Qt6::Core gogameKataGoNative)
    set(GOGAME_KATAGO_TEST_MODEL "" CACHE FILEPATH "Optional real model for native inference tests")
    add_test(NAME kataGoNative COMMAND kataGoTest ${GOGAME_KATAGO_TEST_MODEL})
    set_tests_properties(kataGoNative PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
endif()
