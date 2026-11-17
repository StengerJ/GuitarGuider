cmake_minimum_required(VERSION 3.22)

project(GuitarGuider VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(GG_BUILD_TESTS "Build the GuitarGuider unit tests" ON)
option(GG_BUILD_APP "Build the GuitarGuider application" ON)

# ---------------------------------------------------------------------------
# JUCE: fetched automatically. To use a local checkout instead, configure with
#   -DFETCHCONTENT_SOURCE_DIR_JUCE=path/to/JUCE
#
# JUCE's modules are compiled directly (rather than via juce_add_gui_app) so the
# build never has to run JUCE's helper executable, which keeps it working on
# machines where locally built tools are blocked (e.g. Windows Smart App Control).
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.17-security
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  do-not-add-subdirectory)
FetchContent_MakeAvailable(JUCE)
set(JUCE_MODULES_DIR "${juce_SOURCE_DIR}/modules")

find_package(Threads REQUIRED)

function(gg_add_juce_library target)
    set(modules ${ARGN})
    set(srcs "")
    foreach(m IN LISTS modules)
        file(GLOB unit_sources CONFIGURE_DEPENDS
            "${JUCE_MODULES_DIR}/${m}/${m}*.cpp"
            "${JUCE_MODULES_DIR}/${m}/${m}*.c")
        list(APPEND srcs ${unit_sources})
    endforeach()

    add_library(${target} STATIC ${srcs})
    target_include_directories(${target} PUBLIC "${JUCE_MODULES_DIR}")
    target_compile_definitions(${target} PUBLIC
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_STANDALONE_APPLICATION=1
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_MP3AUDIOFORMAT=1
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        $<$<CONFIG:Debug>:DEBUG=1 _DEBUG=1>
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG=1>)
    foreach(m IN LISTS modules)
        target_compile_definitions(${target} PUBLIC JUCE_MODULE_AVAILABLE_${m}=1)
    endforeach()

    if (MSVC)
        target_compile_options(${target} PRIVATE /bigobj /MP /W0)
        target_compile_definitions(${target} PUBLIC _CRT_SECURE_NO_WARNINGS NOMINMAX)
    elseif (UNIX AND NOT APPLE)
        find_package(PkgConfig REQUIRED)
        target_compile_options(${target} PRIVATE -w)
        if ("juce_graphics" IN_LIST modules)
            pkg_check_modules(GG_GFX REQUIRED IMPORTED_TARGET freetype2 fontconfig)
            target_link_libraries(${target} PUBLIC PkgConfig::GG_GFX)
        endif()
        if ("juce_audio_devices" IN_LIST modules)
            pkg_check_modules(GG_ALSA REQUIRED IMPORTED_TARGET alsa)
            target_link_libraries(${target} PUBLIC PkgConfig::GG_ALSA)
        endif()
        target_link_libraries(${target} PUBLIC rt dl)
    endif()
    target_link_libraries(${target} PUBLIC Threads::Threads)
endfunction()

# ---------------------------------------------------------------------------
# Core (GUI-free) sources: shared by the app and the tests
# ---------------------------------------------------------------------------
set(GG_CORE_SOURCES
    src/core/ChordLibrary.cpp
    src/core/StudyProgress.cpp
    src/core/QuizEngine.cpp
    src/core/StrumSynth.cpp
    src/core/PitchDetector.cpp
    src/core/ChordRecognizer.cpp
    src/dsp/Effects.cpp
    src/dsp/AmpSim.cpp
    src/dsp/SignalChain.cpp
    src/dsp/Presets.cpp
    src/audio/Session.cpp)

function(gg_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4 /wd4100 /wd4127 /wd4244 /wd4267 /MP /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wno-unused-parameter)
    endif()
endfunction()

# ---------------------------------------------------------------------------
# The application
# ---------------------------------------------------------------------------
if (GG_BUILD_APP)
    gg_add_juce_library(gg_juce_gui
        juce_core juce_events juce_data_structures juce_graphics juce_gui_basics
        juce_audio_basics juce_audio_devices juce_audio_formats juce_dsp)

    # Embed fonts as C++ arrays (replacement for juce_add_binary_data)
    set(GG_FONTS
        Barlow-Regular Barlow-Medium Barlow-SemiBold Barlow-Bold
        BarlowCondensed-Medium BarlowCondensed-SemiBold BarlowCondensed-Bold)
    set(GG_FONT_FILES "")
    foreach(f IN LISTS GG_FONTS)
        list(APPEND GG_FONT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/${f}.ttf")
    endforeach()
    set(GG_BINARY_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated/GGBinaryData.cpp")
    string(REPLACE ";" "|" GG_FONT_ARG "${GG_FONT_FILES}")
    add_custom_command(
        OUTPUT "${GG_BINARY_CPP}"
        COMMAND ${CMAKE_COMMAND} "-DOUT=${GG_BINARY_CPP}" "-DFILES=${GG_FONT_ARG}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedFiles.cmake"
        DEPENDS ${GG_FONT_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedFiles.cmake"
        COMMENT "Embedding fonts"
        VERBATIM)

    add_executable(GuitarGuider WIN32
        src/Main.cpp
        src/MainComponent.cpp
        src/audio/AudioEngine.cpp
        src/ui/Theme.cpp
        src/ui/Widgets.cpp
        src/ui/ChordDiagram.cpp
        src/learn/LearnPage.cpp
        src/learn/DeckBrowser.cpp
        src/learn/FlashcardsView.cpp
        src/learn/LearnModeView.cpp
        src/learn/MatchView.cpp
        src/learn/PlayAlongView.cpp
        src/studio/StudioPage.cpp
        src/studio/PedalboardView.cpp
        src/studio/AmpPanel.cpp
        src/studio/TrackArea.cpp
        src/studio/TransportBar.cpp
        src/tuner/TunerPage.cpp
        src/settings/SettingsPage.cpp
        ${GG_CORE_SOURCES}
        "${GG_BINARY_CPP}")
    if (WIN32)
        target_sources(GuitarGuider PRIVATE assets/GuitarGuider.rc)
    endif()
    target_include_directories(GuitarGuider PRIVATE src)
    target_compile_definitions(GuitarGuider PRIVATE
        JUCE_APPLICATION_NAME_STRING="GuitarGuider"
        JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}")
    target_link_libraries(GuitarGuider PRIVATE gg_juce_gui)
    gg_warnings(GuitarGuider)
endif()

# ---------------------------------------------------------------------------
# Tests (console, no GUI modules)
# ---------------------------------------------------------------------------
if (GG_BUILD_TESTS)
    enable_testing()
    gg_add_juce_library(gg_juce_headless
        juce_core juce_events juce_audio_basics juce_audio_formats juce_dsp)

    add_executable(GuitarGuiderTests tests/TestMain.cpp ${GG_CORE_SOURCES})
    target_include_directories(GuitarGuiderTests PRIVATE src)
    target_link_libraries(GuitarGuiderTests PRIVATE gg_juce_headless)
    gg_warnings(GuitarGuiderTests)
    add_test(NAME GuitarGuiderTests COMMAND GuitarGuiderTests)
endif()
