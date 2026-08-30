cmake_minimum_required(VERSION 3.22)

project(QQSuperCompression VERSION 1.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Preferred path: point JUCE_PATH at an existing JUCE checkout.
# If no local JUCE is supplied, the project can fetch the pinned JUCE version.
set(JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout")
option(QQSC_FETCH_JUCE "Fetch JUCE automatically when JUCE_PATH is not set" ON)

if (JUCE_PATH AND EXISTS "${JUCE_PATH}/CMakeLists.txt")
    add_subdirectory("${JUCE_PATH}" JUCE)
elseif (QQSC_FETCH_JUCE)
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
else()
    message(FATAL_ERROR "JUCE was not found. Set -DJUCE_PATH=/path/to/JUCE or enable QQSC_FETCH_JUCE.")
endif()

set(QQSC_PLUGIN_FORMATS VST3)

if (APPLE)
    list(APPEND QQSC_PLUGIN_FORMATS AU)
endif()

juce_add_plugin(QQSuperCompression
    COMPANY_NAME "Qing Audio"
    BUNDLE_ID "com.qingaudio.qqsupercompression"
    PLUGIN_MANUFACTURER_CODE Qing
    PLUGIN_CODE Qscp
    FORMATS ${QQSC_PLUGIN_FORMATS}
    PRODUCT_NAME "QQ Super Compression"
    DESCRIPTION "Transparent lookahead dynamics processor with independent ST/LR/MS domains"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Dynamics
)

juce_generate_juce_header(QQSuperCompression)

# v0.9.3 onward: bitmap filmstrip UI was rejected by user testing.
# Rotary controls are again rendered by UTF8LookAndFeel; archived PNG assets remain in source history only.

target_sources(QQSuperCompression PRIVATE
    Source/Parameters.h
    Source/StaticCompressionEngine.h
    Source/MeterState.h
    Source/BS1770LoudnessMatch.h
    Source/UTF8LookAndFeel.h
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/DynamicDisplay.h
    Source/DynamicDisplay.cpp
    Source/LevelMeters.h
    Source/LevelMeters.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
)

target_compile_definitions(QQSuperCompression
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# Critical for Chinese/Japanese/Korean literals on MSVC. Source files are UTF-8.
if (MSVC)
    target_compile_options(QQSuperCompression PRIVATE /utf-8)
endif()

target_link_libraries(QQSuperCompression
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
