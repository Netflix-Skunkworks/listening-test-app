// Listening Test Application
// Copyright(C) 2026 Netflix, Inc.
#include "app/MainComponent.h"
#include "ui/BasicSurveyComponent.h"
#include <stdexcept>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template <typename T> Array<T*> children(Component& parent) {
    Array<T*> found;
    for (auto* child : parent.getChildren())
        if (auto* match = dynamic_cast<T*>(child)) found.add(match);
    return found;
}
TextButton* button(Component& parent, const String& name) {
    for (auto* child : children<TextButton>(parent))
        if (child->getButtonText() == name) return child;
    throw std::runtime_error(("Missing button: " + name).toStdString());
}
void dispatch() { MessageManager::getInstance()->runDispatchLoopUntil(20); }

// Supplies device geometry to the callback without creating a physical device.
struct Device final : AudioIODevice {
    Device() : AudioIODevice("Fixture", "Fixture") {}
    StringArray getOutputChannelNames() override { return { "L", "R" }; }
    StringArray getInputChannelNames() override { return {}; }
    Array<double> getAvailableSampleRates() override { return { 48000 }; }
    Array<int> getAvailableBufferSizes() override { return { 512 }; }
    int getDefaultBufferSize() override { return 512; }
    String open(const BigInteger&, const BigInteger&, double, int) override { return {}; }
    void close() override {}
    bool isOpen() override { return true; }
    void start(AudioIODeviceCallback*) override {}
    void stop() override {}
    bool isPlaying() override { return false; }
    String getLastError() override { return {}; }
    int getCurrentBufferSizeSamples() override { return 512; }
    double getCurrentSampleRate() override { return 48000; }
    int getCurrentBitDepth() override { return 16; }
    BigInteger getActiveOutputChannels() const override { BigInteger bits; bits.setRange(0, 2, true); return bits; }
    BigInteger getActiveInputChannels() const override { return {}; }
    int getOutputLatencyInSamples() override { return 0; }
    int getInputLatencyInSamples() override { return 0; }
};

struct Fixture {
    File root = File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("application-lifecycle", {}, false);
    File stimuli = root.getChildFile("stimuli"), specs = root.getChildFile("specs"), results;
    int loads = 0, notices = 0;
    bool failMedia = false;
    std::function<void()> duringLoad;
    explicit Fixture(const String& limit = "all", testEnum type = TEST_TYPE_AB) {
        check(specs.createDirectory().wasOk(), "Create fixture configuration directory");
        for (const auto* name : { "first", "second" }) {
            const auto folder = stimuli.getChildFile(name);
            check(folder.createDirectory().wasOk(), "Create fixture trial directory");
            for (const auto* file : { "a.wav", type == TEST_TYPE_AB ? "b.wav" : "reference.wav" }) {
                AudioBuffer<float> audio(2, 48000);
                audio.clear();
                WavAudioFormat format;
                auto output = folder.getChildFile(file).createOutputStream();
                check(output != nullptr, "Create WAV stream");
                std::unique_ptr<AudioFormatWriter> writer(format.createWriterFor(output.release(), 48000, 2, 16, {}, 0));
                check(writer && writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples()), "Write fixture WAV");
            }
        }
        XmlElement spec("test");
        spec.setAttribute("name", "Application fixture");
        spec.setAttribute("testType", testTypes[type]);
        spec.setAttribute("stimuliDirectory", stimuli.getFullPathName());
        spec.setAttribute("trialsPerSession", limit);
        check(spec.writeTo(specs.getChildFile("fixture-testspec.xml")), "Write fixture spec");
        TestPlan plan;
        check(buildTrialPlan(specs.getChildFile("fixture-testspec.xml"), plan, [](Array<int>&) {}).wasOk(), "Build fixture plan");
        TestSession session(std::move(plan), "listener", "Application fixture");
        session.beginCurrentTrial();
        check(ResultsStore(specs).save(session, results).wasOk(), "Write resumable fixture results");
    }
    ~Fixture() { root.deleteRecursively(); }
    MainComponent::Options options() {
        MainComponent::Options options;
        options.initialiseDevice = false;
        options.deviceSettings = root.getChildFile("audio-settings.xml");
        options.testsDirectory = specs;
        options.loadMedia = [this](const Trial& trial, LoadedTrialMedia& media) {
            ++loads;
            if (duringLoad) duringLoad();
            return failMedia ? Result::fail("Injected media loading failure") : loadTrialMedia(trial, 2, media);
        };
        options.showMessage = [this](AlertWindow::AlertIconType, const String&, const String&) { ++notices; };
        return options;
    }
};

