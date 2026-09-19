cmake_minimum_required(VERSION 3.16)
project(FeReader VERSION 3.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui Xml)

# pkg-config for system libs
find_package(PkgConfig REQUIRED)
pkg_check_modules(MUPDF REQUIRED mupdf)
pkg_check_modules(LIBZIP REQUIRED libzip)
pkg_check_modules(GUMBO REQUIRED gumbo)

# Sources
set(SOURCES
    src/main.cpp

    src/core/i18n.cpp
    src/core/config_manager.cpp

    src/render/pdf_engine.cpp
    src/render/epub_engine.cpp
    src/render/render_engine.cpp

    src/converter/converter.cpp

    src/ui/page_scroll_area.cpp
    src/ui/clickable_label.cpp
    src/ui/settings_dialog.cpp
    src/ui/convert_dialog.cpp
    src/ui/main_window.cpp
)

set(HEADERS
    src/core/types.h
    src/core/i18n.h
    src/core/config_manager.h

    src/render/pdf_engine.h
    src/render/epub_engine.h
    src/render/render_engine.h

    src/converter/converter.h

    src/ui/page_scroll_area.h
    src/ui/clickable_label.h
    src/ui/settings_dialog.h
    src/ui/convert_dialog.h
    src/ui/main_window.h
)

# Windows resources - compile manually with explicit -i -o flags
# (windres positional args fail with spaces in output path on Windows)
if(WIN32)
    set(RC_OBJ "${CMAKE_CURRENT_BINARY_DIR}/version.rc.obj")
    configure_file(version.rc "${CMAKE_CURRENT_BINARY_DIR}/version.rc" COPYONLY)
    configure_file(icon.ico "${CMAKE_CURRENT_BINARY_DIR}/icon.ico" COPYONLY)
    configure_file(app.manifest "${CMAKE_CURRENT_BINARY_DIR}/app.manifest" COPYONLY)
    add_custom_command(
        OUTPUT  ${RC_OBJ}
        COMMAND C:/msys64/ucrt64/bin/windres.exe
                --use-temp-file
                -O coff
                -i version.rc
                -o version.rc.obj
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/version.rc
                ${CMAKE_CURRENT_SOURCE_DIR}/icon.ico
                ${CMAKE_CURRENT_SOURCE_DIR}/app.manifest
        COMMENT "Compiling version.rc"
    )
    list(APPEND SOURCES ${RC_OBJ} resources.qrc)
endif()

add_executable(FeReader WIN32 ${SOURCES} ${HEADERS})

target_include_directories(FeReader PRIVATE
    src
    ${MUPDF_INCLUDE_DIRS}
    ${LIBZIP_INCLUDE_DIRS}
    ${GUMBO_INCLUDE_DIRS}
)

target_compile_options(FeReader PRIVATE
    ${MUPDF_CFLAGS_OTHER}
    ${LIBZIP_CFLAGS_OTHER}
    ${GUMBO_CFLAGS_OTHER}
)

target_link_libraries(FeReader PRIVATE
    Qt5::Widgets
    Qt5::Core
    Qt5::Gui
    Qt5::Xml
    ${MUPDF_LIBRARIES}
    ${LIBZIP_LIBRARIES}
    ${GUMBO_LIBRARIES}
)

# Windows extra libs needed by MuPDF
if(WIN32)
    target_link_libraries(FeReader PRIVATE
        advapi32 gdi32 comdlg32
    )
endif()

# Copy Qt5 DLLs next to the executable after build
if(WIN32)
    set(QT5_BIN_DIR "C:/msys64/ucrt64/bin")
    set(QT5_DLLS
        Qt5Core.dll Qt5Gui.dll Qt5Widgets.dll Qt5Xml.dll
        libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll
        libzstd.dll libpcre2-16-0.dll libdouble-conversion.dll
        libfreetype-6.dll libharfbuzz-0.dll libpng16-16.dll zlib1.dll
        libzip.dll libgumbo-3.dll libmupdf.dll libjbig2dec-0.dll
    )
    foreach(DLL ${QT5_DLLS})
        add_custom_command(TARGET FeReader POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT5_BIN_DIR}/${DLL}"
                "$<TARGET_FILE_DIR:FeReader>/${DLL}"
            COMMENT "Copying ${DLL}"
        )
    endforeach()
    # Copy Qt5 platform plugin
    add_custom_command(TARGET FeReader POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:FeReader>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT5_BIN_DIR}/../share/qt5/plugins/platforms/qwindows.dll"
            "$<TARGET_FILE_DIR:FeReader>/platforms/qwindows.dll"
        COMMENT "Copying Qt5 platform plugin"
    )
endif()
