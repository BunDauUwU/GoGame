cmake_minimum_required(VERSION 3.16)

project(GoGame VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.2 COMPONENTS Quick Sql REQUIRED)
find_package(Qt6 6.2 COMPONENTS Quick REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS WebSockets)
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Multimedia
)

qt_add_executable(appGoGame
    main.cpp
)

set_source_files_properties(
    qml/Core/Constants.qml
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

        # Core
        qml/Core/Constants.qml
        qml/Core/Motion.qml
        qml/Core/Radius.qml
        qml/Core/SceneManager.qml
        qml/Core/Spacing.qml
        qml/Core/Theme.qml
        qml/Core/Typography.qml

        # Components: Battle
        qml/Components/battle/PlayerPanel.qml
        qml/Components/battle/CountdownClock.qml
        qml/Components/battle/ElementPointBar.qml

        # Components: Cards
        qml/Components/cards/Card.qml
        qml/Components/cards/CharacterCard.qml
        qml/Components/cards/SupportCard.qml

        # Components: Common
        qml/Components/common/Loading.qml
        qml/Components/common/Popup.qml

        # Components: Controls
        qml/Components/controls/GameButton.qml
        qml/Components/controls/GameIcon.qml
        qml/Components/controls/GameImage.qml
        qml/Components/controls/GamePanel.qml
        qml/Components/controls/GameText.qml

        # Components: Layout
        qml/Components/layout/GameBackground.qml
        qml/Components/layout/GamePanel.qml

        # Screens
        qml/Screens/SettingsScreen.qml
        qml/Screens/Board.qml
        qml/Screens/LoginScreen.qml
        qml/Screens/MainMenu.qml

    SOURCES
        main.cpp

        # Core
        src/Core/AI.cpp
        src/Core/AI.h
        src/Core/Game.cpp
        src/Core/Game.h
        src/Core/Enums.h
        src/Core/Enumutils.h
        src/Core/Enumutils.cpp

        # Managers
        src/Managers/AssetManager.cpp
        src/Managers/AssetManager.h
        src/Managers/AuthManager.cpp
        src/Managers/AuthManager.h
        src/Managers/MatchmakingManager.h
        src/Managers/MatchmakingManager.cpp

        # Models
        src/Models/Board.cpp
        src/Models/Board.h
        src/Models/UI.cpp
        src/Models/UI.h

        # Network
        src/Network/NetworkClient.cpp
        src/Network/NetworkClient.h
        src/Network/Protocol.cpp
        src/Network/Protocol.h

    RESOURCES
        users.sqlite
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
        Qt6::Gui
        Qt6::Multimedia
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