void transportActions() {
    Device device;
    AudioPlayer player({}, false);
    player.getEngine().setTotalSamples(480000);
    PlaybackControls transport(player);
    int starts = 0;
    transport.onStartDevice = [&] { ++starts; player.getEngine().prepare(device.getCurrentSampleRate(), device.getCurrentBufferSizeSamples()); };
    transport.configure(480000);
    check(!transport.handleKey(KeyPress(KeyPress::spaceKey)), "Inactive transport must decline keyboard actions");
    transport.setActive(true);
    check(player.isPaused(), "Entering a test must leave playback paused");
    auto* position = children<Slider>(transport)[0];
    position->setMinAndMaxValues(20, 40, sendNotificationSync);
    position->setMinValue(39, sendNotificationSync, true);
    check(position->getMinValue() == 35, "Loop edits must preserve the 500ms minimum at the device sample rate");
    position->setMinAndMaxValues(20, 40, sendNotificationSync);
    player.getEngine().setSample(168000);
    button(transport, "Restart")->onClick();
    check(player.getEngine().getCurrentSample() == 96000, "Restart button must seek to the start of the loop");
    player.getEngine().setSample(168000);
    transport.handleKey(KeyPress(KeyPress::homeKey));
    check(player.getEngine().getCurrentSample() == 96000, "Home must use the same restart action as the button");
    transport.handleKey(KeyPress(KeyPress::spaceKey));
    check(!player.isPaused(), "Space must play");
    button(transport, "Pause")->onClick();
    check(player.isPaused(), "Play/pause button must use the same toggle action as Space");
    button(transport, "Fast forward")->onClick();
    check(position->getMinValue() == 40 && position->getMaxValue() == 60, "Forward must move one loop width");
    transport.handleKey(KeyPress(KeyPress::leftKey));
    check(position->getMinValue() == 20 && position->getMaxValue() == 40, "Left arrow must use the previous-loop action");
    auto* lock = children<ToggleButton>(transport)[1];
    lock->setToggleState(true, dontSendNotification);
    lock->onClick();
    position->setMinValue(30, sendNotificationSync, true);
    check(position->getMinValue() == 30 && position->getMaxValue() == 50, "Locked loop edits must retain interval width");
    lock->setToggleState(false, dontSendNotification);
    lock->onClick();
    position->setValue(45, sendNotificationSync);
    transport.handleKey(KeyPress('X'));
    check(position->getMinValue() == 45, "X must set the loop start at the cursor");
    transport.handleKey(KeyPress('X', ModifierKeys::shiftModifier, 0));
    check(position->getMinValue() == 0, "Shift-X must clear the loop start");
    position->setValue(75, sendNotificationSync);
    // setValue is constrained by the loop end. C uses that actual cursor value.
    const auto cursor = position->getValue();
    transport.handleKey(KeyPress('C'));
    check(position->getMaxValue() == cursor, "C must set the loop end at the cursor");
    transport.handleKey(KeyPress('C', ModifierKeys::shiftModifier, 0));
    check(position->getMaxValue() == 100, "Shift-C must clear the loop end");
    position->onDragStart();
    transport.setActive(false);
    transport.setActive(true);
    player.getEngine().setSample(144000);
    MessageManager::getInstance()->runDispatchLoopUntil(140);
    check(position->getValue() == 30, "Returning after an interrupted drag must restore position updates");
    auto delayedToggle = button(transport, "Play")->onClick;
    transport.setActive(false);
    const int priorStarts = starts, priorSample = player.getEngine().getCurrentSample();
    delayedToggle();
    position->onDragStart();
    position->setValue(10, sendNotificationSync);
    position->onDragEnd();
    dispatch();
    check(starts == priorStarts && player.isPaused() && player.getEngine().getCurrentSample() == priorSample,
          "Late button, slider, and drag callbacks must not reactivate an inactive transport");
    player.getEngine().deviceStopped();
}

