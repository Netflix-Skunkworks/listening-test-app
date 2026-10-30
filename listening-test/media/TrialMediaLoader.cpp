// Copyright(C) 2026 Netflix, Inc.
#include "TrialMediaLoader.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <limits>

using namespace juce;

Result loadTrialMedia(const Trial& trial, int availableOutputs,
                      LoadedTrialMedia& destination, std::function<bool(double)> progress) {
    if (trial.getDefinition().sources.empty())
        return Result::fail("No stimuli files were found for trial " + trial.getDefinition().name);
    LoadedTrialMedia media;
    OwnedArray<AudioFormatReader> readers;
    WavAudioFormat wav;
    int64 shortest = std::numeric_limits<int64>::max();

    for (const auto& source : trial.getDefinition().sources) {
        const auto path = source.file.getFullPathName();
        if (progress && !progress(0.0))
            return Result::fail("Loading stimuli was canceled.");
        auto stream = std::make_unique<FileInputStream>(File(path));
        if (stream->failedToOpen())
            return Result::fail("Could not open file " + path);
        auto* reader = wav.createReaderFor(stream.release(), true);
        if (reader == nullptr)
            return Result::fail("Unable to read " + path + " as a WAV file.");
        readers.add(reader);
        if (media.channels == 0)
            media.channels = static_cast<int>(reader->numChannels);
        if (static_cast<int>(reader->numChannels) != media.channels)
            return Result::fail("Inconsistent audio channel counts in trial " + trial.getDefinition().name);
        shortest = jmin(shortest, reader->lengthInSamples);
    }
    if (shortest <= 0 || shortest > std::numeric_limits<int>::max())
        return Result::fail("Unsupported audio length in trial " + trial.getDefinition().name);
    if (media.channels > availableOutputs)
        return Result::fail("The stimuli in " + trial.getDefinition().name + " have " + String(media.channels)
                + " channels, but the selected audio device only has " + String(availableOutputs)
                + " output channel(s) enabled.");

    media.samples = static_cast<int>(shortest);
    for (int i = 0; i < readers.size(); ++i) {
        if (progress && !progress(static_cast<double>(i) / readers.size()))
            return Result::fail("Loading stimuli was canceled.");
        auto* buffer = media.audio.add(new AudioBuffer<float>(media.channels, media.samples));
        if (!readers[i]->read(buffer->getArrayOfWritePointers(), media.channels, 0, media.samples))
            return Result::fail("Unable to decode " + trial.getDefinition().sources[static_cast<size_t>(i)].file.getFullPathName());
    }
    if (progress && !progress(1.0))
        return Result::fail("Loading stimuli was canceled.");
    if (trial.getDefinition().videoFile.existsAsFile())
        media.video = trial.getDefinition().videoFile;
    destination = std::move(media);
    return Result::ok();
}
