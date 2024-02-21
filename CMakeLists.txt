cmake_minimum_required(VERSION 3.5)

project(AVPStudio VERSION 0.1 LANGUAGES CXX)

# Necessary Qt settings
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add Qt packages
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

# Set multi-language TS files
set(TS_FILES ts/AVPStudio_zh_CN.ts)

# Set project sources
set(FORMS_DIRECTORY src/forms)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

set(PROJECT_SOURCES
        ${SOURCES}
        ${TS_FILES}
)

include_directories(${FORMS_DIRECTORY})

# Set Qt executables
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(AVPStudio
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    add_executable(AVPStudio
        ${PROJECT_SOURCES}
    )
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

# Link Qt libraries
target_link_libraries(AVPStudio PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# MacOS settings for Qt < 6.1.0 (For Qt > 6.1.0 this is configured automatically)
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.AVPStudio)
endif()

# Modify Qt executable properties
set_target_properties(AVPStudio PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# GNU include
include(GNUInstallDirs)

# Set install rules
install(TARGETS AVPStudio
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Qt finalize executable
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(AVPStudio)
endif()
