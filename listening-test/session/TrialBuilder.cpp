// Copyright(C) 2026 Netflix, Inc.
#include "TrialBuilder.h"
#include "../configuration/TestSpecificationRepository.h"

using namespace juce;

namespace {
Trial* appendTrial(TestPlan& plan, const File& directory, const Array<File>& files,
                   int reference, const TrialShuffle& shuffle, const File& video = {}) {
    TrialDefinition definition;
    definition.name = directory.getFileName();
    definition.videoFile = video;
    for (int i = 0; i < files.size(); ++i) {
        definition.sources.push_back({ i, files[i], files[i].getFileNameWithoutExtension(),
                                      methodologyFor(plan.configuration.type).requiresReference && i == reference });
        definition.presentationOrder.add(i);
    }
    if (plan.configuration.type != TEST_TYPE_MUSHRA_DEMO)
        shuffle(definition.presentationOrder);
    return plan.trials.add(new Trial(std::move(definition), plan.configuration.type));
}
}

Result buildTrialPlan(const File& settingsFile, TestPlan& destination, const TrialShuffle& shuffle) {
    TestSpecification spec;
    const auto result = TestSpecificationRepository::read(settingsFile, spec);
    return result.failed() ? result : buildTrialPlan(spec, destination, shuffle);
}

Result buildTrialPlan(const TestSpecification& spec, TestPlan& destination, const TrialShuffle& shuffle) {
    TestMediaInventory inventory;
    const auto result = inspectTestMedia(spec, inventory);
    if (result.failed()) return result;
    TestPlan plan;
    auto& config = plan.configuration;
    config.type = spec.type;
    config.stimuliDirectory = spec.stimuliDirectory;
    config.trialsPerSession = spec.trialsPerSession;
    config.stimuliCount = inventory.stimuliCount;
    for (const auto& folder : inventory.folders) {
        const auto& files = folder.audio;
        const auto& directory = folder.directory;
        const int reference = folder.reference;
        if (config.isPairwise()) {
            for (int first = 0; first < files.size(); ++first)
                for (int second = first + 1; second < files.size(); ++second)
                    appendTrial(plan, directory, {files[first], files[second]}, -1, shuffle, folder.video);
        } else if (config.type == TEST_TYPE_BS1116) {
            for (int i = 0; i < files.size(); ++i)
                if (i != reference)
                    appendTrial(plan, directory, {files[reference], files[i]}, 0, shuffle);
        } else {
            auto ordered = files;
            ordered.remove(reference);
            ordered.add(files[reference]);
            appendTrial(plan, directory, ordered, ordered.size() - 1, shuffle);
        }
    }

    if (config.type != TEST_TYPE_MUSHRA_DEMO) {
        Array<int> order;
        for (int i = 0; i < plan.trials.size(); ++i)
            order.add(i);
        shuffle(order);
        OwnedArray<Trial> randomized;
        for (const auto index : order)
            randomized.add(plan.trials[index]);
        plan.trials.clear(false);
        plan.trials.swapWith(randomized);
    }
    destination = std::move(plan);
    return Result::ok();
}
