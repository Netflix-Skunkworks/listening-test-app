//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017-2026  Netflix, Inc.

#include "MainComponent.h"
#include "MainDocumentWindow.h"

class ListeningTestApplication : public JUCEApplication {
public:
    ListeningTestApplication() {}
    ~ListeningTestApplication() {}

    void initialise(const String & /*commandLine*/) override {
        const auto testsDirectory = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("ListeningTest");
        MainComponent::Options options;
        options.deviceSettings = testsDirectory.getChildFile("audioDeviceSettings.xml");
        options.testsDirectory = testsDirectory;
        auto* content = new MainComponent(options);
        mainDocumentWindow.reset(new MainDocumentWindow(getApplicationName() + " v" + ProjectInfo::versionString, content));
        content->onTestLoaded = [this](testEnum type) { mainDocumentWindow->setTestType(type); };
    }

    void shutdown() override { mainDocumentWindow = nullptr; }

    const String getApplicationName() override { return ProjectInfo::projectName; }

    const String getApplicationVersion() override { return ProjectInfo::versionString; }

    bool moreThanOneInstanceAllowed() override { return false; }

    void systemRequestedQuit() override { quit(); }

    void anotherInstanceStarted(const String & /*commandLine*/) override {}

private:
    std::unique_ptr <MainDocumentWindow> mainDocumentWindow;

};

//==============================================================================
START_JUCE_APPLICATION (ListeningTestApplication)
