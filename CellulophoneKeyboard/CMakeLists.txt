cmake_minimum_required(VERSION 3.19)

project(CellulophoneKeyboard)

set_plugin_standard_srcs(${PROJECT_NAME} TRUE TRUE)

if(WIN32)
    set(platform_srcs
        CellulophoneKeyboard.rc
        ${se_sdk_folder}/mp_sdk.def
    )
    # Falls mp_sdk.def im neuen SDK im Unterordner liegt, fangen wir das sauber ab:
    if(NOT EXISTS "${se_sdk_folder}/mp_sdk.def")
        set(platform_srcs CellulophoneKeyboard.rc)
    endif()
elseif(APPLE)
    set(platform_srcs
        CellulophoneKeyboard.xml
    )
    set_source_files_properties(CellulophoneKeyboard.xml PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
endif()

add_library(${PROJECT_NAME} MODULE
    ${sdk_srcs}
    ${srcs}
    ${platform_srcs}
)

# Wichtig: Damit der Windows-Ressourcencompiler die XML-Datei findet und einbettet!
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".sem"
)

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "sem"
    )
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa" "-framework CoreFoundation" "-framework CoreGraphics" "-framework CoreText")
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} d2d1 dwrite windowscodecs)
    # 64-Bit Windows exportiert MP_GetFactory über die Standard-Aufrufkonvention:
    target_link_options(${PROJECT_NAME} PRIVATE "/EXPORT:MP_GetFactory")
endif()
