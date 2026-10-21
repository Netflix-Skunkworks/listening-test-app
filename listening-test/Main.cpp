//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#include "MainComponent.h"


class ListeningTestApplication : public JUCEApplication {
public:
    ListeningTestApplication() {}
    ~ListeningTestApplication() {}

    void initialise(const String & /*commandLine*/) override {
        mainDocumentWindow.reset(new MainDocumentWindow(getApplicationName() + " v" + ProjectInfo::versionString));
    }

    void shutdown() override { mainDocumentWindow = nullptr; }

    const String getApplicationName() override { return ProjectInfo::projectName; }

    const String getApplicationVersion() override { return ProjectInfo::versionString; }

    bool moreThanOneInstanceAllowed() override { return false; }

    void systemRequestedQuit() override { quit(); }

    void anotherInstanceStarted(const String & /*commandLine*/) override {}

    ApplicationCommandManager &getGlobalCommandManager() { return commandManager; }

private:
    class MainDocumentWindow : public DocumentWindow, private AsyncUpdater {
    public:
        MainDocumentWindow(const String &name)
                : DocumentWindow(name,
                                 Desktop::getInstance().getDefaultLookAndFeel()
                                         .findColour(ResizableWindow::backgroundColourId),
                                 DocumentWindow::closeButton | DocumentWindow::minimiseButton) {
            // Use JUCE's title bar so dragging can be disabled on both platforms.
            setUsingNativeTitleBar(false);
            setDraggable(false);
            setResizable(false, false);
            setContentOwned(new MainComponent(), true);
            keepMaximised();
            setVisible(true);
            triggerAsyncUpdate(); // Native frame dimensions may change when first shown.
        }

        ~MainDocumentWindow() {
            clearContentComponent();
        }

        void closeButtonPressed() override {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

        void moved() override {
            DocumentWindow::moved();
            triggerAsyncUpdate();
        }

        void resized() override {
            DocumentWindow::resized();
            triggerAsyncUpdate();
        }

        void parentSizeChanged() override {
            DocumentWindow::parentSizeChanged();
            triggerAsyncUpdate();
        }

        void minimisationStateChanged(bool isNowMinimised) override {
            DocumentWindow::minimisationStateChanged(isNowMinimised);
            if (!isNowMinimised)
                triggerAsyncUpdate();
        }

    private:
        void handleAsyncUpdate() override { keepMaximised(); }

        void keepMaximised() {
            if (isMinimised())
                return;

            // Fill the available desktop, including any window borders.
            // Reapply after moves, resizes, or display/work-area changes.
            if (const auto* display = Desktop::getInstance().getDisplays().getDisplayForRect(getBounds())) {
                auto bounds = display->userArea;
                if (auto* peer = getPeer()) {
                    const auto frame = peer->getFrameSizeIfPresent();
                    if (frame)
                        frame->subtractFrom(bounds);
                }
                if (getBounds() != bounds)
                    setBounds(bounds);
            }
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainDocumentWindow)
    };
    
    std::unique_ptr <MainDocumentWindow> mainDocumentWindow;
    ApplicationCommandManager commandManager;

};

ApplicationCommandManager &getGlobalCommandManager() {
    return dynamic_cast<ListeningTestApplication *> (JUCEApplication::getInstance())->getGlobalCommandManager();
}


//==============================================================================
START_JUCE_APPLICATION (ListeningTestApplication)
