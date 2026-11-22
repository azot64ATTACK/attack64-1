cmake_minimum_required(VERSION 3.22)
project(Attack64 VERSION 0.1.0)

# Папка JUCE должна лежать рядом с этим файлом (git clone https://github.com/juce-framework/JUCE.git)
add_subdirectory(JUCE)

juce_add_plugin(Attack64
    COMPANY_NAME "MyCompany"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE Mycp
    PLUGIN_CODE A64s
    FORMATS VST3 Standalone
    PRODUCT_NAME "Attack64")

juce_generate_juce_header(Attack64)

target_sources(Attack64 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Attack64 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Attack64 PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_basics)

target_compile_features(Attack64 PRIVATE cxx_std_17)
