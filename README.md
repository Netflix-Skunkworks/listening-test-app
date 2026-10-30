# Listening Test Application
# User's Guide

## Introduction
The Listening Test Application implements multiple standard audio subjective listening tests, including:

* Subjective assessment of intermediate quality level of coding systems (MUSHRA) (ITU-R BS.1534-1)
* Subjective assessment of small impairments in audio systems (ITU-R BS.1116-3)
* Forced-choice, no-reference AB testing, with and without video

## License
The software is released under GNU General Public License Version 3.  See license.txt for details.

## Building & Development

The app uses JUCE **6.1.6**, pinned by the Git submodule commit and checked against
the tag recorded in `.gitmodules`. `listening-test.jucer` is the application build
authority. CMake builds regression tests only.

Fresh-checkout prerequisites:

- Git and access to the public JUCE repository over HTTPS.
- macOS: Xcode with its command-line tools selected (`xcode-select -p`), macOS
  SDK, and Bash. The deployment target is macOS 12; signing is disabled by default.
- Windows: Visual Studio 2022 or Build Tools with **Desktop development with C++**
  and a Windows 10/11 SDK. `build.bat` finds MSBuild through the developer prompt
  or `vswhere`, then uses the newest installed toolset among `v145` (Build
  Tools 2026), `v143` (VS2022), and `v142` (VS2019). Set `TOOLSET` to override.
- Tests: CMake 3.15+ and a C++ compiler. Python 3.11+ enables the XML consumer
  contract check; it needs only Python's standard library.

Run `./build_macos.sh` on macOS or `build.bat` on Windows. Both initialize the
submodule, verify the licensed JUCE tag, build Projucer, regenerate platform
projects, and build the app. They stop at the first failed command and return a
nonzero exit code. Set `BUILD_CONFIGURATION=Release` for a release build (Debug
is the default). On macOS, set `CODE_SIGNING_ALLOWED=YES` only when signing is
configured locally; on Windows, `MSBUILD` can override the executable path.

Artifacts:

| Platform | Application | Generated project |
| --- | --- | --- |
| macOS | `Builds/MacOSX/build/Debug/Listening Test.app` | `Builds/MacOSX/Listening Test.xcodeproj` |
| Windows x64 | `Builds/VisualStudio2019/x64/Debug/App/Listening Test.exe` | `Builds/VisualStudio2019/Listening Test.sln` |

Substitute `Release` in artifact paths when requested. `Builds/` and
`JuceLibraryCode/` are generated and ignored by Git. The checked-in source is
organized as follows, with matching Projucer groups:

| Directory | Responsibility |
| --- | --- |
| `listening-test/app/` | Startup, window, navigation, and session coordination |
| `listening-test/session/` | Trial models, methodology, progress, and results storage |
| `listening-test/media/` | Audio engine, devices, decoding, and video synchronization |
| `listening-test/configuration/` | Typed specifications, repositories, editable drafts, and surveys |
| `listening-test/ui/` | Test views, reusable controls, surveys, and theme/layout |
| `assets/` | Application icon and binary assets |
| `analysis/` | Separate Python consumer of the results XML contract |

`.github/workflows/build.yml` defines service/contract tests on macOS and Windows,
a macOS app build with GUI regression checks, and a Windows x64 app build, all
against the pinned JUCE checkout. These workflow definitions require Actions
runners to be available on the repository; remote CI and Windows execution have
not been verified locally.

Before building directly in Xcode after pulling changes to `listening-test.jucer`,
regenerate the project with the repository's Projucer:

```sh
JUCE/extras/Projucer/Builds/MacOSX/build/Debug/Projucer.app/Contents/MacOS/Projucer --resave listening-test.jucer
```

Then open `Builds/MacOSX/Listening Test.xcodeproj`. The generated projects are
ignored by Git, so pulling changes alone does not update their source lists.
A stale project can produce undefined-symbol errors for newly added classes.
If Xcode was open during regeneration, close and reopen the project before
building so the workspace reloads the source list. Cleaning the build alone
does not reload an outdated project in the editor.
If Projucer has not been built yet, run `./build_macos.sh` first.

### Session services and regression tests

`TrialDefinition` groups source metadata and presentation order. Listener scores,
comments, play counts, and slider state live in `TrialResponses`, keyed by source
identity. `Trial` maps presentation positions to sources and responses and
validates methodology completion rules; demo's visible reference has no rating.
XML compatibility and score rounding remain in `ResultsStore`.

`TestLauncher` coordinates the UI and audio device. The session services depend
on JUCE's core and audio format modules, and can run without a window or device:

* `buildTrialPlan` constructs randomized trials from typed test specifications.
* `TestSession` owns the trial plan, progress, timing, and selected stimulus.
* `loadTrialMedia` decodes a complete trial into staging buffers.
* `ResultsStore` reads and writes the existing results XML format, replacing files atomically.

