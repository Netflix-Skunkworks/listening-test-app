// Copyright(C) 2026 Netflix, Inc.
#include "session/ResultsStore.h"
#include "session/TrialBuilder.h"
#include "media/TrialMediaLoader.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>
#include <set>
#include <limits>

using namespace juce;

namespace {
struct Fixture {
    File root = File::getSpecialLocation(File::tempDirectory)
            .getNonexistentChildFile("listening-test-session", {}, false);
    File stimuli = root.getChildFile("stimuli");
    File fallback = root.getChildFile("fallback");

    Fixture() { stimuli.createDirectory(); }
    ~Fixture() { root.deleteRecursively(); }

    File folder(const String& name, StringArray names) {
        const auto directory = stimuli.getChildFile(name);
        directory.createDirectory();
        for (const auto& file : names)
            directory.getChildFile(file).replaceWithText("fixture");
        return directory;
    }

    File settings(testEnum type, String limit = "all") {
        XmlElement xml("testSettings");
        xml.setAttribute("testType", getTestTypeName(type));
        xml.setAttribute("stimuliDirectory", stimuli.getFullPathName());
        xml.setAttribute("trialsPerSession", limit);
        const auto file = root.getChildFile("settings.xml");
        xml.writeTo(file);
        return file;
    }

    bool wav(const File& file, int channels, int samples, float value) {
        AudioBuffer<float> data(channels, samples);
        for (int channel = 0; channel < channels; ++channel)
            for (int sample = 0; sample < samples; ++sample)
                data.setSample(channel, sample, value * (channel + 1));
        WavAudioFormat format;
        auto stream = file.createOutputStream();
        if (!stream)
            return false;
        std::unique_ptr<AudioFormatWriter> writer(
                format.createWriterFor(stream.release(), 48000, channels, 16, {}, 0));
        return writer && writer->writeFromAudioSampleBuffer(data, 0, samples);
    }
};

// Reverse both stimulus and trial order so mapping tests cannot accidentally
// pass because storage order and presentation order happen to match.
void reverseOrder(Array<int>& order) {
    for (int i = 0; i < order.size() / 2; ++i)
        order.swap(i, order.size() - 1 - i);
}

class SessionTests final : public UnitTest {
public:
    SessionTests() : UnitTest("Session services", "session") {}

    bool ok(const Result& result) {
        expect(result.wasOk(), result.getErrorMessage());
        return result.wasOk();
    }

    void runTest() override {
        methodologyRules();
        identityMapping();
        precisionAndResume();
        progression();
        construction();
        inferredCounts();
        roundTrip();
        demoRoundTrip();
        legacyFiles();
        writeFailures();
        mediaLoading();
    }

    TrialDefinition definitionFor(bool reference = true) {
        TrialDefinition definition;
        definition.name = "trial";
        definition.sources = {{0, File("/tmp/reference.wav"), "reference", reference},
                              {1, File("/tmp/coded.wav"), "coded", false}};
        definition.presentationOrder = {1, 0};
        return definition;
    }

