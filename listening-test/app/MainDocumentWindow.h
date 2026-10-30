// Listening Test Application
// Copyright(C) 2017-2026 Netflix, Inc.
#pragma once

#include <JuceHeader.h>
#include "../session/TestTypes.h"

class MainDocumentWindow : public DocumentWindow, private AsyncUpdater {
public:
    // Takes ownership of content. Tests can omit the native desktop peer.
    MainDocumentWindow(const String& name, Component* content, bool addToDesktop = true);
    ~MainDocumentWindow() override;

    void setTestType(testEnum);
    void closeButtonPressed() override;
    void moved() override;
    void resized() override;
    void parentSizeChanged() override;
    void minimisationStateChanged(bool) override;

private:
    void handleAsyncUpdate() override;
    void keepMaximised();
    bool subjectMayResize = false;
    Rectangle<int> preferredABBounds;
    // TooltipWindow listens globally, including while a test view is hidden.
    // One manager serves every view in this application window.
    TooltipWindow tooltips { this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainDocumentWindow)
};
