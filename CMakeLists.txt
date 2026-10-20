cmake_minimum_required(VERSION 3.16)

project(GoGame VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GOGAME_ENABLE_KATAGO "Build native KataGo model evaluation for HardAI" OFF)
if(GOGAME_ENABLE_KATAGO)
    include(cmake/KataGoBackend.cmake)
endif()

find_package(Qt6 6.2 COMPONENTS Quick Sql REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Concurrent
    Gui
    WebSockets
    Multimedia
)

qt_add_executable(appGoGame
    main.cpp
)

set_source_files_properties(
    qml/Singletons/AppState.qml
    qml/Core/Motion.qml
    qml/Core/Radius.qml
    qml/Core/Spacing.qml
    qml/Core/Theme.qml
    qml/Core/Typography.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

file(GLOB_RECURSE GAME_ASSETS
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/*"
)

foreach(asset IN LISTS GAME_ASSETS)
    file(RELATIVE_PATH asset_alias
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${asset}"
    )

    set_source_files_properties("${asset}" PROPERTIES
        QT_RESOURCE_ALIAS "${asset_alias}"
    )
endforeach()

qt_add_qml_module(appGoGame
    URI GoGame
    VERSION 1.0

    QML_FILES
        main.qml

        # Singletons
        qml/Singletons/AppState.qml

        # Core
        qml/Core/Motion.qml
        qml/Core/Radius.qml
        qml/Core/SceneManager.qml
        qml/Core/Spacing.qml
        qml/Core/Theme.qml
        qml/Core/Typography.qml

        # Components: Battle
        qml/Components/battle/GameClock.qml
        qml/Components/battle/PlayerPanel.qml

        # Components: Controls
        qml/Components/controls/GameButton.qml
        qml/Components/controls/GamePanel.qml
        qml/Components/controls/GameText.qml

        # Screens
        qml/Screens/SettingsScreen.qml
        qml/Screens/MatchScreen.qml
        qml/Screens/LoginScreen.qml
        qml/Screens/MainMenu.qml

    SOURCES
        main.cpp

        # Client AI
        src/AI/AIGame.cpp
        src/AI/SearchTypes.h
        src/AI/EasyAI.cpp
        src/AI/EasyAI.h
        src/AI/MediumAI.cpp
        src/AI/MediumAI.h
        src/AI/HardAI.cpp
        src/AI/HardAI.h
        src/AI/KataGoEvaluator.cpp
        src/AI/KataGoEvaluator.h

        # Shared Go rules
        src/Rules/GoMatch.cpp
        src/Rules/GoMatch.h

        # Core
        src/Core/Game.cpp
        src/Core/Game.h
        src/Core/Enums.h
        src/Core/Enumutils.h
        src/Core/Enumutils.cpp

        # Managers
        src/Managers/DataManager.cpp
        src/Managers/DataManager.h
        src/Managers/AssetManager.cpp
        src/Managers/AssetManager.h
        src/Managers/AuthManager.cpp
        src/Managers/AuthManager.h
        src/Managers/MatchmakingManager.h
        src/Managers/MatchmakingManager.cpp

        # Network
        src/Network/NetworkClient.cpp
        src/Network/NetworkClient.h
        src/Network/Protocol.cpp
        src/Network/Protocol.h

    RESOURCES
        ${GAME_ASSETS}
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appGoGame PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appGoGame
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appGoGame PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Managers"
)

target_link_libraries(appGoGame
    PRIVATE
        Qt6::Core
        Qt6::Concurrent
        Qt6::Gui
        Qt6::Quick
        Qt6::Qml
        Qt6::Network
        Qt6::WebSockets
        Qt6::Sql
)

include(GNUInstallDirs)
install(TARGETS appGoGame
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
add_subdirectory(server)

if(GOGAME_ENABLE_KATAGO)
    target_compile_definitions(appGoGame PRIVATE GOGAME_KATAGO)
    target_link_libraries(appGoGame PRIVATE gogameKataGoNative)
endif()
