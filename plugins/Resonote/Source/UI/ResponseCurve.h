#pragma once

#include "Visualizer.h"
#include "../PluginProcessor.h"

/** Log-frequency response curve rendered through a "convex glass" lens: a
    horizontal fisheye magnifies the notes around the current cutoff and a subtle
    vertical bow lifts the center toward the viewer. Faint note gridlines recede
    with distance; an orange marker sits on the locked note. */
class ResponseCurve : public Visualizer
{
public:
    explicit ResponseCurve (ResonoteProcessor& processor) : proc (processor) {}

    void paint (juce::Graphics& g) override;

private:
    static constexpr float minHz = 20.0f;
    static constexpr float maxHz = 20000.0f;

    ResonoteProcessor& proc;
};