A trial change loads its media and saves the next checkpoint before committing
session progress and replacing playback buffers. Failed loads or saves retain
the current trial. The UI restores ratings from the session when navigating.

Run the service tests with the checked-out JUCE submodule and CMake:

```sh
cmake -S tests -B Builds/Tests -DCMAKE_BUILD_TYPE=Debug
cmake --build Builds/Tests --config Debug
ctest --test-dir Builds/Tests -C Debug --output-on-failure
```

These tests exercise session limits and resume, randomized file-to-score mapping,
legacy XML compatibility, write failures, and WAV loading failures. With Python
3.11+ available at CMake configuration, `xml_contract` also consumes XML emitted
by the actual C++ `ResultsStore` into `Builds/Tests/xml-contract/`, comparing it
with the published analysis fixtures. CTest orders this check after session tests.
Application
projects continue to be generated from `listening-test.jucer` with Projucer.

### Audio playback boundaries

`AudioPlayer` connects a device-free `AudioEngine`, an `AudioDeviceAdapter`, and
`VideoPlaybackCoordinator`. The engine owns media, transport state, and
preallocated rendering buffers; it depends only on JUCE core/audio basics.
The adapter owns device selection and callback registration. The video
coordinator owns the component and message-thread timer, reading audio position
through a read-only `AudioEngine` snapshot.

Stop/detach the device callback before replacing media. Engine preparation and
buffer allocation run only while rendering is excluded; the audio callback
never loads files or allocates working buffers. Video operations run on the
message thread. Ownership and thread requirements are documented in the headers.

The default CMake suite also runs `audio_rendering` without GUI initialization or
hardware. It covers crossfades, short loops, EOF/pause behavior, variable and
oversized callbacks, and media replacement. Native video synchronization and
device interaction still require manual checks on macOS and Windows.

Shared appearance and layout constants live in `AppTheme`. Startup in `Main.cpp`
resolves data paths and injects them into the shell
and storage/device services; model headers contain no application-path globals.

### Configuration and survey boundaries

`TestSpecificationRepository` discovers, reads, and writes test specifications;
`TestSpecification` and shared media inspection supply validation to the test
manager and trial builder. The picker uses the same repository and keeps each
name paired with its file. The manager's `TestSpecificationTable` stores editable
drafts independently of cells and dialogs, tracks unsaved changes and original
file identities, and defers deletion until Save Changes. Names must be unique
without regard to case and valid as filenames. File writes are atomic; a failed
multi-file save retains dirty state and may have saved earlier rows.

Each `SurveyComponent` owns its validation and XML serialization. Implement
`validateResponses()`, `getSurveyResults()`, and `clearSurveyResponses()` to add
a survey with any fields. Return a `surveyResults` element containing the answers;
the launcher, session, and results store preserve its entire subtree without
interpreting it. Historical completion encodings and custom attributes/elements
survive resume/save. Clearing either example survey also resets completion.

The basic survey is the default. To use the BSU customization example, include
`ui/BsuSurveyComponent.h` in `app/Main.cpp` (using the relative path
`../ui/BsuSurveyComponent.h`) and construct the shell with:

```cpp
auto* content = new MainComponent(options, std::make_unique<BsuSurveyComponent>());
```

Other survey subclasses use the same constructor argument; no shared answer
model, serializer, or shell changes are needed.

The service suite includes configuration discovery, malformed specifications,
all methodology roundtrips, manager rename/delete/discard behavior, save failures,
and opaque survey XML compatibility. The optional GUI suite checks recycled
cells, picker refreshes, validation/completion/reset for both surveys, and an
injected custom survey flowing through startup, persistence, and resume.

The remaining structural work is recorded in the [architecture plan](docs/architecture-plan.md).

### Test view controls

`StimulusSelector`, `RatingControls`, and `PreferenceControls` own reusable
controls with explicit inputs and callbacks. `BaseTestComponent` owns the Next
button and demo's Previous button, including their layout and navigation limits.
`RatingTestComponent` selects scale and comment settings for MUSHRA, BS.1116, and demo. A/B uses
only preference controls; audio/video A/B reserves separate regions for the
video and those same controls.

After generating the application project with Projucer, enable the optional
control regression suite (requires JUCE GUI initialization and access to desktop
display geometry, but opens no window or audio device):

```sh
cmake -S tests -B Builds/Tests -DCMAKE_BUILD_TYPE=Debug -DBUILD_VIEW_CONTROL_TESTS=ON
cmake --build Builds/Tests --config Debug
ctest --test-dir Builds/Tests -C Debug --output-on-failure
```

It checks reference and number shortcuts, rating keyboard steps, restored
preferences and comments, demo navigation limits, text height, alignment of
stimulus buttons with rating sliders, and per-test window sizing transitions.
The application lifecycle checks in `ApplicationTests.cpp` load temporary WAV
and results fixtures through the actual shell without opening a window or audio
device. They cover setup/shortcut visits, chooser cancellation and load failures,
startup/survey navigation, loop bounds and locking, shared keyboard/button
actions, and late events after completion or a session limit.
Interactive focus, native device/chooser interaction, playback, and video
synchronization still require platform checks.

