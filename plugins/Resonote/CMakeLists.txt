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
