cmake_minimum_required(VERSION 3.22)

project(QQSuperCompression VERSION 1.1.8 LANGUAGES C CXX)

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

juce_add_plugin(QQSuperCompression
    COMPANY_NAME "Qing Audio"
    BUNDLE_ID "com.qingaudio.qqsupercompression"
    PLUGIN_MANUFACTURER_CODE Qing
    PLUGIN_CODE Qscp
    FORMATS VST3 AU
    PRODUCT_NAME "QQ Super Compression"
    DESCRIPTION "Transparent lookahead dynamics processor with independent ST/LR/MS domains and external key"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Dynamics
)

juce_generate_juce_header(QQSuperCompression)

# The rejected v0.9.2 filmstrip stays archived. v1.1.6 uses the user's accepted
# live material compositor, with embedded lit/unlit textures and continuous light.
juce_add_binary_data(QQSCWarmKnobAssets
    NAMESPACE QQSCWarmKnobData
    HEADER_NAME QQSCWarmKnobData.h
    SOURCES Assets/WarmKnob/approved-lit.png Assets/WarmKnob/unlit-base.png)

target_sources(QQSuperCompression PRIVATE
    Source/Parameters.h
    Source/StaticCompressionEngine.h
    Source/MeterState.h
    Source/BS1770LoudnessMatch.h
    Source/UTF8LookAndFeel.h
    Source/WarmMaterial.h
    Source/WarmKnobAsset.h
    Source/LightKnobScale.h
    Source/DarkMaterial.h
    Source/RefinedDarkKnob.h
    Source/DarkPanelMaterial.h
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
        QQSCWarmKnobAssets
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Opt-in, offline rendering of the actual editor, never a mockup or DAW launch.
option(QQSC_BUILD_VISUAL_CHECK "Build the offscreen UI regression renderer" OFF)
if(QQSC_BUILD_VISUAL_CHECK)
    juce_add_console_app(QQSCVisualCheck PRODUCT_NAME "QQSC Visual Check")
    target_sources(QQSCVisualCheck PRIVATE tests/visual_check.cpp)
    target_include_directories(QQSCVisualCheck PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/QQSuperCompression_artefacts/JuceLibraryCode")
    target_link_libraries(QQSCVisualCheck PRIVATE QQSuperCompression juce::juce_audio_utils juce::juce_dsp)
    if(MSVC)
        target_compile_options(QQSCVisualCheck PRIVATE /utf-8)
    endif()
endif()
