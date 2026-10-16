cmake_minimum_required(VERSION 3.22)

project(AnalogMasterStrip VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(AMS_BUILD_PLUGIN "Build the VST3 plugin" ON)
option(AMS_BUILD_TESTS "Build the unit tests" ON)

include(FetchContent)

add_library(analog_master_core STATIC
    Source/Core/BiquadFilter.cpp
    Source/Core/ButterworthCoeffs.cpp
    Source/Core/MultiPoleFilter.cpp
    Source/Core/LinkwitzRiley.cpp
    Source/Core/StereoFieldShaper.cpp
    Source/Core/MasterProcessorCore.cpp
)
target_include_directories(analog_master_core PUBLIC Source/Core)
set_target_properties(analog_master_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(AMS_BUILD_PLUGIN)
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.9
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(juce)

    juce_add_plugin(AnalogMasterStrip
        COMPANY_NAME "Alan Marcero"
        PRODUCT_NAME "Analog Master Strip"
        PLUGIN_MANUFACTURER_CODE Amrc
        PLUGIN_CODE Amst
        FORMATS VST3
        VST3_CATEGORIES Fx Mastering EQ
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE)

    target_sources(AnalogMasterStrip PRIVATE
        Source/Plugin/Parameters.cpp
        Source/Plugin/AnalogLookAndFeel.cpp
        Source/Plugin/VintageMeter.cpp
        Source/Plugin/PluginProcessor.cpp
        Source/Plugin/PluginEditor.cpp)

    target_include_directories(AnalogMasterStrip PRIVATE
        Source/Core
        Source/Plugin)

    target_compile_definitions(AnalogMasterStrip PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    target_link_libraries(AnalogMasterStrip
        PRIVATE
            analog_master_core
            juce::juce_audio_processors
            juce::juce_audio_utils
            juce::juce_gui_basics
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endif()

if(AMS_BUILD_TESTS)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.9.1
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(Catch2)

    add_executable(analog_master_tests
        Tests/BiquadFilterTests.cpp
        Tests/ButterworthCoeffsTests.cpp
        Tests/StereoFieldShaperTests.cpp
        Tests/MasterProcessorCoreTests.cpp)

    target_link_libraries(analog_master_tests PRIVATE analog_master_core Catch2::Catch2WithMain)

    enable_testing()
    include(Catch)
    catch_discover_tests(analog_master_tests)
endif()
