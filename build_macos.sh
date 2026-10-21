#!/bin/bash

git submodule init && git submodule update

# Refuse to build against a JUCE the licence does not cover. Git pins a
# submodule by commit rather than by tag, so the tag is named here and in
# .gitmodules, and checked against what is actually checked out.
JUCE_TAG=$(git config --file .gitmodules submodule.JUCE.tag)
if [ "$(git -C JUCE rev-parse HEAD)" != "$(git -C JUCE rev-parse "$JUCE_TAG^{commit}" 2>/dev/null)" ]; then
    echo "error: JUCE submodule is not at tag $JUCE_TAG" >&2
    echo "       run: git -C JUCE fetch --tags && git -C JUCE checkout $JUCE_TAG" >&2
    exit 1
fi

# Projucer's own Xcode project ships with a deployment target older than the
# minimum current Xcode SDKs accept, and it belongs to the JUCE submodule, so it
# is overridden here rather than edited in place. The app project sets its own
# target in listening-test.jucer.
xcodebuild -project JUCE/extras/Projucer/Builds/MacOSX/Projucer.xcodeproj \
    MACOSX_DEPLOYMENT_TARGET=12.0
./JUCE/extras/Projucer/Builds/MacOSX/build/Debug/Projucer.app/Contents/MacOS/Projucer --resave ./listening-test.jucer
xcodebuild -project Builds/MacOSX/Listening\ Test.xcodeproj
