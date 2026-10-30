#!/usr/bin/env bash
# Copyright(C) 2026 Netflix, Inc.
set -euo pipefail
trap 'echo "error: macOS build failed at line $LINENO" >&2' ERR
cd -- "$(dirname -- "$0")"

# The submodule commit and licensed JUCE tag remain the source of truth.
git submodule update --init --recursive
juce_tag=$(git config --file .gitmodules submodule.JUCE.tag)
juce_head=$(git -C JUCE rev-parse HEAD)
juce_tag_commit=$(git -C JUCE rev-parse "$juce_tag^{commit}")
if [[ "$juce_head" != "$juce_tag_commit" ]]; then
    echo "error: JUCE submodule must be at tag $juce_tag" >&2
    exit 1
fi

configuration=${BUILD_CONFIGURATION:-Debug}
case "$configuration" in Debug|Release) ;; *) echo 'error: BUILD_CONFIGURATION must be Debug or Release' >&2; exit 1 ;; esac
xcodebuild -project JUCE/extras/Projucer/Builds/MacOSX/Projucer.xcodeproj \
    -configuration Debug MACOSX_DEPLOYMENT_TARGET=12.0 CODE_SIGNING_ALLOWED=NO build
projucer=JUCE/extras/Projucer/Builds/MacOSX/build/Debug/Projucer.app/Contents/MacOS/Projucer
"$projucer" --resave listening-test.jucer
xcodebuild -project 'Builds/MacOSX/Listening Test.xcodeproj' \
    -configuration "$configuration" CODE_SIGNING_ALLOWED="${CODE_SIGNING_ALLOWED:-NO}" build
echo "Application: $PWD/Builds/MacOSX/build/$configuration/Listening Test.app"
