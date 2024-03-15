# Set multi-language TS files
set(TS_FILES ts/imageorganizer_zh_CN.ts)

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

# Set Qt executables
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(imageorganizer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    add_executable(imageorganizer
        ${PROJECT_SOURCES}
    )
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

# Link libraries
target_link_libraries(imageorganizer PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    ${LIBAVUTIL_PATH}
    ${LIBAVFORMAT_PATH}
    ${LIBAVCODEC_PATH}
    ${LIBAVFILTER_PATH}
    ${LIBSWSCALE_PATH}
)

# MacOS settings for Qt < 6.1.0 (For Qt > 6.1.0 this is configured automatically)
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER tech.izwb.AVPStudio.imageorganizer)
endif()

# Modify Qt executable properties
set_target_properties(imageorganizer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Qt finalize executable
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(imageorganizer)
endif()