    void methodologyRules() {
        beginTest("MUSHRA requires a top score; matching scores request confirmation; strict permits only one top score");
        for (const auto type : {TEST_TYPE_MUSHRA, TEST_TYPE_MUSHRA_STRICT}) {
            Trial trial(definitionFor(), type);
            const auto initial = trial.validateCompletion();
            expect(initial.status.wasOk() == (type == TEST_TYPE_MUSHRA));
            expect(initial.needsConfirmation == (type == TEST_TYPE_MUSHRA));
            trial.getPresentedResponse(0)->score = 60;
            auto validation = trial.validateCompletion();
            expect(validation.status.wasOk());
            expect(!validation.needsConfirmation);
            trial.getPresentedResponse(1)->score = 60;
            expect(trial.validateCompletion().status.failed());
            trial.getPresentedResponse(1)->score = 100;
            for (const auto invalid : {-1.0, 101.0, std::numeric_limits<double>::quiet_NaN()}) {
                trial.getPresentedResponse(0)->score = invalid;
                expect(trial.validateCompletion().status.failed());
            }
        }

        beginTest("BS.1116 accepts fractional impairment scores and exactly one imperceptible rating");
        Trial bs(definitionFor(), TEST_TYPE_BS1116);
        expect(bs.validateCompletion().status.failed()); // Both default to 5.
        bs.getPresentedResponse(0)->score = 3.7;
        expect(bs.validateCompletion().status.wasOk());
        bs.getPresentedResponse(1)->score = 4.9;
        expect(bs.validateCompletion().status.failed());
        bs.getPresentedResponse(1)->score = 5;
        bs.getPresentedResponse(0)->score = 0.9;
        expect(bs.validateCompletion().status.failed());
        const auto bsScale = methodologyFor(TEST_TYPE_BS1116);
        expectWithinAbsoluteError(bsScale.minimum, 1.0, 0.00001);
        expectWithinAbsoluteError(bsScale.maximum, 5.0, 0.00001);
        expectWithinAbsoluteError(bsScale.interval, 0.1, 0.00001);

        beginTest("A/B and video A/B require one explicit preference and no reference");
        for (const auto type : {TEST_TYPE_AB, TEST_TYPE_AVAB}) {
            Trial pair(definitionFor(false), type);
            expectEquals(pair.getReferenceSource(), noSource);
            expect(pair.validateCompletion().status.failed());
            pair.getPresentedResponse(0)->score = 1;
            expect(pair.validateCompletion().status.wasOk());
            pair.getPresentedResponse(1)->score = 1;
            expect(pair.validateCompletion().status.failed());
            pair.getPresentedResponse(0)->score = 0;
            expect(pair.validateCompletion().status.wasOk());
            pair.getPresentedResponse(0)->score = 0.5;
            expect(pair.validateCompletion().status.failed());
        }

        beginTest("Demo accepts ratings without a hidden top score and reference methods require a reference");
        Trial demo(definitionFor(), TEST_TYPE_MUSHRA_DEMO);
        expectEquals(demo.getPresentedStimulusCount(), 1);
        expect(demo.findResponse(demo.getReferenceSource()) == nullptr);
        demo.getPresentedResponse(0)->score = 21;
        expect(demo.validateCompletion().status.wasOk());
        expect(!demo.validateCompletion().needsConfirmation);
        for (const auto type : {TEST_TYPE_MUSHRA, TEST_TYPE_MUSHRA_STRICT, TEST_TYPE_MUSHRA_DEMO, TEST_TYPE_BS1116}) {
            Trial missingReference(definitionFor(false), type);
            expect(missingReference.validateCompletion().status.failed());
        }
    }

    void identityMapping() {
        beginTest("Source identities retain their responses for every presentation permutation, including demo reference positions");
        for (const auto type : {TEST_TYPE_MUSHRA, TEST_TYPE_MUSHRA_DEMO}) {
            auto definition = definitionFor();
            definition.sources.push_back({2, File("/tmp/anchor.wav"), "anchor", false});
            for (const auto& order : {Array<int>{0, 1, 2}, Array<int>{0, 2, 1}, Array<int>{1, 0, 2},
                                      Array<int>{1, 2, 0}, Array<int>{2, 0, 1}, Array<int>{2, 1, 0}}) {
                definition.presentationOrder = order;
                Trial trial(definition, type);
                trial.findResponse(1)->score = 73;
                trial.findResponse(1)->comment = "coded comment";
                trial.findResponse(1)->plays = 7;
                trial.findResponse(1)->sliderMoved = true;
                trial.findResponse(2)->score = 19;
                int presented = 0;
                for (const auto id : order) {
                    if (type == TEST_TYPE_MUSHRA_DEMO && id == 0)
                        continue;
                    const auto* source = trial.getPresentedSource(presented);
                    const auto* response = trial.getPresentedResponse(presented++);
                    expectEquals(source->id, id);
                    expectEquals(response->source, id);
                    if (id == 1) {
                        expectEquals(source->displayName, String("coded"));
                        expectEquals(response->score, 73.0);
                        expectEquals(response->comment, String("coded comment"));
                        expectEquals(response->plays, 7);
                        expect(response->sliderMoved);
                    } else if (id == 2) {
                        expectEquals(response->score, 19.0);
                    }
                }
                expectEquals(trial.getPresentedStimulusCount(), presented);
                expect(trial.getPresentedSource(-1) == nullptr);
                expect(trial.getPresentedResponse(presented) == nullptr);
                expect(trial.findSource(99) == nullptr);
                expect(trial.findResponse(noSource) == nullptr);
            }
        }
    }

