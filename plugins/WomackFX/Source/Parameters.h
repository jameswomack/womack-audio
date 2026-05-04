#pragma once

#include <juce_core/juce_core.h>

namespace ParamIDs
{
    // Fuzz
    inline constexpr const char* fuzzEnabled    = "fuzzEnabled";
    inline constexpr const char* fuzzDrive      = "fuzzDrive";
    inline constexpr const char* fuzzTone       = "fuzzTone";
    inline constexpr const char* fuzzMix        = "fuzzMix";
    inline constexpr const char* fuzzType       = "fuzzType";

    // Univibe
    inline constexpr const char* vibeEnabled    = "vibeEnabled";
    inline constexpr const char* vibeRate       = "vibeRate";
    inline constexpr const char* vibeDepth      = "vibeDepth";
    inline constexpr const char* vibeFeedback   = "vibeFeedback";
    inline constexpr const char* vibeMix        = "vibeMix";

    // Tape Delay
    inline constexpr const char* delayEnabled   = "delayEnabled";
    inline constexpr const char* delayTime      = "delayTime";
    inline constexpr const char* delayFeedback  = "delayFeedback";
    inline constexpr const char* delayMod       = "delayMod";
    inline constexpr const char* delaySat       = "delaySat";
    inline constexpr const char* delayMix       = "delayMix";
}
