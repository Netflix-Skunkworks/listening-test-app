# Copyright(C) 2026 Netflix, Inc.
cmake_minimum_required(VERSION 3.15)
option(BUILD_VIEW_CONTROL_TESTS "Build GUI control regression tests (requires generated JuceHeader)" OFF)
if(CMAKE_HOST_APPLE AND BUILD_VIEW_CONTROL_TESTS AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    # Match the app's target. JUCE 6 uses APIs unavailable when targeting macOS 15+.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version" FORCE)
endif()
project(ListeningTestSessionTests LANGUAGES C CXX)

# The default suite compiles only services, without a GUI or generated header.
# The app's authoritative build remains listening-test.jucer.
set(JUCE_MODULES_ONLY ON CACHE BOOL "" FORCE)
add_subdirectory(../JUCE JUCE EXCLUDE_FROM_ALL)
add_executable(session_tests
    SessionTests.cpp
    ConfigurationTests.cpp
    ../listening-test/session/TestSession.cpp
    ../listening-test/session/TrialBuilder.cpp
    ../listening-test/configuration/TestSpecification.cpp
    ../listening-test/configuration/TestSpecificationRepository.cpp
    ../listening-test/configuration/TestSpecificationTable.cpp
    ../listening-test/session/ResultsStore.cpp
    ../listening-test/media/TrialMediaLoader.cpp
    ../listening-test/session/Trial.cpp
    ../listening-test/session/TestTypes.cpp)
target_include_directories(session_tests PRIVATE ../listening-test)
target_compile_features(session_tests PRIVATE cxx_std_14)
target_compile_definitions(session_tests PRIVATE
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_FLAC=0
    JUCE_USE_OGGVORBIS=0
    TEST_REPO_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/.."
    TEST_CONTRACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/xml-contract")
target_link_libraries(session_tests PRIVATE
    juce::juce_core juce::juce_audio_basics juce::juce_audio_formats)
add_executable(audio_engine_tests AudioEngineTests.cpp ../listening-test/media/AudioEngine.cpp)
target_include_directories(audio_engine_tests PRIVATE ../listening-test)
target_compile_features(audio_engine_tests PRIVATE cxx_std_14)
target_compile_definitions(audio_engine_tests PRIVATE
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
target_link_libraries(audio_engine_tests PRIVATE juce::juce_audio_basics)
enable_testing()
add_test(NAME audio_rendering COMMAND audio_engine_tests)

add_test(NAME session_services COMMAND session_tests)

# Cross-language contract checks consume XML emitted by the real ResultsStore.
# Python's parser uses only the standard library; no analysis packages required.
find_package(Python3 3.11 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME xml_contract COMMAND ${CMAKE_COMMAND} -E env
        "LISTENING_TEST_XML_CONTRACT_DIR=${CMAKE_CURRENT_BINARY_DIR}/xml-contract"
        ${Python3_EXECUTABLE} -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/../analysis/tests -p test_xml_contract.py)
    set_tests_properties(session_services PROPERTIES FIXTURES_SETUP stored_results)
    set_tests_properties(xml_contract PROPERTIES FIXTURES_REQUIRED stored_results)
endif()

# Small control tests use JUCE's GUI initialisation, but open no window or device.
# The application source list remains exclusively in listening-test.jucer.
if(BUILD_VIEW_CONTROL_TESTS)
    add_executable(view_controls_tests
        ViewControlsTests.cpp
        ApplicationTests.cpp
        ../listening-test/app/MainComponent.cpp
        ../listening-test/ui/AppTheme.cpp
        ../listening-test/ui/PlaybackControls.cpp
        ../listening-test/media/AudioPlayer.cpp
        ../listening-test/media/AudioEngine.cpp
        ../listening-test/media/AudioDeviceAdapter.cpp
        ../listening-test/media/VideoPlaybackCoordinator.cpp
        ../listening-test/app/TestLauncher.cpp
        ../listening-test/session/TestSession.cpp
        ../listening-test/session/ResultsStore.cpp
        ../listening-test/media/TrialMediaLoader.cpp
        ../listening-test/ui/BaseTestComponent.cpp
        ../listening-test/ui/RatingTestComponent.cpp
        ../listening-test/ui/ABTestComponent.cpp
        ../listening-test/ui/NewTestSelectComponent.cpp
        ../listening-test/ui/SurveyComponent.cpp
        ../listening-test/ui/BasicSurveyComponent.cpp
        ../listening-test/ui/BsuSurveyComponent.cpp
        ../listening-test/ui/TestManagerComponent.cpp
        ../listening-test/session/TrialBuilder.cpp
        ../listening-test/configuration/TestSpecification.cpp
        ../listening-test/configuration/TestSpecificationRepository.cpp
        ../listening-test/configuration/TestSpecificationTable.cpp
        ../listening-test/session/Trial.cpp
        ../listening-test/session/TestTypes.cpp
        ../listening-test/ui/TestViewControls.cpp
        ../listening-test/app/MainDocumentWindow.cpp)
    target_include_directories(view_controls_tests PRIVATE ../listening-test ../JuceLibraryCode)
    target_compile_features(view_controls_tests PRIVATE cxx_std_14)
    target_compile_definitions(view_controls_tests PRIVATE
        JUCE_APP_CONFIG_HEADER="${CMAKE_CURRENT_SOURCE_DIR}/../JuceLibraryCode/AppConfig.h"
        JUCE_STANDALONE_APPLICATION=1
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_USE_CURL=0)
    target_link_libraries(view_controls_tests PRIVATE juce::juce_audio_utils juce::juce_video)
    add_test(NAME view_controls COMMAND view_controls_tests)
endif()