    void precisionAndResume() {
        beginTest("Storage rounds fractional scores to three decimals and preserves historical resume defaults");
        Fixture f;
        f.folder("trial", {"coded.wav", "reference.wav"});
        TestPlan plan;
        if (!ok(buildTrialPlan(f.settings(TEST_TYPE_BS1116), plan, reverseOrder)))
            return;
        TestSession session(std::move(plan), "listener", "precision");
        auto* trial = session.getCurrentTrial();
        trial->getPresentedResponse(0)->score = 3.7126;
        trial->getPresentedResponse(0)->sliderMoved = true;
        trial->getPresentedResponse(0)->plays = 8;
        ResultsStore store(f.fallback);
        File saved;
        if (!ok(store.save(session, saved)))
            return;
        auto xml = parseXML(saved);
        auto* file = xml->getChildByName("trials")->getFirstChildElement()->getFirstChildElement();
        expectWithinAbsoluteError(file->getDoubleAttribute("score"), 3.713, 0.0000001);
        expect(!file->hasAttribute("comment"));
        std::unique_ptr<TestSession> resumed;
        if (!ok(store.load(saved, resumed)))
            return;
        const auto* response = resumed->getCurrentTrial()->getPresentedResponse(0);
        expectWithinAbsoluteError(response->score, 3.713, 0.0000001);
        expectEquals(response->plays, 8);
        expect(response->comment.isEmpty());
        expect(!response->sliderMoved); // Not present in historical XML for the current trial.
        expectEquals(trial->getPresentedResponse(0)->score, 3.7126); // Serialization leaves the model alone.
        const auto completed = session.prepareAdvance(1);
        if (!ok(store.save(session, saved, &completed)) || !ok(store.load(saved, resumed)))
            return;
        for (const auto& completedResponse : resumed->getCurrentTrial()->getResponses().stimuli)
            expect(completedResponse.sliderMoved);
    }

    void progression() {
        beginTest("Session boundaries save the next unplayed trial; resume starts a new allowance");
        Fixture f;
        for (int i = 0; i < 3; ++i)
            f.folder("trial" + String(i), {"reference.wav", "coded.wav"});
        TestPlan plan;
        if (!ok(buildTrialPlan(f.settings(TEST_TYPE_MUSHRA, "1"), plan, reverseOrder)))
            return;
        TestSession session(std::move(plan), "listener", "evaluation");
        session.beginCurrentTrial();
        session.selectStimulus(0);
        const auto boundary = session.prepareAdvance(1);
        expect(boundary.outcome == TestSession::AdvanceOutcome::sessionLimitReached);
        expectEquals(boundary.progress.trialIndex, 1);
        expectEquals(session.getProgress().trialIndex, 0); // prepare must not mutate
        expectEquals(session.getSelectedStimulus(), 0);
        expect(session.prepareAdvance(-1).outcome == TestSession::AdvanceOutcome::invalidRequest);
        expect(session.prepareAdvance(20).outcome == TestSession::AdvanceOutcome::invalidRequest);
        ResultsStore store(f.fallback);
        File checkpoint;
        if (!ok(store.save(session, checkpoint, &boundary)))
            return;
        expectEquals(session.getProgress().trialIndex, 0); // write must not commit either
        std::unique_ptr<TestSession> resumed;
        if (!ok(store.load(checkpoint, resumed)))
            return;
        expectEquals(resumed->getProgress().trialIndex, 1);
        expectEquals(resumed->getProgress().completedThisSession, 0);
        resumed->beginCurrentTrial();
        expectEquals(resumed->getSelectedStimulus(), resumed->getCurrentTrial()->getReferenceSource());
        const auto backward = resumed->prepareAdvance(0);
        expect(backward.outcome == TestSession::AdvanceOutcome::trialReady);
        expectEquals(backward.progress.completedThisSession, 0);
        const auto next = resumed->prepareAdvance(2);
        resumed->commitAdvance(next);
        expect(next.outcome == TestSession::AdvanceOutcome::sessionLimitReached);
        const auto finished = resumed->prepareAdvance(3);
        expect(finished.outcome == TestSession::AdvanceOutcome::testComplete);
        expect(ok(store.save(*resumed, checkpoint, &finished)));
        resumed->commitAdvance(finished);
        expect(resumed->getProgress().complete);
        expect(resumed->prepareAdvance(0).outcome == TestSession::AdvanceOutcome::invalidRequest);

        beginTest("Reference defaults use source indices after shuffling; A/B starts unselected");
        TestPlan referencePlan;
        if (!ok(buildTrialPlan(f.settings(TEST_TYPE_MUSHRA), referencePlan, reverseOrder)))
            return;
        TestSession reference(std::move(referencePlan), "listener", "evaluation");
        reference.beginCurrentTrial();
        expectEquals(reference.getSelectedStimulus(), reference.getCurrentTrial()->getReferenceSource());
        reference.commitAdvance(reference.prepareAdvance(1));
        expectEquals(reference.getSelectedStimulus(), reference.getCurrentTrial()->getReferenceSource());
        for (const auto type : {TEST_TYPE_AB, TEST_TYPE_AVAB}) {
            for (int i = 0; i < 3; ++i)
                f.stimuli.getChildFile("trial" + String(i)).getChildFile("video.mp4").replaceWithText("video");
            TestPlan pairPlan;
            if (!ok(buildTrialPlan(f.settings(type), pairPlan, reverseOrder)))
                return;
            TestSession pair(std::move(pairPlan), "listener", "evaluation");
            pair.beginCurrentTrial();
            expectEquals(pair.getSelectedStimulus(), -1);
            pair.selectStimulus(1);
            pair.commitAdvance(pair.prepareAdvance(1));
            expectEquals(pair.getSelectedStimulus(), -1);
        }
    }

