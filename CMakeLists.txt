cmake_minimum_required(VERSION 3.22)
project(AGO VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt)
    add_subdirectory(JUCE)
elseif (DEFINED JUCE_DIR AND EXISTS ${JUCE_DIR}/CMakeLists.txt)
    add_subdirectory(${JUCE_DIR} juce)
else()
    include(FetchContent)
    FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 7.0.12 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(juce)
endif()

juce_add_plugin(AGO
    COMPANY_NAME "audiouser69"
    BUNDLE_ID com.audiouser69.ago
    PRODUCT_NAME "AGO"
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_AUTO_MANIFEST FALSE
    PLUGIN_MANUFACTURER_CODE Efal
    PLUGIN_CODE Agoc
    VST3_CATEGORIES Instrument Generator)

juce_generate_juce_header(AGO)

target_sources(AGO PRIVATE
    Source/Engine/Engine.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(AGO PRIVATE Source)

target_compile_definitions(AGO PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(AGO
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# host di prova (solo Linux, non va nel pacchetto)
option(AGO_HOST_TEST "Compila il host di prova" OFF)
if (AGO_HOST_TEST)
    juce_add_console_app(ago_host_test PRODUCT_NAME "ago_host_test")
    juce_generate_juce_header(ago_host_test)
    target_sources(ago_host_test PRIVATE Tests/host_test.cpp Source/Engine/Engine.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(ago_host_test PRIVATE Source)
    target_compile_definitions(ago_host_test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(ago_host_test PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
