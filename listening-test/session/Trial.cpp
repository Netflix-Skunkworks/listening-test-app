//
//    Listening Test Application
//    Copyright(C) 2017-2026  Netflix, Inc.

#include "Trial.h"
#include <assert.h>
#include <cmath>
#include <random>

using namespace juce;

// Seed a global C++11 Mersenne Twister random number generator
std::random_device g_seed;
std::mt19937 g_rng(g_seed());

void randomizeArrayOrder(Array<int> &anArray) {
    if (anArray.size() < 2)
        return;
    int ind;
    Array<int> tempArray;
    int N = anArray.size();
    std::uniform_int_distribution<int> dist(0, N - 1);

    while (N-- > 1) // Stop looping when only one element remains
    {
        // Get a random number < (N + 1)
        // Note this is a better way to get a uniform distribiution
        // in a range than just using  rand() % (N + 1) and faster
        // than recreating a new uniform districution object each loop
        do {
            ind = dist(g_rng);
        } while (ind >= (N + 1));

        // Place selected item index in temp array and remove from old array
        tempArray.add(anArray[ind]);
        anArray.remove(ind);
    }

    // Add last element from old array to new array
    tempArray.add(anArray[0]);

    // Clear old array and replace with items now in random order from temp array
    anArray.clear();
    anArray.addArray(tempArray);
}

Methodology methodologyFor(testEnum type) {
    switch (type) {
        case TEST_TYPE_BS1116: return { 1.0, 5.0, 0.1, BS1116_DEFAULT_SCORE, true, true };
        case TEST_TYPE_AB:
        case TEST_TYPE_AVAB: return { 0.0, 1.0, 1.0, AB_DEFAULT_SCORE, false, true };
        case TEST_TYPE_MUSHRA_DEMO: return { 0.0, 100.0, 1.0, MUSHRA_DEFAULT_SCORE, true, false };
        default: return { 0.0, 100.0, 1.0, MUSHRA_DEFAULT_SCORE, true, true };
    }
}

Trial::Trial(TrialDefinition newDefinition, testEnum methodology)
    : definition(std::move(newDefinition)), type(methodology) {
    const auto policy = methodologyFor(type);
    for (const auto& source : definition.sources)
        if (!source.reference || policy.ratesReference)
            responses.stimuli.push_back({ source.id, policy.defaultScore, {}, 0, false });
}

const StimulusSource* Trial::findSource(SourceId id) const {
    for (const auto& source : definition.sources)
        if (source.id == id)
            return &source;
    return nullptr;
}

SourceId Trial::getReferenceSource() const {
    for (const auto& source : definition.sources)
        if (source.reference)
            return source.id;
    return noSource;
}

int Trial::getPresentedStimulusCount() const {
    return static_cast<int>(responses.stimuli.size());
}

const StimulusSource* Trial::getPresentedSource(int index) const {
    if (index < 0)
        return nullptr;
    for (const auto id : definition.presentationOrder)
        if (findResponse(id) != nullptr && index-- == 0)
            return findSource(id);
    return nullptr;
}

const StimulusResponse* Trial::findResponse(SourceId id) const {
    for (const auto& response : responses.stimuli)
        if (response.source == id)
            return &response;
    return nullptr;
}

StimulusResponse* Trial::findResponse(SourceId id) {
    return const_cast<StimulusResponse*>(static_cast<const Trial&>(*this).findResponse(id));
}

const StimulusResponse* Trial::getPresentedResponse(int index) const {
    const auto* source = getPresentedSource(index);
    return source ? findResponse(source->id) : nullptr;
}

StimulusResponse* Trial::getPresentedResponse(int index) {
    return const_cast<StimulusResponse*>(static_cast<const Trial&>(*this).getPresentedResponse(index));
}

CompletionValidation Trial::validateCompletion() const {
    const auto policy = methodologyFor(type);
    if (responses.stimuli.empty())
        return { Result::fail("There are no stimuli to evaluate."), false, {} };
    if (policy.requiresReference && getReferenceSource() == noSource)
        return { Result::fail("This trial requires a reference stimulus."), false, {} };

    int maximumCount = 0;
    bool allMatch = true;
    for (const auto& response : responses.stimuli) {
        if (!std::isfinite(response.score) || response.score < policy.minimum || response.score > policy.maximum)
            return { Result::fail("Please rate all stimuli within the rating scale."), false, {} };
        if (response.score == policy.maximum)
            ++maximumCount;
        allMatch = allMatch && response.score == responses.stimuli.front().score;
    }
    if (type == TEST_TYPE_AB || type == TEST_TYPE_AVAB) {
        if (responses.stimuli.size() != 2 || maximumCount != 1
                || (responses.stimuli[0].score != 0 && responses.stimuli[0].score != 1)
                || (responses.stimuli[1].score != 0 && responses.stimuli[1].score != 1))
            return { Result::fail("Please choose your preference -- A or B."), false, {} };
    } else if (type == TEST_TYPE_BS1116) {
        if (responses.stimuli.size() != 2 || maximumCount != 1)
            return { Result::fail("Cannot proceed.  One stimulus must be rated 5.0 and one must be rated otherwise"), false, {} };
    } else if (type != TEST_TYPE_MUSHRA_DEMO) {
        if (maximumCount == 0)
            return { Result::fail("None of the stimuli were rated 100.  There is a hidden reference; at least one stimulus must be scored 100."), false, {} };
        if (type == TEST_TYPE_MUSHRA_STRICT && maximumCount > 1)
            return { Result::fail("Please rate all stimuli before proceeding.  Only one stimulus can be scored 100."), false, {} };
        if (allMatch)
            return { Result::ok(), true, "All of the responses are the same score!  Are you sure you want to proceed?" };
    }
    return { Result::ok(), false, {} };
}
