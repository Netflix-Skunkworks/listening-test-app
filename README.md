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

The listening test application is build on top of JUCE, an open-source audio processing & UI framework.  JUCE is a cross-platform framework, but only MacOS targets have been tested.  Windows build projects under MSVS 2019 are generated, but untested.

Invoking `build_macos.sh` will do the following:
* initialize & update JUCE submodules
* Build the "Projucer" project manager
* Run "Projucer" to generate target platform build projects
* Build the MacOS application using xCode command line tools

## Installation & Use

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

Click **Finish** after adding your test.  The test configuration will be saved in ~/Documents/  (the test app will pop up a window showing you the location).

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

Click **Start Test** to start a new test.  Consult the relevant test specification for details on different test methodologies.

Progress is saved after each trial is completed (i.e. whenever the 'next' button is pressed).  A subject may exit the application at any time, and click **Load Test** to continue, but locating and opening their temporary test file, which is stored in the base Stimuli directory.

### Analyzing test results
Scripts in the **analysis** folder can be used to analyze test results.
