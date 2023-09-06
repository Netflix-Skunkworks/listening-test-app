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
    class MainDocumentWindow : public DocumentWindow {
    public:
        MainDocumentWindow(const String &name)
                : DocumentWindow(name,
                                 Desktop::getInstance().getDefaultLookAndFeel()
                                         .findColour(ResizableWindow::backgroundColourId),
                                 DocumentWindow::allButtons) {
            setContentOwned(new MainComponent(), true);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
            setUsingNativeTitleBar(true);
            setResizable(true, false);
        }

        ~MainDocumentWindow() {
            clearContentComponent();
        }

        void closeButtonPressed() {
            JUCEApplication::getInstance()->systemRequestedQuit();
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
