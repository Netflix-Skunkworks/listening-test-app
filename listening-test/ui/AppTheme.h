// Listening Test Application
// Copyright(C) 2011 Roman Kosobrodov; Copyright(C) 2017-2026 Netflix, Inc.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace AppTheme {
constexpr int positionMaximum = 100;
constexpr int transportRefreshMs = 100;
// ITU-R BS.1534 specifies a minimum playback loop of 500 ms.
constexpr double minimumLoopSeconds = 0.5;
constexpr int defaultHeight = 600, defaultWidth = 900;
constexpr int buttonHeight = 32, buttonWidth = 96;
constexpr float relativeButtonSpacing = 0.75f, disabledSliderAlpha = 0.4f;
extern const juce::Colour activeButtonColour, referenceButtonColour, stimuliButtonColour, lineColour, transparent;
void setupLabel(juce::Label&, int x, int y);
}
