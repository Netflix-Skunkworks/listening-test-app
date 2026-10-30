// Copyright(C) 2026 Netflix, Inc.
#include "TestLauncher.h"
#include "../media/AudioPlayer.h"

using namespace juce;

namespace {
// The only loading code that knows about a window. Decoding itself is usable
// without a message loop, an audio device, or the application.
class MediaLoadingTask final : public ThreadWithProgressWindow {
public:
    MediaLoadingTask(const Trial& trial, int outputs)
        : ThreadWithProgressWindow("Loading stimuli into memory", true, false),
          trial(trial), outputs(outputs) {}

    void run() override {
        result = loadTrialMedia(trial, outputs, media, [this](double progress) {
            setProgress(progress);
            return !threadShouldExit();
        });
    }

    LoadedTrialMedia media;
    Result result = Result::fail("Loading stimuli was canceled.");

private:
    const Trial& trial;
    int outputs;
};
}

TestLauncher::TestLauncher(AudioPlayer& player, const File& resultsDirectory, MediaLoader loader)
    : audioPlayer(player), loadMediaOverride(std::move(loader)), resultsStore(resultsDirectory),
      fileLogger(FileLogger::createDefaultAppLogger("ListeningTest", "listening-test.log.txt", {}, 0)) {}

bool TestLauncher::report(const Result& result) {
    lastError = result.getErrorMessage();
    if (result.failed())
        dbgOut(lastError);
    return result.wasOk();
}

void TestLauncher::dbgOut(const String& message) {
    if (fileLogger)
        fileLogger->logMessage(message);
}

Result TestLauncher::loadMedia(const Trial& trial, LoadedTrialMedia& media) {
    if (loadMediaOverride) return loadMediaOverride(trial, media);
    MediaLoadingTask task(trial, audioPlayer.getOutputChannelCount());
    if (!task.runThread())
        return Result::fail("Loading stimuli was canceled.");
    if (task.result.wasOk())
        media = std::move(task.media);
    return task.result;
}

void TestLauncher::installMedia(LoadedTrialMedia& media) {
    inputChannels = media.channels;
    samplesCount = media.samples;
    audioPlayer.replaceTrialMedia(media);
}

bool TestLauncher::activate(std::unique_ptr<TestSession> next, bool saveCheckpoint) {
    const bool wasRunning = audioPlayer.isRunning();
    audioPlayer.stop();
    LoadedTrialMedia media;
    auto result = loadMedia(*next->getCurrentTrial(), media);
    if (result.wasOk()) {
        next->beginCurrentTrial();
        if (saveCheckpoint)
            result = resultsStore.save(*next, lastSavedFile);
    }
    if (result.failed()) {
        if (wasRunning)
            startPlaybackDevice();
        return report(result);
    }
    session = std::move(next);
    installMedia(media);
    startPlaybackDevice();
    return report(Result::ok());
}

bool TestLauncher::init(const File& settingsFile) {
    TestPlan plan;
    const auto result = buildTrialPlan(settingsFile, plan);
    if (result.failed())
        return report(result);
    auto next = std::make_unique<TestSession>(std::move(plan), subjectID, testID);
    next->setSurveyResults(surveyResults ? std::make_unique<XmlElement>(*surveyResults) : nullptr);
    return activate(std::move(next), true);
}

bool TestLauncher::loadResults(const File& resultsFile) {
    std::unique_ptr<TestSession> next;
    const auto result = resultsStore.load(resultsFile, next);
    if (result.failed())
        return report(result);
    if (next->getProgress().complete)
        return report(Result::fail("This test is already complete."));
    return activate(std::move(next), false);
}

bool TestLauncher::saveResults() {
    if (!session)
        return report(Result::fail("There is no test session to save."));
    session->finishCurrentTrial();
    return report(resultsStore.save(*session, lastSavedFile));
}

TestLauncher::AdvanceResult TestLauncher::goToTrial(int index) {
    using Outcome = TestSession::AdvanceOutcome;
    if (!session) {
        const auto result = Result::fail("There is no test session to advance.");
        report(result);
        return { Outcome::invalidRequest, result };
    }
    const auto transition = session->prepareAdvance(index);
    if (transition.outcome == Outcome::invalidRequest) {
        const auto result = Result::fail("Invalid trial index " + String(index));
        report(result);
        return { transition.outcome, result };
    }

    const bool wasRunning = audioPlayer.isRunning();
    audioPlayer.stop();
    session->finishCurrentTrial();
    LoadedTrialMedia media;
    auto result = transition.outcome == Outcome::trialReady
            ? loadMedia(*session->getTrial(index), media) : Result::ok();
    if (result.wasOk())
        result = resultsStore.save(*session, lastSavedFile, &transition);
    if (result.failed()) {
        if (wasRunning)
            startPlaybackDevice();
        report(result);
        return { transition.outcome, result };
    }

    session->commitAdvance(transition);
    if (transition.outcome == Outcome::trialReady) {
        installMedia(media);
        startPlaybackDevice();
    }
    report(Result::ok());
    return { transition.outcome, Result::ok() };
}

void TestLauncher::startPlaybackDevice() {
    audioPlayer.start();
    if (session)
        audioPlayer.switchStimulus(session->getSelectedStimulus());
}

void TestLauncher::selectStimulus(int sourceIndex) {
    if (session) {
        session->selectStimulus(sourceIndex);
        audioPlayer.switchStimulus(session->getSelectedStimulus());
    }
}

String TestLauncher::getCurrentTrialName() const {
    return getCurrentTrial() ? getCurrentTrial()->getDefinition().name : String();
}

String TestLauncher::getSoundFileNameOnly(int index) const {
    const auto* trial = getCurrentTrial();
    const auto* source = trial ? trial->getPresentedSource(index) : nullptr;
    return source ? source->displayName : String();
}

void TestLauncher::incrementPlayCount(int index) {
    if (auto* trial = getCurrentTrial())
        if (auto* response = trial->getPresentedResponse(index))
            ++response->plays;
}

void TestLauncher::incrementReferencePlayCount() {
    if (auto* trial = getCurrentTrial())
        ++trial->getResponses().referencePlays;
}