    void construction() {
        beginTest("A/B creates every unique pair per folder and retains its associated video");
        Fixture f;
        f.folder("first", {"a.wav", "b.wav", "c.wav", "video.mp4"});
        f.folder("second", {"a.wav", "b.wav", "c.wav", "video.mp4"});
        TestPlan plan;
        if (!ok(buildTrialPlan(f.settings(TEST_TYPE_AVAB), plan, reverseOrder)))
            return;
        expectEquals(plan.trials.size(), 6);
        std::set<String> pairs;
        for (const auto* trial : plan.trials) {
            expectEquals(trial->getDefinition().presentationOrder[0], 1);
            expectEquals(trial->getDefinition().presentationOrder[1], 0);
            expectEquals(trial->getPresentedStimulusCount(), 2);
            expect(trial->getDefinition().videoFile == f.stimuli.getChildFile(trial->getDefinition().name).getChildFile("video.mp4"));
            pairs.insert(trial->getDefinition().name + ":" + trial->findSource(0)->file.getFileName()
                    + ":" + trial->findSource(1)->file.getFileName());
        }
        expectEquals(static_cast<int>(pairs.size()), 6);
        expectEquals(plan.trials[0]->getDefinition().name, String("second"));

        beginTest("BS.1116 pairs each coded stimulus with the reference; failed rebuild preserves the plan");
        Fixture bs;
        const auto folder = bs.folder("trial", {"a.wav", "reference.wav", "b.wav"});
        if (!ok(buildTrialPlan(bs.settings(TEST_TYPE_BS1116), plan, reverseOrder)))
            return;
        expectEquals(plan.trials.size(), 2);
        for (const auto* trial : plan.trials) {
            expect(trial->findSource(trial->getReferenceSource())->file.getFileName() == "reference.wav");
            expect(trial->getPresentedResponse(0)->score == BS1116_DEFAULT_SCORE);
            expect(trial->getPresentedResponse(1)->score == BS1116_DEFAULT_SCORE);
        }
        auto* original = plan.trials[0];
        folder.getChildFile("reference.wav").moveFileTo(folder.getChildFile("missing.wav"));
        expect(buildTrialPlan(bs.settings(TEST_TYPE_BS1116), plan).failed());
        expect(plan.trials[0] == original);
        expectEquals(plan.trials.size(), 2);
    }

