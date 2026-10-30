// Listening Test Application
// Copyright(C) 2026 Netflix, Inc.
#pragma once

enum class PlaybackAction {
    play, pause, toggle, restart, previousLoop, nextLoop,
    setLoopStart, setLoopEnd, clearLoopStart, clearLoopEnd
};
