// Copyright(C) 2026 Netflix, Inc.
#include "media/AudioEngine.h"
#include <iostream>
#include <limits>
#include <vector>

using namespace juce;

namespace {
class RenderingTests final : public UnitTest {
public:
    RenderingTests() : UnitTest("Device-free audio rendering", "audio") {}

    void load(AudioEngine& engine, int samples = 16) {
        engine.deviceStopped();
        OwnedArray<AudioBuffer<float>> media;
        for (int stimulus = 0; stimulus < 2; ++stimulus) {
            auto* buffer = media.add(new AudioBuffer<float>(2, samples));
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < samples; ++i)
                    buffer->setSample(ch, i, (float) (100 * stimulus + 20 * ch + i));
        }
        engine.replaceMedia(media, samples, 2);
        engine.prepare(48000, 8);
        engine.setPlayLoop(true);
        engine.switchStimulus(0);
        engine.resume();
    }

    std::vector<float> render(AudioEngine& engine, int count) {
        std::vector<float> left((size_t) count + 2, -999.0f);
        std::vector<float> right((size_t) count + 2, -999.0f);
        std::vector<float> extra((size_t) count + 2, -999.0f);
        float* outputs[] = {left.data() + 1, right.data() + 1, extra.data() + 1, nullptr};
        engine.render(outputs, 4, count);
        expectEquals(left.front(), -999.0f);
        expectEquals(left.back(), -999.0f);
        expectEquals(right.front(), -999.0f);
        expectEquals(right.back(), -999.0f);
        for (int i = 1; i <= count; ++i) expectEquals(extra[(size_t) i], 0.0f);
        left.erase(left.begin());
        left.pop_back();
        return left;
    }

