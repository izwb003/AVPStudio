cmake_minimum_required(VERSION 3.18)

project(AVPStudio VERSION 0.2.0 LANGUAGES CXX)

# Link ffmpeg
if(WIN32)
    set(FFMPEG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg)
    set(FFMPEG_VERSION 6.1.1)

    # Download ffmpeg gyan.dev builds from GitHub
    if(NOT EXISTS ${FFMPEG_PATH})
        file(DOWNLOAD
            https://github.com/GyanD/codexffmpeg/releases/download/${FFMPEG_VERSION}/ffmpeg-${FFMPEG_VERSION}-full_build-shared.zip
            ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg.zip
            SHOW_PROGRESS
        )
        file(ARCHIVE_EXTRACT
            INPUT ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg.zip
            DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/external
        )
        file(RENAME
            ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg-${FFMPEG_VERSION}-full_build-shared
            ${FFMPEG_PATH}
        )
        file(REMOVE ${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg.zip)
    endif()
endif(WIN32)

find_library(LIBAVUTIL_PATH avutil ${FFMPEG_PATH}/lib)
find_library(LIBAVCODEC_PATH avcodec ${FFMPEG_PATH}/lib)
find_library(LIBAVFORMAT_PATH avformat ${FFMPEG_PATH}/lib)
find_library(LIBAVFILTER_PATH avfilter ${FFMPEG_PATH}/lib)
find_library(LIBSWSCALE_PATH swscale ${FFMPEG_PATH}/lib)
find_library(LIBSWRESAMPLE_PATH swresample ${FFMPEG_PATH}/lib)

include_directories(${FFMPEG_PATH}/include)

# Link SDL
add_subdirectory(external/SDL)
include_directories(external/SDL/include)

# Necessary Qt settings
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add Qt packages
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools Multimedia MultimediaWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools Multimedia MultimediaWidgets)

# Set multi-language TS files
set(TS_FILES ts/AVPStudio_zh_CN.ts)

# Set project sources
set(FORMS_DIRECTORY src/forms)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

set(PROJECT_SOURCES
        ${SOURCES}
        ${TS_FILES}
        res/resources.qrc
)

if(WIN32)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/res/resources_win.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/res/resources_win.rc
        @ONLY
    )
    list(APPEND
        PROJECT_SOURCES
        ${CMAKE_CURRENT_BINARY_DIR}/res/resources_win.rc
    )
endif(WIN32)

#Pass version number
add_definitions(
    /DPROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    /DPROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    /DPROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Set Qt executables
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(AVPStudio
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
    qt_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR} ${TS_FILES})
else()
    add_executable(AVPStudio
        ${PROJECT_SOURCES}
    )
    qt5_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR} ${TS_FILES})
endif()

# Configure executable include rules
target_include_directories(AVPStudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(AVPStudio PRIVATE ${FORMS_DIRECTORY})

# MacOS settings for Qt < 6.1.0 (For Qt > 6.1.0 this is configured automatically)
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER tech.izwb.AVPStudio)
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

# Link libraries
target_link_libraries(AVPStudio PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::MultimediaWidgets
    ${LIBAVUTIL_PATH}
    ${LIBAVCODEC_PATH}
    ${LIBAVFORMAT_PATH}
    ${LIBAVFILTER_PATH}
    ${LIBSWSCALE_PATH}
    ${LIBSWRESAMPLE_PATH}
)

# Build tools
add_subdirectory(tools)

# Set install rules
install(TARGETS AVPStudio wavgenerator imageorganizer mxlplayer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Qt finalize executable
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(AVPStudio)
endif()
