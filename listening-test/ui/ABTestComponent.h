// Listening Test Application
// Copyright(C) 2017-2026 Netflix, Inc.
#pragma once
#include "BaseTestComponent.h"

class ABTestComponent : public BaseTestComponent {
public:
    ABTestComponent(AudioPlayer&, TestLauncher&, Component* video = nullptr);
    void createButtons() override;
    void restoreResponses() override;
    void resized() override;
private:
    void storeResponses() override;
    PreferenceControls preference;
    Label buttonHelp;
    Component* const video;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ABTestComponent)
};