    void runTest() override {
        AudioEngine engine;
        beginTest("Unprepared and paused rendering clears all outputs and preserves position");
        for (float sample : render(engine, 8)) expectEquals(sample, 0.0f);
        load(engine);
        engine.pause();
        for (float sample : render(engine, 17)) expectEquals(sample, 0.0f);
        expectEquals(engine.getCurrentSample(), 0);

        beginTest("Variable and oversized callback sizes preserve every sample across repeated loops");
        load(engine);
        engine.setFragmentStartSample(2);
        engine.setFragmentEndSample(5);
        engine.setSample(2);
        int expectedPosition = 2;
        for (int count : {1, 7, 0, 63, 1024, 4099, 5}) {
            for (float sample : render(engine, count)) {
                expectEquals(sample, (float) expectedPosition);
                expectedPosition = expectedPosition == 4 ? 2 : expectedPosition + 1;
            }
            expectEquals(engine.getCurrentSample(), expectedPosition);
        }

        beginTest("Stimulus switches crossfade both channels at the same playback position");
        load(engine);
        render(engine, 4);
        engine.switchStimulus(1);
        float left[4]{}, right[4]{};
        float* outputs[]{left, right};
        engine.render(outputs, 2, 4);
        for (int i = 0; i < 4; ++i) {
            expectWithinAbsoluteError(left[i], (float) (4 + i + 25 * i), 0.0001f);
            expectWithinAbsoluteError(right[i], left[i] + 20.0f, 0.0001f);
        }
        expectEquals(engine.getCurrentStimulus(), 1);
        expectEquals(render(engine, 1)[0], 108.0f);

        beginTest("Crossfades stay aligned through repeated fragment boundaries");
        load(engine);
        engine.setFragmentStartSample(2);
        engine.setFragmentEndSample(5);
        engine.setSample(4);
        render(engine, 1);
        engine.switchStimulus(1);
        const auto fade = render(engine, 7);
        for (int i = 0; i < 7; ++i)
            expectWithinAbsoluteError(fade[(size_t) i], 2.0f + (float) (i % 3) + 100.0f * i / 7.0f, 0.0001f);
        expectEquals(engine.getCurrentSample(), 3);
        engine.deviceStopped();
        for (float sample : render(engine, 8)) expectEquals(sample, 0.0f);
        expectEquals(engine.getCurrentSample(), 3);

        beginTest("Nonlooping EOF clears the tail, pauses, and returns to the fragment start");
        load(engine);
        engine.setPlayLoop(false);
        engine.setFragmentStartSample(3);
        engine.setFragmentEndSample(7);
        engine.setSample(5);
        auto tail = render(engine, 4099);
        expectEquals(tail[0], 5.0f);
        expectEquals(tail[1], 6.0f);
        for (size_t i = 2; i < tail.size(); ++i) expectEquals(tail[i], 0.0f);
        expect(engine.isPaused());
        expectEquals(engine.getCurrentSample(), 3);
        for (float sample : render(engine, 8)) expectEquals(sample, 0.0f);
        engine.resume();
        expectEquals(render(engine, 1)[0], 3.0f);

        beginTest("Clamped and empty fragments, invalid selection, and mismatched source lengths are safe");
        load(engine);
        engine.setFragmentStartSample(-100);
        engine.setFragmentEndSample(100);
        engine.setSample(15);
        auto wrap = render(engine, 3);
        expectEquals(wrap[0], 15.0f);
        expectEquals(wrap[1], 0.0f);
        expectEquals(wrap[2], 1.0f);
        engine.setFragmentStartSample(5);
        engine.setFragmentEndSample(2);
        engine.setSample(5);
        for (float sample : render(engine, 32)) expectEquals(sample, 0.0f);
        engine.switchStimulus(100);
        render(engine, 4);
        for (float sample : render(engine, 4)) expectEquals(sample, 0.0f);
        engine.deviceStopped();
        OwnedArray<AudioBuffer<float>> shortMedia;
        auto* shortBuffer = shortMedia.add(new AudioBuffer<float>(1, 2));
        shortBuffer->clear();
        shortBuffer->setSample(0, 0, 0.5f);
        engine.replaceMedia(shortMedia, 8, 2);
        engine.prepare(48000, 8);
        engine.switchStimulus(0);
        engine.setPlayLoop(false);
        engine.resume();
        auto shortOutput = render(engine, 8);
        expectEquals(shortOutput[0], 0.5f);
        for (size_t i = 1; i < shortOutput.size(); ++i) expectEquals(shortOutput[i], 0.0f);

        beginTest("Detached media replacement resets selection, position, and fragment boundaries");
        load(engine);
        render(engine, 5);
        engine.deviceStopped();
        OwnedArray<AudioBuffer<float>> replacement;
        auto* buffer = replacement.add(new AudioBuffer<float>(1, 3));
        for (int i = 0; i < 3; ++i) buffer->setSample(0, i, 0.75f);
        engine.replaceMedia(replacement, 3, 1);
        expectEquals(replacement.size(), 2); // Old media returns to the caller for disposal off the audio thread.
        expectEquals(engine.getCurrentSample(), 0);
        expectEquals(engine.getCurrentStimulus(), -1);
        expectEquals(engine.getNextStimulus(), -1);
        engine.prepare(44100, 2048);
        engine.switchStimulus(0);
        engine.setPlayLoop(true);
        engine.resume();
        for (float sample : render(engine, 4099)) expectEquals(sample, 0.75f);
        const AudioEngine& position = engine;
        const auto snapshot = position.playbackPosition();
        expectEquals(snapshot.sampleRate, 44100.0);
        expect(snapshot.running && !snapshot.paused);
        expectEquals(snapshot.seconds(), snapshot.sample / 44100.0);
        expect(!engine.setPosition(std::numeric_limits<float>::quiet_NaN()));
        expect(!engine.setPosition(1.0f));
        expect(!engine.setPosition(-0.1f));
        expect(engine.setPosition(0.5f));
        expectEquals(engine.getCurrentSample(), 1);
    }
};
class Runner : public UnitTestRunner {
    void logMessage(const String& message) override { std::cout << message << std::endl; }
};
}

int main() {
    RenderingTests tests;
    Runner runner;
    runner.setAssertOnFailure(false);
    runner.runTestsInCategory("audio", 12345);
    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i) failures += runner.getResult(i)->failures;
    return failures == 0 ? 0 : 1;
}