void setupVisitsAndLoadOutcomes() {
    Fixture fixture;
    MainComponent main(fixture.options());
    main.setVisible(true);
    check(main.resumeTest(fixture.results) == MainComponent::Outcome::loaded, "Resume actual trial media");
    auto* view = children<ABTestComponent>(main)[0];
    auto* preference = children<PreferenceControls>(*view)[0];
    auto* selector = children<StimulusSelector>(*view)[0];
    auto* transport = children<PlaybackControls>(main)[0];
    auto* selected = children<TextButton>(*selector)[0];
    selected->onClick();
    children<Slider>(*preference)[0]->setValue(0, sendNotificationSync);
    children<TextEditor>(*preference)[0]->setText("retained comment", false);
    auto* position = children<Slider>(*transport)[0];
    position->setMinAndMaxValues(20, 60, sendNotificationSync);
    for (const auto* page : { "Audio Setup", "Keyboard Shortcuts" }) {
        button(main, page)->onClick();
        check(!view->isVisible() && !transport->isActive(), "Auxiliary pages must deactivate test and transport together");
        check(!main.keyPressed(KeyPress(KeyPress::spaceKey)), "Auxiliary pages must reject playback shortcuts");
        button(main, "Return to Test")->onClick();
        check(view->isVisible() && transport->isActive(), "Return must restore the existing test view");
        check(preference->preference() == 0 && preference->comment(0) == "retained comment",
              "Setup visits must preserve uncommitted preferences and comments");
        check(selected->getToggleState(), "Setup visits must preserve stimulus selection");
        check(position->getMinValue() == 20 && position->getMaxValue() == 60 && fixture.loads == 1,
              "Setup visits must preserve loop bounds without reloading trial media");
    }
    check(main.resumeTest({}) == MainComponent::Outcome::loadCancelled, "Dismissal must have an explicit cancellation outcome");
    check(main.resumeTest(fixture.root.getChildFile("missing.xml")) == MainComponent::Outcome::loadFailed,
          "Unreadable results must have an explicit failure outcome");
    fixture.failMedia = true;
    check(main.resumeTest(fixture.results) == MainComponent::Outcome::loadFailed, "Staging failure must be explicit");
    fixture.failMedia = false;
    check(children<ABTestComponent>(main)[0] == view && view->isVisible() && preference->comment(0) == "retained comment",
          "Cancellation and failed loads must retain the current trial controls and answers");
    check(selected->getToggleState() && position->getMinValue() == 20 && position->getMaxValue() == 60,
          "Cancellation and failed loads must retain selection and loop bounds");
    int starts = 0;
    const auto start = transport->onStartDevice;
    transport->onStartDevice = [&] { ++starts; start(); };
    fixture.duringLoad = [&] {
        check(main.lastOutcome() == MainComponent::Outcome::loading && !transport->isActive() && !view->isEnabled(),
              "Trial staging must deactivate all test interaction before loading media");
        const int before = starts;
        view->onPlaybackAction(PlaybackAction::play);
        check(starts == before, "A late view callback must not restart devices during media staging");
    };
    fixture.failMedia = true;
    auto* nextTrial = button(*view, "Next >");
    nextTrial->onClick();
    check(main.lastOutcome() == MainComponent::Outcome::loadFailed && view->isVisible() && transport->isActive(),
          "Failed trial navigation must return to the same active test");
    check(selected->getToggleState() && preference->comment(0) == "retained comment" && position->getMinValue() == 20,
          "Failed trial navigation must preserve the current selection, edits, and loop");
    fixture.failMedia = false;
    nextTrial->onClick();
    check(main.lastOutcome() == MainComponent::Outcome::loaded && view->isEnabled() && position->getMinValue() == 0,
          "Retrying trial navigation must load the next trial and reset its loop");
    std::unique_ptr<TestSession> saved;
    check(ResultsStore(fixture.specs).load(fixture.results, saved).wasOk()
          && saved->getProgress().trialIndex == 1
          && saved->getTrial(0)->getPresentedResponse(0)->score == 1
          && saved->getTrial(0)->getPresentedResponse(0)->comment == "retained comment",
          "Trial transition callbacks must persist answers against their original presented sources");
}

