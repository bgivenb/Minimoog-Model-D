cmake_minimum_required(VERSION 3.22)
project(EmberModel3 VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.deps/JUCE/CMakeLists.txt")
    add_subdirectory(.deps/JUCE)
else()
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 91ad83ae34a81e0833b1a2b0866f54846370ae53)
    FetchContent_MakeAvailable(JUCE)
endif()
juce_add_plugin(EmberModel3
    COMPANY_NAME "Given Peace"
    BUNDLE_ID "audio.ember.model3"
    PLUGIN_MANUFACTURER_CODE Embr
    PLUGIN_CODE Emb3
    FORMATS VST3 Standalone
    PRODUCT_NAME "Model D 1970"
    IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(EmberModel3 PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(EmberModel3 PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(EmberModel3 PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
option(EMBER_BUILD_TESTS "Build offline audio and VST3 integration checks" ON)
if(EMBER_BUILD_TESTS)
    juce_add_console_app(EmberChecks PRODUCT_NAME "Ember Checks")
    target_sources(EmberChecks PRIVATE Tests/Checks.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(EmberChecks PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_PLUGINHOST_VST3=1)
    target_link_libraries(EmberChecks PRIVATE juce::juce_audio_utils juce::juce_dsp)
    enable_testing()
    add_test(NAME EmberVST3 COMMAND EmberChecks "$<TARGET_FILE_DIR:EmberModel3_VST3>/../.."
        "${CMAKE_CURRENT_SOURCE_DIR}/dist")
endif()