    void inferredCounts() {
        beginTest("Specifications infer counts without stimuliCount and ignore stale legacy values");
        Fixture f;
        const auto first = f.folder("first", {"a.wav", "b.wav", "c.wav"});
        const auto second = f.folder("second", {"a.wav", "b.wav", "c.wav"});
        const auto settingsFile = f.settings(TEST_TYPE_AB);
        auto spec = parseXML(settingsFile);
        expect(spec != nullptr && !spec->hasAttribute("stimuliCount"));
        if (spec == nullptr) return;
        TestPlan plan;
        if (!ok(buildTrialPlan(settingsFile, plan, reverseOrder))) return;
        expectEquals(plan.configuration.stimuliCount, 3);
        expectEquals(plan.trials.size(), 6); // Two folders, three unique pairs each.
        spec->setAttribute("stimuliCount", 999);
        expect(spec->writeTo(settingsFile));
        if (!ok(buildTrialPlan(settingsFile, plan, reverseOrder))) return;
        expectEquals(plan.configuration.stimuliCount, 3);
        expectEquals(plan.trials.size(), 6);

        beginTest("Reloading recounts WAVs and trial folders rather than using a cached spec count");
        expect(first.getChildFile("d.wav").replaceWithText("fixture"));
        expect(second.getChildFile("d.wav").replaceWithText("fixture"));
        if (!ok(buildTrialPlan(settingsFile, plan, reverseOrder))) return;
        expectEquals(plan.configuration.stimuliCount, 4);
        expectEquals(plan.trials.size(), 12); // Four stimuli yield six pairs per folder.
        f.folder("third", {"a.wav", "b.wav", "c.wav", "d.wav"});
        if (!ok(buildTrialPlan(settingsFile, plan, reverseOrder))) return;
        expectEquals(plan.trials.size(), 18);

        beginTest("Invalid folder counts fail without replacing the active plan");
        auto* original = plan.trials[0];
        expect(second.getChildFile("d.wav").deleteFile());
        expect(buildTrialPlan(settingsFile, plan, reverseOrder).failed());
        expect(plan.trials[0] == original);
        expectEquals(plan.trials.size(), 18);
        Fixture empty;
        expect(buildTrialPlan(empty.settings(TEST_TYPE_AB), plan, reverseOrder).failed());
        empty.folder("one", {"a.wav"});
        expect(buildTrialPlan(empty.settings(TEST_TYPE_AB), plan, reverseOrder).failed());
        expect(plan.trials[0] == original);
    }

    void roundTrip() {
        beginTest("XML roundtrip preserves shuffled file/score/comment/play mappings and fractional scores");
        Fixture f;
        f.folder("trial", {"coded.wav", "reference.wav"});
        TestPlan plan;
        if (!ok(buildTrialPlan(f.settings(TEST_TYPE_BS1116), plan, reverseOrder)))
            return;
        TestSession session(std::move(plan), "listener", "evaluation");
        auto* trial = session.getCurrentTrial();
        trial->getPresentedResponse(0)->score = 3.7;
        trial->getPresentedResponse(1)->score = 5.0;
        trial->getPresentedResponse(0)->comment = "distinct comment <&> \"quoted\"";
        trial->getPresentedResponse(1)->comment = "reference comment";
        trial->getPresentedResponse(0)->plays = 7;
        trial->getPresentedResponse(1)->plays = 11;
        trial->getResponses().referencePlays = 4;
        trial->setStartTime(Time(100000));
        trial->setStopTime(Time(112000));
        XmlElement survey("surveyResults");
        survey.createNewChildElement("answer")->setAttribute("value", "preserved");
        session.setSurveyResults(std::make_unique<XmlElement>(survey));
        ResultsStore store(f.fallback);
        File checkpoint;
        if (!ok(store.save(session, checkpoint)))
            return;
        const auto xml = parseXML(checkpoint);
        const auto* first = xml->getChildByName("trials")->getFirstChildElement()->getFirstChildElement();
        expectEquals(first->getStringAttribute("fileName"), String("coded.wav"));
        expectWithinAbsoluteError(first->getDoubleAttribute("score"), 3.7, 0.00001);
        std::unique_ptr<TestSession> resumed;
        if (!ok(store.load(checkpoint, resumed)))
            return;
        const auto* restored = resumed->getCurrentTrial();
        for (int i = 0; i < 2; ++i) {
            expect(restored->getPresentedSource(i)->file == trial->getPresentedSource(i)->file);
            expectWithinAbsoluteError(restored->getPresentedResponse(i)->score, trial->getPresentedResponse(i)->score, 0.00001);
            expectEquals(restored->getPresentedResponse(i)->comment, trial->getPresentedResponse(i)->comment);
            expectEquals(restored->getPresentedResponse(i)->plays, trial->getPresentedResponse(i)->plays);
        }
        expectEquals(restored->getResponses().referencePlays, 4);
        expectWithinAbsoluteError((restored->getStopTime() - restored->getStartTime()).inSeconds(), 12.0, 0.001);
        expect(resumed->getSurveyResults()->isEquivalentTo(&survey, true));
        expect(resumed->getStartTime() == session.getStartTime());
        resumed->beginCurrentTrial();
        expectEquals(resumed->getSelectedStimulus(), restored->getReferenceSource());
        auto* original = resumed.get();
        f.root.getChildFile("invalid.xml").replaceWithText("<MUSHRA><info/></MUSHRA>");
        expect(store.load(f.root.getChildFile("invalid.xml"), resumed).failed());
        expect(resumed.get() == original);
    }