void ratingSetupVisits() {
    for (auto type : { TEST_TYPE_MUSHRA, TEST_TYPE_MUSHRA_STRICT, TEST_TYPE_MUSHRA_DEMO, TEST_TYPE_BS1116 }) {
        Fixture fixture("all", type);
        MainComponent main(fixture.options());
        check(main.resumeTest(fixture.results) == MainComponent::Outcome::loaded, "Load rating fixture");
        auto* view = children<RatingTestComponent>(main)[0];
        auto* ratings = children<RatingControls>(*view)[0];
        auto* selector = children<StimulusSelector>(*view)[0];
        TextButton* selected = nullptr;
        for (auto* candidate : children<TextButton>(*selector))
            if (candidate->getButtonText() != "Reference") { selected = candidate; break; }
        check(selected != nullptr, "A rating fixture must have a hidden stimulus");
        selected->onClick();
        const double score = type == TEST_TYPE_BS1116 ? 4.2 : 73;
        ratings->setResponse(0, score, "retained rating comment");
        for (const auto* page : { "Audio Setup", "Keyboard Shortcuts" }) {
            button(main, page)->onClick();
            button(main, "Return to Test")->onClick();
            check(view->isVisible() && selected->getToggleState() && ratings->score(0) == score && fixture.loads == 1,
                  "Setup visits must retain rating values and selection for every rating methodology");
            if (type != TEST_TYPE_MUSHRA_DEMO)
                check(ratings->comment(0) == "retained rating comment", "Setup visits must retain rating comments");
        }
    }
}

void sessionEndGuards(const String& limit, MainComponent::Outcome expected) {
    Fixture fixture(limit);
    MainComponent main(fixture.options());
    check(main.resumeTest(fixture.results) == MainComponent::Outcome::loaded, "Load session-ending fixture");
    auto* view = children<ABTestComponent>(main)[0];
    auto* preference = children<PreferenceControls>(*view)[0];
    auto* nextTrial = button(*view, "Next >");
    auto* transport = children<PlaybackControls>(main)[0];
    check(children<TextButton>(*view).size() == 1, "Normal views must allocate only the Next button");
    int starts = 0;
    const auto start = transport->onStartDevice;
    transport->onStartDevice = [&] { ++starts; start(); };
    children<Slider>(*preference)[0]->setValue(0, sendNotificationSync);
    if (limit != "all") button(*transport, "Play")->triggerClick();
    nextTrial->onClick();
    if (limit == "all") {
        check(main.lastOutcome() == MainComponent::Outcome::loaded, "Advancing a trial must keep the session active");
        check(nextTrial->getButtonText() == "Finish Test", "The last trial must offer Finish Test");
        children<Slider>(*preference)[0]->setValue(1, sendNotificationSync);
        button(*transport, "Play")->triggerClick(); // queued before completion, delivered afterward
        nextTrial->onClick();
    }
    check(main.lastOutcome() == expected && !transport->isActive() && !view->isVisible(),
          "Completion and session limits must deactivate playback before notifying the user");
    const int priorStarts = starts, priorLoads = fixture.loads;
    view->onPlaybackAction(PlaybackAction::play);
    nextTrial->onClick();
    button(*transport, "Play")->onClick();
    dispatch();
    check(starts == priorStarts && fixture.loads == priorLoads && !main.keyPressed(KeyPress(KeyPress::spaceKey)),
          "Late playback and navigation events must not restart a completed or paused session");
    check(!button(main, "Return to Test")->isVisible(), "An ended session must not offer in-memory continuation");
    check(button(main, "Start Test")->isVisible(), "An ended session must return Home");
}

void startupAndSurvey() {
    Fixture fixture;
    MainComponent main(fixture.options());
    main.setVisible(true);
    button(main, "Start Test")->onClick();
    auto* selection = children<NewTestSelectComponent>(main)[0];
    check(selection->isVisible(), "Start Test must open selection");
    children<TextEditor>(*selection)[0]->setText("listener", false);
    button(*selection, "Ready!")->onClick();
    auto* survey = children<BasicSurveyComponent>(main)[0];
    check(survey->isVisible() && !selection->isVisible(), "Selection must explicitly advance to the survey");
    button(main, "Back")->onClick();
    check(selection->isVisible() && children<TextEditor>(*selection)[0]->getText() == "listener",
          "Survey cancellation must preserve participant entry");
    children<ToggleButton>(*selection)[0]->setToggleState(true, dontSendNotification);
    fixture.failMedia = true;
    button(*selection, "Ready!")->onClick();
    check(main.lastOutcome() == MainComponent::Outcome::loadFailed && selection->isVisible(),
          "Startup failure must leave the selection page available for retry");
    fixture.failMedia = false;
    button(*selection, "Ready!")->onClick();
    check(main.lastOutcome() == MainComponent::Outcome::loaded && children<ABTestComponent>(main)[0]->isVisible(),
          "Retrying startup must enter the selected methodology");
}

