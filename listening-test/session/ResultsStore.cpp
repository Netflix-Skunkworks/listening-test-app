// Copyright(C) 2026 Netflix, Inc.
#include "ResultsStore.h"
#include <cmath>

using namespace juce;

namespace {
Result readTrial(const XmlElement& xml, const TestConfiguration& config, std::unique_ptr<Trial>& destination) {
    TrialDefinition definition;
    definition.name = xml.getStringAttribute("trialName");
    const auto directory = config.stimuliDirectory.getChildFile(definition.name);
    const auto seconds = xml.getDoubleAttribute("trialSeconds", -1);
    const auto referencePlays = xml.getIntAttribute("referencePlays", -1);
    if (definition.name.isEmpty() || !directory.isDirectory() || seconds < 0
            || !std::isfinite(seconds) || referencePlays < 0)
        return Result::fail("Invalid trial metadata for " + definition.name);

    std::vector<StimulusResponse> storedResponses;
    bool hasReference = false;
    for (auto* child : xml.getChildIterator()) {
        if (child->isTextElement())
            continue;
        if (child->hasTagName("videoFile")) {
            definition.videoFile = directory.getChildFile(child->getStringAttribute("fileName"));
            continue;
        }
        if (!child->hasTagName("testFile"))
            return Result::fail("Unknown element in trial " + definition.name);
        const auto file = directory.getChildFile(child->getStringAttribute("fileName"));
        const auto score = child->getDoubleAttribute("score", -1);
        const auto plays = child->getIntAttribute("plays", -1);
        if (!file.existsAsFile() || score < 0 || !std::isfinite(score) || plays < 0)
            return Result::fail("Invalid or missing stimulus " + file.getFullPathName());
        const auto id = static_cast<SourceId>(definition.sources.size());
        const bool reference = methodologyFor(config.type).requiresReference
                && file.getFileName().containsIgnoreCase("REFERENCE");
        if (reference && hasReference)
            return Result::fail("More than one reference in trial " + definition.name);
        hasReference = hasReference || reference;
        definition.sources.push_back({ id, file, file.getFileNameWithoutExtension(), reference });
        definition.presentationOrder.add(id); // Historical XML records presentation order.
        storedResponses.push_back({ id, score, child->getStringAttribute("comment"), plays, false });
    }
    if (definition.sources.size() < 2 || (methodologyFor(config.type).requiresReference && !hasReference))
        return Result::fail("Missing stimuli or reference in trial " + definition.name);
    if (config.isPairwise() && definition.sources.size() != 2)
        return Result::fail("A/B trials must contain exactly two stimuli.");

    auto trial = std::make_unique<Trial>(std::move(definition), config.type);
    trial->setStartTime(Time(0));
    trial->setStopTime(Time(0) + RelativeTime(seconds));
    trial->getResponses().referencePlays = referencePlays;
    for (const auto& stored : storedResponses)
        if (auto* response = trial->findResponse(stored.source))
            *response = stored; // Demo's visible reference deliberately has no response.
    destination = std::move(trial);
    return Result::ok();
}

std::unique_ptr<XmlElement> writeTrial(const Trial& trial) {
    auto xml = std::make_unique<XmlElement>("trial");
    const auto& definition = trial.getDefinition();
    xml->setAttribute("trialName", definition.name);
    xml->setAttribute("trialSeconds", std::round((trial.getStopTime() - trial.getStartTime()).inSeconds()));
    xml->setAttribute("referencePlays", trial.getResponses().referencePlays);
    for (const auto id : definition.presentationOrder) {
        const auto* source = trial.findSource(id);
        const auto* response = trial.findResponse(id);
        auto* file = xml->createNewChildElement("testFile");
        file->setAttribute("fileName", source->file.getFileName());
        // Preserve the legacy XML entry for demo's unrated visible reference.
        file->setAttribute("plays", response ? response->plays : trial.getResponses().referencePlays);
        file->setAttribute("score", response ? std::round(response->score * 1000.0) / 1000.0 : MUSHRA_DEFAULT_SCORE);
        if (response && response->comment.isNotEmpty())
            file->setAttribute("comment", response->comment);
    }
    if (definition.videoFile.existsAsFile())
        xml->createNewChildElement("videoFile")->setAttribute("fileName", definition.videoFile.getFileName());
    return xml;
}

Result replaceResults(const XmlElement& xml, const File& destination) {
    const auto directoryResult = destination.getParentDirectory().createDirectory();
    if (directoryResult.failed())
        return directoryResult;
    TemporaryFile temporary(destination);
    if (!xml.writeTo(temporary.getFile()) || !temporary.overwriteTargetFileWithTemporary())
        return Result::fail("Unable to write results to " + destination.getFullPathName());
    return Result::ok();
}
}

