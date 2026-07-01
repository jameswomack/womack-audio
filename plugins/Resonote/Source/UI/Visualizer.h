#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class Visualizer : public juce::Component,
                   public juce::SettableTooltipClient,
                   public juce::Timer
{
public:
    Visualizer() { startTimerHz (30); }
    ~Visualizer() override { stopTimer(); }

    void timerCallback() override { repaint(); }
};