    void demoRoundTrip() {
        beginTest("Demo's unrated reference serializes safely and maps correctly even when it is in the middle");
        Fixture f;
        f.folder("trial", {"a.wav", "b.wav", "reference.wav"});
        TestPlan plan;
        if (!ok(buildTrialPlan(f.settings(TEST_TYPE_MUSHRA_DEMO), plan, reverseOrder)))
            return;
        auto definition = plan.trials[0]->getDefinition();
        definition.presentationOrder = {0, 2, 1};
        plan.trials.set(0, new Trial(std::move(definition), TEST_TYPE_MUSHRA_DEMO));
        TestSession session(std::move(plan), "listener", "demo");
        auto* trial = session.getCurrentTrial();
        expectEquals(trial->getPresentedStimulusCount(), 2);
        trial->getPresentedResponse(0)->score = 21.0;
        trial->getPresentedResponse(1)->score = 82.0;
        trial->getPresentedResponse(1)->comment = "demo comment";
        trial->getPresentedResponse(1)->plays = 6;
        trial->getResponses().referencePlays = 3;
        expect(trial->findResponse(trial->getReferenceSource()) == nullptr);

        ResultsStore store(f.fallback);
        File checkpoint;
        if (!ok(store.save(session, checkpoint)))
            return;
        std::unique_ptr<TestSession> resumed;
        if (!ok(store.load(checkpoint, resumed)))
            return;
        expectEquals(resumed->getCurrentTrial()->getReferenceSource(), 1);
        expect(resumed->getCurrentTrial()->findResponse(1) == nullptr);
        expectEquals(resumed->getCurrentTrial()->getPresentedResponse(1)->comment, String("demo comment"));
        expectEquals(resumed->getCurrentTrial()->getPresentedResponse(1)->plays, 6);
        const auto xml = parseXML(checkpoint);
        const auto* reference = xml->getChildByName("trials")->getFirstChildElement()->getChildElement(1);
        expectEquals(reference->getStringAttribute("fileName"), String("reference.wav"));
        expectEquals(reference->getDoubleAttribute("score"), MUSHRA_DEFAULT_SCORE);
        expectEquals(reference->getIntAttribute("plays"), 3);
        expectEquals(resumed->getCurrentTrial()->getPresentedStimulusCount(), 2);
        expectEquals(resumed->getStimulusSourceIndex(0), 0);
        expectEquals(resumed->getStimulusSourceIndex(1), 2);
        expect(resumed->getCurrentTrial()->getPresentedResponse(0)->score == 21.0);
        expect(resumed->getCurrentTrial()->getPresentedResponse(1)->score == 82.0);
        resumed->beginCurrentTrial();
        expectEquals(resumed->getSelectedStimulus(), 1);
        if (!ok(store.save(*resumed, checkpoint)))
            return;
        std::unique_ptr<TestSession> twice;
        if (ok(store.load(checkpoint, twice))) {
            expectEquals(twice->getStimulusSourceIndex(1), 2);
            expect(twice->getCurrentTrial()->getPresentedResponse(1)->score == 82.0);
        }
    }

