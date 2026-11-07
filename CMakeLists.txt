cmake_minimum_required(VERSION 3.22)
project(SteamSynth VERSION 0.9.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)
add_library(steamsynth_dsp INTERFACE)
target_include_directories(steamsynth_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(BUILD_TESTING)
  add_executable(steamsynth_dsp_tests tests/dsp_tests.cpp)
  target_link_libraries(steamsynth_dsp_tests PRIVATE steamsynth_dsp)
  add_test(NAME SteamSynthDSPStress COMMAND steamsynth_dsp_tests)
endif()
option(STEAMSYNTH_BUILD_PLUGIN "Build JUCE VST3 + standalone" OFF)
if(STEAMSYNTH_BUILD_PLUGIN)
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
  juce_add_binary_data(SteamSynthUIAssets SOURCES assets/ui/faceplate.jpg assets/ui/knob_atlas.png)
  juce_add_plugin(SteamSynth
    COMPANY_NAME "ThomC"
    BUNDLE_ID "com.thomc.steamsynth"
    PLUGIN_MANUFACTURER_CODE Thmc
    PLUGIN_CODE StSy
    FORMATS VST3 Standalone
    PRODUCT_NAME "SteamSynth"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
  juce_generate_juce_header(SteamSynth)
  target_sources(SteamSynth PRIVATE src/plugin/PluginProcessor.cpp src/plugin/PluginEditor.cpp)
  target_include_directories(SteamSynth PRIVATE src)
  target_link_libraries(SteamSynth PRIVATE steamsynth_dsp SteamSynthUIAssets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_processors juce::juce_gui_extra juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
  target_compile_definitions(SteamSynth PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  if(BUILD_TESTING)
    juce_add_console_app(steamsynth_plugin_smoke PRODUCT_NAME "SteamSynth Plugin Smoke")
    juce_generate_juce_header(steamsynth_plugin_smoke)
    target_sources(steamsynth_plugin_smoke PRIVATE tests/plugin_smoke.cpp src/plugin/PluginProcessor.cpp src/plugin/PluginEditor.cpp)
    target_include_directories(steamsynth_plugin_smoke PRIVATE src)
    target_link_libraries(steamsynth_plugin_smoke PRIVATE steamsynth_dsp SteamSynthUIAssets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_processors juce::juce_gui_extra juce::juce_recommended_config_flags)
    target_compile_definitions(steamsynth_plugin_smoke PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
    add_test(NAME SteamSynthPluginSmoke COMMAND steamsynth_plugin_smoke)
  endif()
endif()