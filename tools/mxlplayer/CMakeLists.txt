# Set multi-language TS files
set(TS_FILES ts/mxlplayer_zh_CN.ts)

# Set project sources
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

set(PROJECT_SOURCES
    ${SOURCES}
    ${TS_FILES}
    ${CMAKE_SOURCE_DIR}/res/resources.qrc
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

# Copy icon
configure_file(
    ${CMAKE_SOURCE_DIR}/res/icons/mxlplayer_icon256.ico
    ${CMAKE_BINARY_DIR}/mxlplayer.ico
    COPYONLY
)

# Set Qt executables
qt_add_executable(mxlplayer
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)
qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

# Link libraries
target_link_libraries(mxlplayer PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    SDL2::SDL2
    ${LIBAVUTIL_PATH}
    ${LIBAVFORMAT_PATH}
    ${LIBAVCODEC_PATH}
    ${LIBAVFILTER_PATH}
    ${LIBSWSCALE_PATH}
    ${LIBSWRESAMPLE_PATH}
)

# MacOS settings for Qt < 6.1.0 (For Qt > 6.1.0 this is configured automatically)
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER tech.izwb.AVPStudio.mxlplayer)
endif()

# Modify Qt executable properties
set_target_properties(mxlplayer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Qt finalize executable
qt_finalize_executable(mxlplayer)
