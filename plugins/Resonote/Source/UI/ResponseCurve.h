#pragma once

#include "Visualizer.h"
#include "../PluginProcessor.h"

/** Log-frequency response curve with faint vertical note gridlines and a
    marker at the current cutoff labelled with its locked note. */
class ResponseCurve : public Visualizer
{
public:
    explicit ResponseCurve (ResonoteProcessor& processor) : proc (processor) {}

    void paint (juce::Graphics& g) override;

private:
    static constexpr float minHz = 20.0f;
    static constexpr float maxHz = 20000.0f;

    static float hzToX (float hz, juce::Rectangle<float> area) noexcept;

    ResonoteProcessor& proc;
};
