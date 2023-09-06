#!/bin/bash

git submodule init && git submodule update
xcodebuild -project JUCE/extras/Projucer/Builds/MacOSX/Projucer.xcodeproj
./JUCE/extras/Projucer/Builds/MacOSX/build/Debug/Projucer.app/Contents/MacOS/Projucer --resave ./listening-test.jucer
xcodebuild -project Builds/MacOSX/Listening\ Test.xcodeproj