    void legacyFiles() {
        beginTest("Existing published example XML remains readable without trialsPerSession");
        for (const auto path : {
                "analysis/resources/mushra_example_results/mushra_example_1.xml",
                "analysis/resources/bs1116_example_results/bs1116_example_1.xml",
                "analysis/resources/ab_example_results/ab_example_1.xml"}) {
            Fixture f;
            auto xml = parseXML(File(TEST_REPO_ROOT).getChildFile(path));
            expect(xml != nullptr);
            if (!xml)
                continue;
            xml->getChildByName("info")->setAttribute("stimuliDirectory", f.stimuli.getFullPathName());
            int trialCount = 0;
            for (const auto* trial : xml->getChildByName("trials")->getChildIterator()) {
                if (!trial->hasTagName("trial"))
                    continue;
                ++trialCount;
                const auto folder = f.folder(trial->getStringAttribute("trialName"), {});
                for (const auto* media : trial->getChildIterator())
                    folder.getChildFile(media->getStringAttribute("fileName")).replaceWithText("fixture");
            }
            const auto file = f.root.getChildFile("legacy.xml");
            expect(xml->writeTo(file));
            std::unique_ptr<TestSession> loaded;
            if (!ok(ResultsStore(f.fallback).load(file, loaded)))
                continue;
            expect(loaded->getProgress().complete);
            expectEquals(loaded->getConfiguration().trialsPerSession, -1);
            expectEquals(loaded->getPlan().trials.size(), trialCount);
            File saved;
            if (!ok(ResultsStore(f.fallback).save(*loaded, saved)))
                continue;
            // Export actual serializer output for the independent Python consumer.
            const File contractDirectory(TEST_CONTRACT_ROOT);
            expect(contractDirectory.createDirectory().wasOk());
            expect(saved.copyFileTo(contractDirectory.getChildFile(
                File(TEST_REPO_ROOT).getChildFile(path).getFileName())));
            const auto written = parseXML(saved);
            const auto* writtenTrial = written->getChildByName("trials")->getFirstChildElement();
            for (const auto* originalTrial : xml->getChildByName("trials")->getChildIterator()) {
                if (!originalTrial->hasTagName("trial"))
                    continue; // The published BS.1116 fixture contains a stray text node.
                expect(writtenTrial != nullptr);
                if (!writtenTrial)
                    break;
                expectEquals(writtenTrial->getStringAttribute("trialName"), originalTrial->getStringAttribute("trialName"));
                expectEquals(writtenTrial->getIntAttribute("referencePlays"), originalTrial->getIntAttribute("referencePlays"));
                expectEquals(writtenTrial->getNumChildElements(), originalTrial->getNumChildElements());
                const auto* writtenFile = writtenTrial->getFirstChildElement();
                for (const auto* originalFile : originalTrial->getChildIterator()) {
                    expect(writtenFile != nullptr);
                    if (!writtenFile)
                        break;
                    expectEquals(writtenFile->getStringAttribute("fileName"), originalFile->getStringAttribute("fileName"));
                    if (originalFile->hasTagName("testFile")) {
                        expectWithinAbsoluteError(writtenFile->getDoubleAttribute("score"), originalFile->getDoubleAttribute("score"), 0.00051);
                        expectEquals(writtenFile->getIntAttribute("plays"), originalFile->getIntAttribute("plays"));
                        expectEquals(writtenFile->getStringAttribute("comment"), originalFile->getStringAttribute("comment"));
                    }
                    writtenFile = writtenFile->getNextElement();
                }
                writtenTrial = writtenTrial->getNextElement();
            }
            expect(writtenTrial == nullptr);
        }
    }

