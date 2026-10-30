// Copyright(C) 2026 Netflix, Inc.
#include "configuration/TestSpecificationTable.h"
#include "session/TrialBuilder.h"
#include "session/ResultsStore.h"
#include <limits>
using namespace juce;

namespace {
struct Fixture {
    File root = File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("configuration-tests", {}, false);
    File specs = root.getChildFile("specs"), stimuli = root.getChildFile("stimuli"), trial = stimuli.getChildFile("trial");
    Fixture() {
        specs.createDirectory();
        trial.createDirectory();
        for (const auto* name : {"reference.wav", "coded.wav", "other.wav", "video.mp4"})
            trial.getChildFile(name).replaceWithText("discovery fixture");
    }
    ~Fixture() { root.deleteRecursively(); }
    TestSpecification spec(String name = "Example", testEnum type = TEST_TYPE_MUSHRA) {
        return {name, type, stimuli, -1};
    }
};

class ConfigurationTests final : public UnitTest {
public:
    ConfigurationTests() : UnitTest("Typed configuration and surveys", "session") {}
    bool ok(const Result& result) { expect(result.wasOk(), result.getErrorMessage()); return result.wasOk(); }
    using Column = TestSpecificationTable::Column;

    void runTest() override {
        specifications();
        discovery();
        tableEdits();
        validationFailures();
        surveyCompatibility();
    }
    void specifications() {
        beginTest("Repository roundtrips every methodology and shares media validation with trial construction");
        Fixture f;
        TestSpecificationRepository repository(f.specs);
        for (int type = 0; type < NUMBER_OF_TEST_TYPES; ++type) {
            const auto spec = f.spec("Example", (testEnum) type);
            File file;
            if (!ok(repository.write(spec, file))) continue;
            TestSpecification restored;
            if (!ok(repository.read(file, restored))) continue;
            expectEquals(restored.name, spec.name);
            expectEquals((int) restored.type, type);
            expect(restored.stimuliDirectory == f.stimuli);
            TestMediaInventory media;
            if (!ok(inspectTestMedia(restored, media))) continue;
            TestPlan plan;
            if (!ok(buildTrialPlan(restored, plan, [](Array<int>&) {}))) continue;
            expectEquals(plan.trials.size(), type == TEST_TYPE_BS1116 ? 2 : type >= TEST_TYPE_AB ? 3 : 1);
            expectEquals(media.trialCount, plan.trials.size());
            expectEquals(plan.configuration.stimuliCount, 3);
            expect(!parseXML(file)->hasAttribute("stimuliCount"));
        }
        beginTest("Historical roots, missing defaults, case-insensitive types and stale counts translate at storage");
        const auto file = f.specs.getChildFile("legacy-testspec.xml");
        XmlElement xml("testSettings");
        xml.setAttribute("testType", "mushra");
        xml.setAttribute("stimuliDirectory", f.stimuli.getFullPathName());
        xml.setAttribute("stimuliCount", "nonsense");
        expect(xml.writeTo(file));
        TestSpecification spec;
        if (!ok(repository.read(file, spec))) return;
        expectEquals(spec.name, String("legacy-testspec"));
        expectEquals(spec.trialsPerSession, -1);
        TestPlan plan;
        expect(buildTrialPlan(spec, plan).wasOk());
        xml.setAttribute("trialsPerSession", "-1");
        expect(xml.writeTo(file));
        expect(repository.read(file, spec).wasOk());
    }
    void discovery() {
        beginTest("Discovery sorts and maps files, ignores other XML, reports malformed entries, and refreshes to empty");
        Fixture f;
        TestSpecificationRepository repository(f.specs);
        File first, last;
        ok(repository.write(f.spec("Z last"), last));
        ok(repository.write(f.spec("A first"), first));
        f.specs.getChildFile("bad-testspec.xml").replaceWithText("<broken");
        f.specs.getChildFile("audioDeviceSettings.xml").replaceWithText("<irrelevant/>");
        std::vector<StoredTestSpecification> specs;
        StringArray errors;
        if (!ok(repository.discover(specs, errors))) return;
        expectEquals((int) specs.size(), 2);
        expectEquals(errors.size(), 1);
        expect(specs[0].file == first && specs[1].file == last);
        expectEquals(specs[0].specification.name, String("A first"));
        first.deleteFile(); last.deleteFile();
        ok(repository.discover(specs, errors));
        expect(specs.empty());
        expectEquals(errors.size(), 1);
    }
    void tableEdits() {
        beginTest("Table edits retain source file identities through renames, deferred deletes, discard, and name swaps");
        Fixture f;
        TestSpecificationRepository repository(f.specs);
        File file;
        if (!ok(repository.write(f.spec("Old"), file))) return;
        const auto original = f.specs.getChildFile("different-filename-testspec.xml");
        expect(file.moveFileTo(original));
        TestSpecificationTable table(f.specs);
        StringArray errors;
        ok(table.reload(errors));
        expect(!table.hasUnsavedChanges());
        table.setText(0, Column::name, "New");
        expect(table.hasUnsavedChanges());
        ok(table.save());
        const auto renamed = repository.fileFor(f.spec("New"));
        expect(renamed.existsAsFile() && !original.exists());
        expect(!table.hasUnsavedChanges());
        table.remove(0);
        expect(renamed.existsAsFile() && table.hasUnsavedChanges());
        ok(table.reload(errors));
        expectEquals(table.size(), 1);
        expect(!table.hasUnsavedChanges());
        table.remove(0);
        ok(table.save());
        expect(!renamed.exists());
        File a, b;
        ok(repository.write(f.spec("A", TEST_TYPE_AB), a));
        ok(repository.write(f.spec("B", TEST_TYPE_BS1116), b));
        ok(table.reload(errors));
        table.setText(0, Column::name, "B");
        table.setText(1, Column::name, "A");
        ok(table.save());
        TestSpecification spec;
        ok(repository.read(a, spec)); expectEquals((int) spec.type, (int) TEST_TYPE_BS1116);
        ok(repository.read(b, spec)); expectEquals((int) spec.type, (int) TEST_TYPE_AB);
    }
    void validationFailures() {
        beginTest("Malformed specifications leave destination intact; failed table validation writes and deletes nothing");
        Fixture f;
        TestSpecificationRepository repository(f.specs);
        File file;
        if (!ok(repository.write(f.spec("Original"), file))) return;
        const auto originalContents = file.loadFileAsString();
        auto malformed = *parseXML(file);
        const auto invalid = f.specs.getChildFile("invalid-testspec.xml");
        TestSpecification destination = f.spec("unchanged");
        for (const auto* limit : {"0", "-2", "2junk", "1.5", "", "2147483648", "18446744073709551617"}) {
            malformed.setAttribute("trialsPerSession", limit);
            expect(malformed.writeTo(invalid));
            expect(repository.read(invalid, destination).failed());
            expectEquals(destination.name, String("unchanged"));
        }
        malformed.setAttribute("trialsPerSession", "all");
        malformed.setAttribute("testType", "Unknown");
        expect(malformed.writeTo(invalid));
        expect(repository.read(invalid, destination).failed());
        malformed.setAttribute("testType", "MUSHRA");
        malformed.setAttribute("stimuliDirectory", "relative/path");
        expect(malformed.writeTo(invalid));
        expect(repository.read(invalid, destination).failed());
        invalid.replaceWithText("<other name=\"not a test\"/>");
        expect(repository.read(invalid, destination).failed());
        invalid.deleteFile();
        TestSpecificationTable table(f.specs);
        StringArray errors;
        ok(table.reload(errors));
        table.setText(0, Column::sessionLimit, "9");
        expect(table.save().failed());
        expect(file.loadFileAsString() == originalContents && table.hasUnsavedChanges());
        table.setText(0, Column::sessionLimit, "all");
        table.setText(0, Column::name, "../escape");
        expect(table.save().failed());
        expect(file.loadFileAsString() == originalContents);
        table.setText(0, Column::name, "Original");
        table.add();
        table.setText(1, Column::name, "original");
        table.setText(1, Column::directory, f.stimuli.getFullPathName());
        expect(table.save().failed());
        expect(file.loadFileAsString() == originalContents);
        table.setText(1, Column::name, "Original");
        table.remove(0);
        table.setText(0, Column::type, "unsupported");
        expect(table.save().failed());
        expect(file.existsAsFile()); // Pending delete waits for a valid batch.
        const auto blocked = f.root.getChildFile("not-a-directory");
        blocked.replaceWithText("occupied");
        File saved = file;
        expect(TestSpecificationRepository(blocked).write(f.spec(), saved).failed());
        expect(saved == file && blocked.loadFileAsString() == "occupied");
    }
    void surveyCompatibility() {
        beginTest("Opaque survey payloads preserve historical encodings, skipped surveys and custom schemas");
        Fixture f;
        TestPlan plan;
        if (!ok(buildTrialPlan(f.spec(), plan))) return;
        TestSession session(std::move(plan), "listener", "survey persistence");
        ResultsStore store(f.specs);
        auto roundtrip = [&](const XmlElement* survey) {
            session.setSurveyResults(survey ? std::make_unique<XmlElement>(*survey) : nullptr);
            File file;
            if (!ok(store.save(session, file))) return;
            std::unique_ptr<TestSession> restored;
            if (!ok(store.load(file, restored))) return;
            if (survey) {
                expect(restored->getSurveyResults() != nullptr);
                if (!restored->getSurveyResults()) return;
                expect(restored->getSurveyResults()->isEquivalentTo(survey, true));
                if (!ok(store.save(*restored, file))) return;
                auto saved = parseXML(file);
                expect(saved->getChildByName(survey->getTagName())->isEquivalentTo(survey, true));
            } else {
                expect(restored->getSurveyResults() == nullptr);
                auto saved = parseXML(file);
                expect(saved->getChildByName("surveyResults") == nullptr);
                expect(saved->getChildByName("surveySkipped") == nullptr);
            }
        };
        roundtrip(nullptr);
        XmlElement skipped("surveySkipped");
        skipped.setAttribute("reason", "custom reason");
        roundtrip(&skipped);
        for (const auto* completion : {"0", "1", "no", "yes", "true", "false", ""}) {
            XmlElement survey("surveyResults");
            survey.setAttribute("surveyCompleted", completion);
            survey.setAttribute("custom", "preserve <&>");
            survey.setAttribute("age", "invalid historical value");
            survey.createNewChildElement("answer")->setAttribute("value", "preserved");
            roundtrip(&survey);
        }
        XmlElement custom("surveyResults");
        custom.setAttribute("schema", "third-party-survey-v2");
        auto* answer = custom.createNewChildElement("listeningEnvironment");
        answer->setAttribute("room", "Studio <A&B>");
        answer->createNewChildElement("notes")->addTextElement("No shared field definitions required");
        roundtrip(&custom);
    }

} configurationTests;
}
