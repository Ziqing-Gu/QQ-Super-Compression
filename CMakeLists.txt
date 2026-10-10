cmake_minimum_required(VERSION 3.22)

project(QQSuperCompression VERSION 1.3.4 LANGUAGES C CXX)

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

option(QQSC_BUILD_DYNAMICS_CHECK "Build actual processor dynamics regression checks" OFF)
if(QQSC_BUILD_DYNAMICS_CHECK)
    juce_add_console_app(QQSCLoudnessCheck PRODUCT_NAME QQSCLoudnessCheck)
    target_sources(QQSCLoudnessCheck PRIVATE tests/loudness_check.cpp)
    target_include_directories(QQSCLoudnessCheck PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/QQSuperCompression_artefacts/JuceLibraryCode")
    target_link_libraries(QQSCLoudnessCheck PRIVATE QQSuperCompression juce::juce_audio_utils juce::juce_dsp)
    if(MSVC)
        target_compile_options(QQSCLoudnessCheck PRIVATE /utf-8)
    endif()
    juce_add_console_app(QQSCUnityCheck PRODUCT_NAME "QQSCUnityCheck")
    target_sources(QQSCUnityCheck PRIVATE tests/unity_check.cpp)
    target_include_directories(QQSCUnityCheck PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/QQSuperCompression_artefacts/JuceLibraryCode")
    target_link_libraries(QQSCUnityCheck PRIVATE QQSuperCompression juce::juce_audio_utils juce::juce_dsp)
    if(MSVC)
        target_compile_options(QQSCUnityCheck PRIVATE /utf-8)
    endif()
    juce_add_console_app(QQSCCeilingCompare PRODUCT_NAME "QQSC Ceiling Compare")
    target_sources(QQSCCeilingCompare PRIVATE tests/ceiling_compare.cpp)
    target_include_directories(QQSCCeilingCompare PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/QQSuperCompression_artefacts/JuceLibraryCode")
    target_link_libraries(QQSCCeilingCompare PRIVATE QQSuperCompression juce::juce_audio_utils juce::juce_dsp)
    juce_add_console_app(QQSCCeilingCheck PRODUCT_NAME "QQSC Ceiling Check")
    target_sources(QQSCCeilingCheck PRIVATE tests/ceiling_check.cpp)
    target_include_directories(QQSCCeilingCheck PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/QQSuperCompression_artefacts/JuceLibraryCode")
    target_link_libraries(QQSCCeilingCheck PRIVATE QQSuperCompression juce::juce_audio_utils juce::juce_dsp)
    if(MSVC)
        target_compile_options(QQSCCeilingCheck PRIVATE /utf-8)
    endif()
endif()
if(QQSC_BUILD_DYNAMICS_CHECK)
    juce_add_console_app(QQSCStablePromotionCheck PRODUCT_NAME "QQSC Stable Promotion Check")
    target_sources(QQSCStablePromotionCheck PRIVATE tests/stable_promotion_check.cpp)
    target_include_directories(QQSCStablePromotionCheck PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/QQSuperCompression_artefacts/JuceLibraryCode")
    target_compile_definitions(QQSCStablePromotionCheck PRIVATE JUCE_PLUGINHOST_VST3=1)
    target_link_libraries(QQSCStablePromotionCheck PRIVATE QQSuperCompression juce::juce_audio_utils juce::juce_dsp)
    if(MSVC)
        target_compile_options(QQSCStablePromotionCheck PRIVATE /utf-8)
    endif()
    juce_add_console_app(QQSCPreviewVSTCheck PRODUCT_NAME "QQSC Preview VST Check")
    target_sources(QQSCPreviewVSTCheck PRIVATE tests/preview_vst_check.cpp)
    target_include_directories(QQSCPreviewVSTCheck PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/QQSuperCompression_artefacts/JuceLibraryCode")
    target_compile_definitions(QQSCPreviewVSTCheck PRIVATE JUCE_PLUGINHOST_VST3=1)
    target_link_libraries(QQSCPreviewVSTCheck PRIVATE QQSuperCompression juce::juce_audio_utils juce::juce_dsp)
    if(MSVC)
        target_compile_options(QQSCPreviewVSTCheck PRIVATE /utf-8)
    endif()
    juce_add_console_app(QQSCAlgorithmVSTCheck PRODUCT_NAME "QQSC Algorithm VST Check")
    target_sources(QQSCAlgorithmVSTCheck PRIVATE tests/algorithm_vst_check.cpp)
    target_include_directories(QQSCAlgorithmVSTCheck PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/QQSuperCompression_artefacts/JuceLibraryCode")
    target_compile_definitions(QQSCAlgorithmVSTCheck PRIVATE JUCE_PLUGINHOST_VST3=1)
    target_link_libraries(QQSCAlgorithmVSTCheck PRIVATE QQSuperCompression juce::juce_audio_utils juce::juce_dsp)
    if(MSVC)
        target_compile_options(QQSCAlgorithmVSTCheck PRIVATE /utf-8)
    endif()
    juce_add_console_app(QQSCDynamicsCheck PRODUCT_NAME "QQSC Dynamics Check")
    target_sources(QQSCDynamicsCheck PRIVATE tests/db_dynamics_test.cpp)
    target_include_directories(QQSCDynamicsCheck PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/QQSuperCompression_artefacts/JuceLibraryCode")
    target_link_libraries(QQSCDynamicsCheck PRIVATE QQSuperCompression juce::juce_audio_utils juce::juce_dsp)
    if(MSVC)
        target_compile_options(QQSCDynamicsCheck PRIVATE /utf-8)
    endif()
    juce_add_console_app(QQSCClickReplay PRODUCT_NAME "QQSC Click Replay")
    target_sources(QQSCClickReplay PRIVATE tests/click_replay.cpp)
    target_include_directories(QQSCClickReplay PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/QQSuperCompression_artefacts/JuceLibraryCode")
    target_link_libraries(QQSCClickReplay PRIVATE QQSuperCompression juce::juce_audio_utils juce::juce_dsp)
    if(MSVC)
        target_compile_options(QQSCClickReplay PRIVATE /utf-8)
    endif()
endif()

juce_add_plugin(QQSuperCompression
    COMPANY_NAME "Qing Audio"
    BUNDLE_ID "com.qingaudio.qqsupercompression"
    PLUGIN_MANUFACTURER_CODE Qing
    PLUGIN_CODE Qscp
    FORMATS VST3 AU
    PRODUCT_NAME "QQ Super Compression"
    DESCRIPTION "Transparent lookahead dynamics processor with Classic / Super algorithms and independent ST/LR/MS domains"
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
    Source/DynamicsLimits.h
    Source/ABTransfer.h
    Source/StaticCompressionEngine.h
    Source/MeterState.h
    Source/ExactEnergyIndex.h
    Source/BS1770LoudnessMatch.h
    Source/KWeightingFilter.h
    Source/IntegratedLoudnessMeter.h
    Source/UTF8LookAndFeel.h
    Source/WarmMaterial.h
    Source/WarmKnobAsset.h
    Source/LightKnobScale.h
    Source/DarkMaterial.h
    Source/RefinedDarkKnob.h
    Source/DarkPanelMaterial.h
    Source/PluginProcessor.h
    Source/LimiterMode.cpp
    Source/LimiterModeContinuity.h
    Source/LimiterEditor.cpp
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

if(QQSC_BUILD_DYNAMICS_CHECK)
    juce_add_console_app(QQSCLimiterCheck PRODUCT_NAME QQSCLimiterCheck)
    target_sources(QQSCLimiterCheck PRIVATE tests/limiter_check.cpp)
    target_include_directories(QQSCLimiterCheck PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/QQSuperCompression_artefacts/JuceLibraryCode")
    target_link_libraries(QQSCLimiterCheck PRIVATE QQSuperCompression juce::juce_audio_utils juce::juce_dsp)
    if(MSVC)
        target_compile_options(QQSCLimiterCheck PRIVATE /utf-8)
    endif()
endif()

if(QQSC_BUILD_DYNAMICS_CHECK)
    juce_add_console_app(QQSCLimiterVSTCheck PRODUCT_NAME QQSCLimiterVSTCheck)
    target_sources(QQSCLimiterVSTCheck PRIVATE tests/ratio_mix_vst_check.cpp)
    target_include_directories(QQSCLimiterVSTCheck PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/QQSuperCompression_artefacts/JuceLibraryCode")
    target_compile_definitions(QQSCLimiterVSTCheck PRIVATE JUCE_PLUGINHOST_VST3=1)
    target_link_libraries(QQSCLimiterVSTCheck PRIVATE QQSuperCompression juce::juce_audio_utils juce::juce_dsp)
    if(MSVC)
        target_compile_options(QQSCLimiterVSTCheck PRIVATE /utf-8)
    endif()
endif()


if(QQSC_BUILD_DYNAMICS_CHECK)
    juce_add_console_app(QQSCReviewCheck PRODUCT_NAME "QQSC Review Check")
    target_sources(QQSCReviewCheck PRIVATE tests/review_fixes_check.cpp)
    target_include_directories(QQSCReviewCheck PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/QQSuperCompression_artefacts/JuceLibraryCode")
    target_link_libraries(QQSCReviewCheck PRIVATE QQSuperCompression juce::juce_audio_utils juce::juce_dsp)
    if(MSVC)
        target_compile_options(QQSCReviewCheck PRIVATE /utf-8)
    endif()
endif()

if(QQSC_BUILD_DYNAMICS_CHECK)
    find_package(Threads REQUIRED)
    add_executable(QQSCMatchIndexCheck tests/match_index_check.cpp)
    target_link_libraries(QQSCMatchIndexCheck PRIVATE Threads::Threads)
    target_include_directories(QQSCMatchIndexCheck PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(QQSC_BUILD_DYNAMICS_CHECK)
    juce_add_console_app(QQSCSilenceVSTCheck PRODUCT_NAME QQSCSilenceVSTCheck)
    target_sources(QQSCSilenceVSTCheck PRIVATE tests/silence_vst_check.cpp)
    target_include_directories(QQSCSilenceVSTCheck PRIVATE Source "${CMAKE_CURRENT_BINARY_DIR}/QQSuperCompression_artefacts/JuceLibraryCode")
    target_compile_definitions(QQSCSilenceVSTCheck PRIVATE JUCE_PLUGINHOST_VST3=1)
    target_link_libraries(QQSCSilenceVSTCheck PRIVATE QQSuperCompression juce::juce_audio_utils juce::juce_dsp)
    if(MSVC)
        target_compile_options(QQSCSilenceVSTCheck PRIVATE /utf-8)
    endif()
endif()