`MainComponent` owns page and outcome state, assembles views, and performs screen
transitions; `PlaybackControls` owns the transport UI, loop
editing, keyboard actions, and refresh timer. Named callbacks replace view
request flags and change broadcasts. Loading transitions disable interaction
before media staging, and terminal outcomes disable it before showing a message.

## Installation & Use

The app opens on Home, with actions to start or resume a test, manage test
specifications, and configure audio. Each page has an explicit Back or Done
action. Save Changes in Manage Tests saves edits and returns Home; Back can
discard unsaved edits, including pending deletions.

During a test, Audio Setup and Keyboard Shortcuts remain available. Opening
either pauses playback and preserves the current trial, scores, comments,
selection, and loop settings. Return to Test restores the same controls with
playback paused. Exit Test saves the current responses before returning Home;
Return to Test on Home continues the in-memory session, while Resume a Test
opens a saved results file. Completing a session returns Home.

### Installation

#### MacOS

After building the application, should be as easy as copying the ouptut .app package into your /Applications folder and running from finder / terminal.

#### Windows

TBD

### Test Configuration
Setup your audio output device.  Make sure it has at least as many output channels as you intend to use for the test!

Click the **Manage Tests** button.  Click 'Add Test' and enter your test name, the directory of your sound files, and the test type.  Use "Trials Per Session" to break long tests into application-enforced multiple sessions.

- **BS-1116** is a test designed for high-quality audio testing.  It sets up paired blind comparisons of a coded audio file & a reference file.  The grading scale goes from 'imperceptible' to 'Very Annoying'.
- In **MUSHRA**, it is expected that some systems will be easily identified, and the focus of the test is to compare multiple systems against each other.  It sets up a blind comparison of up to nine files, including a hidden reference.  The grading scale goes from "Excellent" to "Bad".
- **MUSHRA_Demo** is identical to MUSHRA, except that items are not randomized and the filenames are revealed when you mouse-over the selection buttons.  It is intended for casual use or demonstration.
- **AB_Forced_Choice** Is intended for use in comparing two audio systems where no reference exists.  Stimuli in each directory will be grouped into pairs and presented to the listener, who simply chooses whichever he/she prefers.
- **AudioVideo_AB_Forced_Choice** is identical to AB_Forced_Choice but includes video playback.  Each stimuli directory must have one video file which will be played along with all audio comparisons for that trial.

Click **Save Changes** after adding your test. The test configuration will be
saved in `~/Documents/ListeningTest` and the app returns Home.

`stimuliCount` is inferred from the WAV files in the trial folders each time a
test loads. It is no longer required or written in test specifications; legacy
values are ignored. Each trial folder must contain at least two WAV files and
the same number of stimuli as the other folders. Trial counts are derived from
the folders and methodology: one trial per folder for MUSHRA, one comparison per
coded stimulus for BS.1116, and every unique pair for A/B.

### Stimuli Directory & file naming format
* All must should be placed in one folder, with different subfolders corresponding to each trial in the test. The name of the subfolder will be displayed to the user during the tests.
* Each stimulus in the trial must be saved as a (multichannel) WAV file.
* For BS.1116 and BS.1534, one of the files must have the word 'reference' included in its filename. All other filenames can be arbitrary.
* Test results are automatically stored in the base Stimuli directory.  In-progress tests are saved with the naming convention temp\_[subject name].xml.  Completed tests are saved with the naming convention [test name]\_[date]\_[time]\_[subject name].xml.

```
    -> Test
     |
     --> Item1
       |
       |-> Item1_reference.wav
       |-> Item1_sys1.wav
       |-> Item1_sys2.wav
       --> ...
     |
     --> Item2
       |
       |-> Item2_reference.wav
       |-> Item2_sys1.wav
       |-> Item2_sys2.wav
       --> ...
     |
     --> ...
     --> temp_subject2.xml
     --> Test_2023-01-01_155914_subject1.xml
```

### Running tests

BS.1116, MUSHRA (including strict and demo), and the startup screen fill the
available desktop area and can be minimized. They return to their maximized size
when restored. When an audio or audio/video A/B test is loaded or resumed, the
window can be moved, resized, and maximized. It starts at 900 × 600 and remembers
the subject's A/B window size and position while the app is open. The minimum
size is 800 × 600 to keep the controls usable. Settings and canceled or failed
loads retain the current window mode. Press **Cmd-Q** on macOS or **Alt-F4** on
Windows to quit.

Click **Start Test** to start a new test.  Consult the relevant test specification for details on different test methodologies.

Progress is saved after each trial is completed (i.e. whenever the 'next' button is pressed).  A subject may exit the application at any time, and click **Load Test** to continue, but locating and opening their temporary test file, which is stored in the base Stimuli directory.

### Analyzing test results
Scripts in the **analysis** folder can be used to analyze test results.
