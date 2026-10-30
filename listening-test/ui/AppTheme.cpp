// Listening Test Application
// Copyright(C) 2011 Roman Kosobrodov; Copyright(C) 2017-2026 Netflix, Inc.
#include "AppTheme.h"

namespace AppTheme {
const juce::Colour activeButtonColour = juce::Colours::hotpink;
const juce::Colour referenceButtonColour = juce::Colours::darkgreen;
const juce::Colour stimuliButtonColour = juce::Colours::darkslateblue;
const juce::Colour lineColour {238, 238, 209};
const juce::Colour transparent = juce::Colours::transparentBlack;

void setupLabel(juce::Label& label, int x, int y) {
    label.setBorderSize(juce::BorderSize<int>(0));
    const auto text = label.getText();
    int lines = 0, width = 0, start = 0;
    while (start < text.length()) {
        ++lines;
        int end = text.indexOfChar(start, '\n');
        if (end == -1) end = text.length();
        width = juce::jmax(width, label.getFont().getStringWidth(text.substring(start, end)));
        start = end + 1;
    }
    label.setBounds(x, y, width, (int) (label.getFont().getHeight() * lines));
}
}
