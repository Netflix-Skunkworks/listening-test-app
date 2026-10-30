// Listening Test Application
// Copyright(C) 2017-2026 Netflix, Inc.
#include "MainDocumentWindow.h"

namespace {
#if JUCE_MAC
constexpr bool titleButtonsOnLeft = true;
#else
constexpr bool titleButtonsOnLeft = false;
#endif
}

MainDocumentWindow::MainDocumentWindow(const String& name, Component* content, bool addToDesktop)
    : DocumentWindow(name, Desktop::getInstance().getDefaultLookAndFeel()
                               .findColour(ResizableWindow::backgroundColourId),
                     closeButton | minimiseButton, addToDesktop) {
    setUsingNativeTitleBar(false);
    setDraggable(false);
    setResizable(false, false);
    setContentOwned(content, true);
    keepMaximised();
    setVisible(true);
    triggerAsyncUpdate();
}

MainDocumentWindow::~MainDocumentWindow() {
    cancelPendingUpdate();
    clearContentComponent();
}

void MainDocumentWindow::setTestType(testEnum type) {
    const bool resizable = type == TEST_TYPE_AB || type == TEST_TYPE_AVAB;
    if (subjectMayResize == resizable) return;

    // Change the policy before window operations enqueue move/resize updates.
    // A queued update from the maximised mode must not undo an A/B resize.
    subjectMayResize = resizable;
    if (isFullScreen()) setFullScreen(false);
    if (!resizable) preferredABBounds = getBounds();

    setDraggable(resizable);
    setResizable(resizable, resizable);
    setTitleBarButtonsRequired(closeButton | minimiseButton | (resizable ? maximiseButton : 0), titleButtonsOnLeft);
    if (resizable) {
        // Keep the transport and test controls usable at the smallest size.
        setResizeLimits(800, 600, 32767, 32767);
        auto bounds = preferredABBounds.isEmpty()
            ? getBounds().withSizeKeepingCentre(900, 600) : preferredABBounds;
        if (const auto* display = Desktop::getInstance().getDisplays().getDisplayForRect(bounds))
            bounds = bounds.constrainedWithin(display->userArea);
        setBoundsConstrained(bounds);
    } else {
        setResizeLimits(1, 1, 32767, 32767);
        keepMaximised();
    }
}

void MainDocumentWindow::closeButtonPressed() {
    JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainDocumentWindow::moved() {
    DocumentWindow::moved();
    triggerAsyncUpdate();
}

void MainDocumentWindow::resized() {
    DocumentWindow::resized();
    triggerAsyncUpdate();
}

void MainDocumentWindow::parentSizeChanged() {
    DocumentWindow::parentSizeChanged();
    triggerAsyncUpdate();
}

void MainDocumentWindow::minimisationStateChanged(bool minimised) {
    DocumentWindow::minimisationStateChanged(minimised);
    if (!minimised) triggerAsyncUpdate();
}

void MainDocumentWindow::handleAsyncUpdate() { keepMaximised(); }

void MainDocumentWindow::keepMaximised() {
    if (subjectMayResize || isMinimised()) return;
    if (const auto* display = Desktop::getInstance().getDisplays().getDisplayForRect(getBounds())) {
        auto bounds = display->userArea;
        if (auto* peer = getPeer()) {
            const auto frame = peer->getFrameSizeIfPresent();
            if (frame) frame->subtractFrom(bounds);
        }
        if (getBounds() != bounds) setBounds(bounds);
    }
}
