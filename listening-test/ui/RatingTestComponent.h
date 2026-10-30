// Listening Test Application
// Copyright(C) 2011 Roman Kosobrodov
// Copyright(C) 2017-2026 Netflix, Inc.
#pragma once
#include "BaseTestComponent.h"

class RatingTestComponent : public BaseTestComponent {
public:
    RatingTestComponent(AudioPlayer&, TestLauncher&, testEnum);
    void createButtons() override;
    void restoreResponses() override;
    void resized() override;
    bool keyPressed(const KeyPress&) override;
    void storeResponses() override;
private:
    void selectionChanged(int index) override;
    const testEnum type;
    RatingControls ratings;
};
