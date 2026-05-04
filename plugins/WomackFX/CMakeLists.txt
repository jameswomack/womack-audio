juce_add_plugin(WomackFX
    COMPANY_NAME "Womack"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Wmck
    PLUGIN_CODE Wfx1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Womack FX"
)

target_sources(WomackFX
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Effects/FuzzEffect.cpp
        Source/Effects/UnivibeEffect.cpp
        Source/Effects/TapeDelayEffect.cpp
        Source/UI/MainPanel.cpp
        Source/UI/FuzzPanel.cpp
        Source/UI/UnivibePanel.cpp
        Source/UI/TapeDelayPanel.cpp
        Source/UI/Visualizer.cpp
)

target_compile_definitions(WomackFX
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(WomackFX
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