    void writeFailures() {
        beginTest("Write failure uses fallback; failed completion retains checkpoint until a final file is saved");
        Fixture f;
        f.folder("trial", {"coded.wav", "reference.wav"});
        TestPlan plan;
        if (!ok(buildTrialPlan(f.settings(TEST_TYPE_MUSHRA), plan)))
            return;
        TestSession session(std::move(plan), "listener", "evaluation");
        ResultsStore store(f.fallback);
        File saved;
        if (!ok(store.save(session, saved)))
            return;
        const auto originalCheckpoint = saved.loadFileAsString();
        const auto backup = f.root.getChildFile("original-stimuli");
        expect(f.stimuli.moveFileTo(backup));
        // A regular file in place of the primary directory deterministically
        // prevents a save, including when the tests run as an administrator.
        expect(f.stimuli.replaceWithText("blocked"));
        if (!ok(store.save(session, saved)))
            return;
        expect(saved.getParentDirectory() == f.fallback);
        const auto fallbackCheckpoint = saved;
        const auto fallbackText = saved.loadFileAsString();
        const auto completion = session.prepareAdvance(1);
        const auto blockedFallback = f.root.getChildFile("blocked-fallback");
        expect(blockedFallback.replaceWithText("blocked"));
        const auto before = saved;
        expect(ResultsStore(blockedFallback).save(session, saved, &completion).failed());
        expect(saved == before);
        expectEquals(backup.getChildFile("temp_listener.xml").loadFileAsString(), originalCheckpoint);
        expectEquals(fallbackCheckpoint.loadFileAsString(), fallbackText);
        expect(!session.getProgress().complete);
        expect(f.stimuli.deleteFile());
        expect(backup.moveFileTo(f.stimuli));
        if (!ok(store.save(session, saved, &completion)))
            return;
        expect(saved.existsAsFile());
        expect(!f.stimuli.getChildFile("temp_listener.xml").exists());
        expect(!fallbackCheckpoint.exists());
        auto finalXml = parseXML(saved);
        expectEquals(finalXml->getChildByName("info")->getStringAttribute("testStatus"), String("complete"));
    }

    void mediaLoading() {
        beginTest("WAV loading preserves source order/channels and truncates all stimuli to the shortest");
        Fixture f;
        const auto folder = f.folder("trial", {"video.mp4"});
        expect(f.wav(folder.getChildFile("a.wav"), 2, 96, 0.1f));
        expect(f.wav(folder.getChildFile("b.wav"), 2, 48, 0.3f));
        TrialDefinition definition;
        definition.name = "trial";
        definition.sources = {{0, folder.getChildFile("a.wav"), "a", false},
                              {1, folder.getChildFile("b.wav"), "b", false}};
        definition.presentationOrder = {1, 0}; // Decode by source, not randomized UI order.
        definition.videoFile = folder.getChildFile("video.mp4");
        Trial trial(definition, TEST_TYPE_AB);
        LoadedTrialMedia media;
        if (!ok(loadTrialMedia(trial, 2, media)))
            return;
        expectEquals(media.channels, 2);
        expectEquals(media.samples, 48);
        expectEquals(media.audio.size(), 2);
        expect(media.video == trial.getDefinition().videoFile);
        for (int source = 0; source < 2; ++source) {
            expectEquals(media.audio[source]->getNumSamples(), 48);
            for (int channel = 0; channel < 2; ++channel)
                expectWithinAbsoluteError(media.audio[source]->getSample(channel, 47),
                        (source == 0 ? 0.1f : 0.3f) * (channel + 1), 0.0001f);
        }

        beginTest("Channel errors, invalid files, and cancellation after decoding leave active media intact");
        const auto* originalBuffer = media.audio[0];
        auto unchanged = [&] {
            expect(media.audio[0] == originalBuffer);
            expectEquals(media.audio.size(), 2);
            expectEquals(media.samples, 48);
            expect(media.video == trial.getDefinition().videoFile);
            expectWithinAbsoluteError(media.audio[1]->getSample(1, 47), 0.6f, 0.0001f);
        };
        expect(loadTrialMedia(trial, 1, media).failed());
        unchanged();
        expect(loadTrialMedia(trial, 2, media, [](double progress) { return progress < 1.0; }).failed());
        unchanged();
        expect(f.wav(folder.getChildFile("mono.wav"), 1, 48, 0.2f));
        definition.sources[1].file = folder.getChildFile("mono.wav");
        Trial mono(definition, TEST_TYPE_AB);
        expect(loadTrialMedia(mono, 2, media).failed());
        unchanged();
        definition.sources[1].file = folder.getChildFile("invalid.wav");
        Trial invalid(definition, TEST_TYPE_AB);
        folder.getChildFile("invalid.wav").replaceWithText("not a wav");
        expect(loadTrialMedia(invalid, 2, media).failed());
        unchanged();
    }
};

class Runner final : public UnitTestRunner {
    void logMessage(const String& message) override { std::cout << message << std::endl; }
};
}

int main() {
    SessionTests tests;
    Runner runner;
    runner.setAssertOnFailure(false);
    runner.runTestsInCategory("session", 12345);
    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult(i)->failures;
    return failures == 0 ? 0 : 1;
}