Result ResultsStore::load(const File& resultsFile, std::unique_ptr<TestSession>& destination) const {
    const auto xml = parseXML(resultsFile);
    if (xml == nullptr)
        return Result::fail("Could not parse XML in " + resultsFile.getFullPathName());
    TestPlan plan;
    auto& config = plan.configuration;
    config.type = getTestTypeEnum(xml->getTagName());
    if (!isKnownTestType(config.type))
        return Result::fail("Unknown test type '" + xml->getTagName() + "' in " + resultsFile.getFileName());
    const auto* info = xml->getChildByName("info");
    const auto* trials = xml->getChildByName("trials");
    if (info == nullptr || trials == nullptr || trials->getNumChildElements() < 1)
        return Result::fail("Missing test info or trials in " + resultsFile.getFileName());
    const auto subject = info->getStringAttribute("subjectName");
    const auto test = info->getStringAttribute("testName");
    const auto directory = info->getStringAttribute("stimuliDirectory");
    if (subject.isEmpty() || test.isEmpty() || directory.isEmpty())
        return Result::fail("Missing subject, test name, or stimuli directory in " + resultsFile.getFileName());
    config.stimuliDirectory = File(directory);
    const auto limit = info->getStringAttribute("trialsPerSession", "all");
    config.trialsPerSession = limit.equalsIgnoreCase("all") ? -1 : limit.getIntValue();
    if (config.trialsPerSession != -1 && config.trialsPerSession < 1)
        return Result::fail("Invalid trials per session in " + resultsFile.getFileName());

    for (auto* trialXml : trials->getChildIterator()) {
        if (trialXml->isTextElement())
            continue;
        if (!trialXml->hasTagName("trial"))
            return Result::fail("Unknown element in trials in " + resultsFile.getFileName());
        std::unique_ptr<Trial> trial;
        const auto result = readTrial(*trialXml, config, trial);
        if (result.failed())
            return result;
        if (config.stimuliCount != 0 && static_cast<int>(trial->getDefinition().sources.size()) != config.stimuliCount)
            return Result::fail("Inconsistent stimulus counts in " + resultsFile.getFileName());
        config.stimuliCount = static_cast<int>(trial->getDefinition().sources.size());
        plan.trials.add(trial.release());
    }

    auto session = std::make_unique<TestSession>(std::move(plan), subject, test);
    session->progress.complete = info->getStringAttribute("testStatus") == "complete";
    session->progress.trialIndex = session->progress.complete ? session->plan.trials.size() - 1
            : info->getIntAttribute("currentTrial", -1);
    if (session->getCurrentTrial() == nullptr)
        return Result::fail("Invalid current trial in " + resultsFile.getFileName());
    const auto start = Time::fromISO8601(info->getStringAttribute("startTime"));
    if (start.toMilliseconds() != 0)
        session->startTime = start;
    const auto* survey = xml->getChildByName("surveyResults");
    if (!survey) survey = xml->getChildByName("surveySkipped");
    if (survey) session->setSurveyResults(std::make_unique<XmlElement>(*survey));

    auto time = session->startTime;
    for (int i = 0; i < session->plan.trials.size(); ++i) {
        auto* trial = session->getTrial(i);
        const auto elapsed = trial->getStopTime() - trial->getStartTime();
        trial->setStartTime(time);
        time += elapsed;
        trial->setStopTime(time);
        // Legacy results did not store slider interaction state. Completed trials
        // count as touched; the current trial retains the historical default.
        for (auto& response : trial->getResponses().stimuli)
            response.sliderMoved = session->progress.complete || i < session->progress.trialIndex;
    }
    destination = std::move(session);
    return Result::ok();
}

Result ResultsStore::save(const TestSession& session, File& savedFile,
                          const TestSession::Transition* pending) const {
    const auto& config = session.getConfiguration();
    const auto& progress = pending == nullptr ? session.getProgress() : pending->progress;
    if (session.getPlan().trials.isEmpty())
        return Result::fail("There is no test session to save.");
    const auto temporaryName = "temp_" + session.getSubjectID() + ".xml";
    const auto filename = progress.complete
            ? (session.getTestID() + "_" + Time::getCurrentTime().formatted("%Y-%m-%d_%H%M%S_")
                    + session.getSubjectID() + ".xml").replaceCharacter(' ', '_')
            : temporaryName;

    XmlElement xml(getTestTypeName(config.type));
    auto* info = xml.createNewChildElement("info");
    info->setAttribute("startTime", session.getStartTime().toISO8601(true));
    info->setAttribute("stopTime", Time::getCurrentTime().toISO8601(true));
    info->setAttribute("subjectName", session.getSubjectID());
    info->setAttribute("testName", session.getTestID());
    info->setAttribute("stimuliDirectory", config.stimuliDirectory.getFullPathName());
    info->setAttribute("trialsPerSession", config.trialsPerSession == -1 ? String("all") : String(config.trialsPerSession));
    info->setAttribute("testStatus", progress.complete ? "complete" : "incomplete");
    if (!progress.complete)
        info->setAttribute("currentTrial", progress.trialIndex);
    if (const auto* survey = session.getSurveyResults())
        xml.addChildElement(new XmlElement(*survey));
    auto* trials = xml.createNewChildElement("trials");
    for (const auto* trial : session.getPlan().trials)
        trials->addChildElement(writeTrial(*trial).release());

    auto destination = config.stimuliDirectory.getChildFile(filename);
    auto result = replaceResults(xml, destination);
    if (result.failed()) {
        destination = fallbackDirectory.getChildFile(filename);
        result = replaceResults(xml, destination);
    }
    if (result.failed())
        return result;
    savedFile = destination;

    if (progress.complete) {
        config.stimuliDirectory.getChildFile(temporaryName).deleteFile();
        fallbackDirectory.getChildFile(temporaryName).deleteFile();
    }
    return Result::ok();
}
