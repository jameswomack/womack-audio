# --- Resonote plugin -------------------------------------------------------
juce_add_plugin(Resonote
    COMPANY_NAME "Womack"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Wmck
    PLUGIN_CODE Rnt1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Womack Resonote")

target_sources(Resonote
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/ResonantSVF.cpp
        Source/UI/Visualizer.cpp
        Source/UI/ResonotePanel.cpp
        Source/UI/ResponseCurve.cpp)

target_compile_definitions(Resonote
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Resonote
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Unit test console app -------------------------------------------------
juce_add_console_app(ResonoteTests
    PRODUCT_NAME "ResonoteTests")

target_sources(ResonoteTests
    PRIVATE
        Tests/NoteFrequencyTests.cpp
        Source/DSP/ResonantSVF.cpp)

target_compile_definitions(ResonoteTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(ResonoteTests
    PRIVATE
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
