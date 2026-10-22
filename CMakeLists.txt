cmake_minimum_required(VERSION 3.22)

# Static MSVC runtime avoids requiring the VC++ Redistributable on target PCs.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(OctaveHavoc VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  JUCE
  URL https://codeload.github.com/juce-framework/JUCE/zip/refs/tags/8.0.4
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(OctaveHavoc
  COMPANY_NAME "HavocLab"
  PLUGIN_MANUFACTURER_CODE Hvlb
  PLUGIN_CODE Ohvc
  FORMATS VST3 Standalone
  PRODUCT_NAME "Octave Havoc"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  VST3_CATEGORIES Fx "Pitch Shift"
  COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(OctaveHavoc PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp
)

target_compile_definitions(OctaveHavoc PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(OctaveHavoc
  PRIVATE
    juce::juce_audio_utils
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
