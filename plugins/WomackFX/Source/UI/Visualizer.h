#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Base class for per-pedal visualizations
class Visualizer : public juce::Component,
                   public juce::SettableTooltipClient,
                   public juce::Timer
{
public:
    Visualizer() { startTimerHz (30); }
    ~Visualizer() override { stopTimer(); }

    void timerCallback() override { repaint(); }
};