void demoNavigation() {
    Fixture fixture("all", TEST_TYPE_MUSHRA_DEMO);
    MainComponent main(fixture.options());
    main.setVisible(true);
    check(main.resumeTest(fixture.results) == MainComponent::Outcome::loaded, "Load a real demo session");
    auto* view = children<RatingTestComponent>(main)[0];
    auto* previous = button(*view, "< Prev");
    auto* next = button(*view, "Next >");
    check(!previous->isEnabled() && next->isEnabled(), "Demo must disable Previous at the first trial");
    const int initialLoads = fixture.loads;
    previous->onClick();
    check(fixture.loads == initialLoads, "A late Previous callback at the start must not load another trial");
    next->onClick();
    check(previous->isEnabled() && !next->isEnabled(), "Demo must disable Next at the last trial");
    const int finalLoads = fixture.loads;
    next->onClick();
    check(fixture.loads == finalLoads, "A late Next callback at the end must not advance or end a demo");
    previous->onClick();
    check(!previous->isEnabled() && next->isEnabled() && fixture.loads == finalLoads + 1,
          "Previous must return to the prior demo trial and restore the navigation limits");
    const auto nextBounds = next->getBounds(), previousBounds = previous->getBounds();
    check(!nextBounds.intersects(previousBounds) && nextBounds.getBottom() == view->getHeight(),
          "Demo navigation must stay within the view without overlapping");
    check(view->getComponentAt(nextBounds.getCentre()) == next,
          "Rating controls must not intercept clicks on the navigation buttons");
}

// A survey with fields unknown to the shell and persistence implementation.
class CustomSurvey final : public SurveyComponent {
public:
    StringArray validateResponses() const override { return {}; }
    std::unique_ptr<XmlElement> getSurveyResults() const override {
        auto xml = std::make_unique<XmlElement>("surveyResults");
        xml->setAttribute("schema", "custom-survey");
        xml->createNewChildElement("room")->setAttribute("name", "Studio <A&B>");
        return xml;
    }
    void clearSurveyResponses() override { surveyCompleted = false; }
};

void customSurveyPersistence() {
    Fixture fixture;
    auto survey = std::make_unique<CustomSurvey>();
    auto* custom = survey.get();
    MainComponent main(fixture.options(), std::move(survey));
    main.setVisible(true);
    button(main, "Start Test")->onClick();
    auto* selection = children<NewTestSelectComponent>(main)[0];
    children<TextEditor>(*selection)[0]->setText("custom-listener", false);
    button(*selection, "Ready!")->onClick();
    check(custom->isVisible(), "The shell must show an injected survey through the base contract");
    button(*custom, "Continue")->onClick();
    check(main.lastOutcome() == MainComponent::Outcome::loaded, "A custom survey must launch the selected test");
    Array<File> results;
    fixture.stimuli.findChildFiles(results, File::findFiles, false, "*.xml");
    File checkpoint;
    for (const auto& file : results) {
        auto xml = parseXML(file);
        const auto* answers = xml ? xml->getChildByName("surveyResults") : nullptr;
        if (answers && answers->getStringAttribute("schema") == "custom-survey") checkpoint = file;
    }
    check(checkpoint.existsAsFile(), "Launching must save a checkpoint with custom survey answers");
    auto xml = parseXML(checkpoint);
    auto expected = custom->getSurveyResults();
    check(xml && xml->getChildByName("surveyResults")
          && xml->getChildByName("surveyResults")->isEquivalentTo(expected.get(), true),
          "The shell and results writer must preserve unfamiliar fields and nested answers");
    check(main.resumeTest(checkpoint) == MainComponent::Outcome::loaded, "Custom survey results must resume");
    std::unique_ptr<TestSession> restored;
    check(ResultsStore(fixture.specs).load(checkpoint, restored).wasOk()
          && restored->getSurveyResults()->isEquivalentTo(expected.get(), true),
          "Custom survey results must survive session loading unchanged");
}
}

void runApplicationTests() {
    transportActions();
    setupVisitsAndLoadOutcomes();
    ratingSetupVisits();
    sessionEndGuards("all", MainComponent::Outcome::completed);
    sessionEndGuards("1", MainComponent::Outcome::sessionPaused);
    startupAndSurvey();
    demoNavigation();
    customSurveyPersistence();
}
