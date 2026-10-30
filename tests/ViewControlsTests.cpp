// Listening Test Application
// Copyright(C) 2026 Netflix, Inc.
#include "ui/TestViewControls.h"
#include "app/MainDocumentWindow.h"
#include "ui/TestManagerComponent.h"
#include "ui/BsuSurveyComponent.h"
#include "ui/BasicSurveyComponent.h"
#include <iostream>
#include <stdexcept>
#include "app/MainComponent.h"

void runApplicationTests();

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename T>
Array<T*> children(Component& component) {
    Array<T*> result;
    for (auto* child : component.getChildren())
        if (auto* match = dynamic_cast<T*>(child)) result.add(match);
    return result;
}

void pageNavigation(const File& snapshotDirectory) {
    const auto settings = File::getSpecialLocation(File::tempDirectory)
        .getNonexistentChildFile("navigation-audio-settings", ".xml", false);
    MainComponent::Options options;
    options.initialiseDevice = false;
    options.deviceSettings = settings;
    options.testsDirectory = settings.getParentDirectory();
    MainComponent main(options);
    main.setVisible(true);
    main.setSize(900, 600);
    auto button = [&](const String& text) -> TextButton* {
        for (auto* item : children<TextButton>(main))
            if (item->getButtonText() == text && item->isVisible()) return item;
        return nullptr;
    };
    auto snapshot = [&](const String& name) {
        if (snapshotDirectory == File()) return;
        snapshotDirectory.createDirectory();
        FileOutputStream stream(snapshotDirectory.getChildFile(name));
        stream.setPosition(0);
        stream.truncate();
        PNGImageFormat().writeImageToStream(main.createComponentSnapshot(main.getLocalBounds()), stream);
    };
    check(button("Start Test") && button("Resume a Test") && button("Manage Tests") && button("Audio Setup"),
          "Home must expose the four preparation actions");
    check(!button("Exit Test") && !button("Keyboard Shortcuts"), "Home must hide test-only navigation");
    for (auto* transport : children<TransportButton>(main))
        check(!transport->isVisible(), "Home must hide transport controls");
    check(!main.keyPressed(KeyPress(KeyPress::spaceKey)), "Playback shortcuts must not run on Home");
    snapshot("home.png");
    button("Audio Setup")->triggerClick();
    MessageManager::getInstance()->runDispatchLoopUntil(20);
    check(button("Done") && !button("Audio Setup") && !button("Start Test"),
          "Audio Setup must have an explicit Done action instead of toggle navigation");
    check(children<AudioDeviceSelectorComponent>(main)[0]->isVisible(), "Audio Setup must show the device controls");
    check(!main.keyPressed(KeyPress(KeyPress::spaceKey)), "Playback shortcuts must not consume audio setup input");
    snapshot("audio-setup.png");
    button("Done")->triggerClick();
    MessageManager::getInstance()->runDispatchLoopUntil(20);
    check(button("Audio Setup") && !button("Done"), "Done must return to Home");
    check(!children<AudioDeviceSelectorComponent>(main)[0]->isVisible(), "Returning Home must hide the device controls");
    settings.deleteFile();
}

void testManagerSaveAndDiscard() {
    const auto root = File::getSpecialLocation(File::tempDirectory)
        .getNonexistentChildFile("test-manager-navigation", {}, false);
    const auto specs = root.getChildFile("specs");
    const auto stimuli = root.getChildFile("stimuli");
    const auto trial = stimuli.getChildFile("trial");
    check(specs.createDirectory().wasOk() && trial.createDirectory().wasOk(), "Create temporary manager fixture");
    for (const auto* name : { "a.wav", "b.wav", "video.mp4" })
        check(trial.getChildFile(name).replaceWithText("fixture"), "Create trial discovery fixture");
    XmlElement spec("test");
    spec.setAttribute("name", "Video AB");
    spec.setAttribute("testType", testTypes[TEST_TYPE_AVAB]);
    spec.setAttribute("stimuliDirectory", stimuli.getFullPathName());
    spec.setAttribute("trialsPerSession", "all");
    const auto file = specs.getChildFile("Video AB-testspec.xml");
    check(spec.writeTo(file), "Create temporary video A/B spec");
    {
        TestManagerComponent manager(specs);
        manager.setVisible(true);
        manager.refreshTestSpecs();
        auto* table = children<TableListBox>(manager)[0];
        auto click = [&](const String& text) {
            for (auto* button : children<TextButton>(manager))
                if (button->getButtonText() == text) {
                    button->triggerClick();
                    MessageManager::getInstance()->runDispatchLoopUntil(20);
                    return;
                }
            check(false, "Expected manager action is missing");
        };
        int saved = 0;
        manager.onSaved = [&] { ++saved; };
        click("Save Changes");
        check(saved == 1 && !manager.hasUnsavedChanges(), "A successful unchanged video A/B save must notify navigation");
        auto stored = parseXML(file);
        check(stored && stored->getStringAttribute("testType") == testTypes[TEST_TYPE_AVAB],
              "Saving unchanged video A/B must preserve the methodology without requiring a reference");
        table->selectRow(0);
        click("Delete Test");
        check(file.existsAsFile() && manager.hasUnsavedChanges() && table->getModel()->getNumRows() == 0,
              "Delete must remain an unsaved edit until Save Changes");
        manager.refreshTestSpecs();
        check(file.existsAsFile() && !manager.hasUnsavedChanges() && table->getModel()->getNumRows() == 1,
              "Reloading after discarding edits must restore a pending deletion");
        table->selectRow(0);
        click("Delete Test");
        click("Save Changes");
        check(!file.existsAsFile() && saved == 2 && !manager.hasUnsavedChanges(),
              "Save Changes must apply deletions and notify navigation only after success");
    }
    root.deleteRecursively();
}

void testManagerCellRestoration() {
    TestManagerComponent manager(File{});
    manager.setVisible(true);
    check(!manager.hasUnsavedChanges(), "A new test manager must start clean");
    manager.setSize(900, 600);
    auto tables = children<TableListBox>(manager);
    check(tables.size() == 1, "Manage Tests must have a specs table");
    auto* model = tables[0]->getModel();
    for (auto* button : children<TextButton>(manager))
        if (button->getButtonText() == "Add Test") {
            button->triggerClick();
            MessageManager::getInstance()->runDispatchLoopUntil(20);
            button->triggerClick();
            MessageManager::getInstance()->runDispatchLoopUntil(20);
        }
    check(model->getNumRows() == 2, "The fixture must create two in-memory specs");
    check(manager.hasUnsavedChanges(), "Adding test specs must mark Manage Tests as unsaved");

    std::unique_ptr<Component> cell(model->refreshComponentForCell(0, 3, false, nullptr));
    auto* combo = dynamic_cast<ComboBox*>(cell.get());
    check(combo != nullptr, "The test type cell must be a dropdown");
    for (int type = 0; type < NUMBER_OF_TEST_TYPES; ++type) {
        combo->setSelectedId(type + 1, sendNotificationSync);
        cell.reset();
        cell.reset(model->refreshComponentForCell(0, 3, false, nullptr));
        combo = dynamic_cast<ComboBox*>(cell.get());
        MessageManager::getInstance()->runDispatchLoopUntil(20);
        check(combo->getSelectedId() == type + 1,
              "A fresh test type cell must restore the spec instead of defaulting to MUSHRA");
        model->refreshComponentForCell(0, 3, false, combo);
        check(combo->getSelectedId() == type + 1,
              "Queued refresh notifications must not overwrite the stored test type");
    }

    model->refreshComponentForCell(1, 3, false, combo);
    combo->setSelectedId(TEST_TYPE_AB + 1, sendNotificationSync);
    model->refreshComponentForCell(0, 3, false, combo);
    check(combo->getSelectedId() == TEST_TYPE_AVAB + 1,
          "Editing a recycled dropdown must not change its former row");
    model->refreshComponentForCell(1, 3, false, combo);
    check(combo->getSelectedId() == TEST_TYPE_AB + 1,
          "Editing a recycled dropdown must update its current row");

    combo->setText("Unsupported test type", sendNotificationSync);
    cell.reset();
    cell.reset(model->refreshComponentForCell(1, 3, false, nullptr));
    MessageManager::getInstance()->runDispatchLoopUntil(20);
    combo = dynamic_cast<ComboBox*>(cell.get());
    check(combo->getText() == "Unsupported test type",
          "Unknown types must remain intact for validation instead of silently becoming MUSHRA");

    std::unique_ptr<Component> textCell(model->refreshComponentForCell(0, 1, false, nullptr));
    auto* editor = dynamic_cast<TextEditor*>(textCell.get());
    check(editor != nullptr, "The test name cell must be a text editor");
    editor->setText("First test");
    MessageManager::getInstance()->runDispatchLoopUntil(20);
    model->refreshComponentForCell(1, 1, false, editor);
    editor->setText("Second test");
    MessageManager::getInstance()->runDispatchLoopUntil(20);
    editor->onFocusLost();
    model->refreshComponentForCell(0, 1, false, editor);
    check(editor->getText() == "First test",
          "Text changes and focus loss in a recycled editor must not alter its former row");
    model->refreshComponentForCell(1, 1, false, editor);
    check(editor->getText() == "Second test",
          "A recycled editor must update its current row");
}

void surveyModelsAndReset() {
    auto choose = [](Component& survey, int group, const String& text) {
        for (auto* button : children<ToggleButton>(survey))
            if (button->getRadioGroupId() == group && button->getButtonText() == text) {
                button->setToggleState(true, dontSendNotification);
                return;
            }
        check(false, "Survey choice is missing");
    };
    auto submit = [](SurveyComponent& survey) {
        int continued = 0;
        survey.onContinue = [&] { ++continued; };
        check(survey.validateResponses().isEmpty(), "Completed fixture survey must validate without dialogs");
        for (auto* button : children<TextButton>(survey))
            if (button->getButtonText() == "OK") button->onClick();
        auto xml = survey.getSurveyResults();
        check(continued == 1 && (xml->getStringAttribute("surveyCompleted") == "yes"
                                || xml->getStringAttribute("surveyCompleted") == "1"),
              "Valid survey must mark completion before notifying the shell");
        survey.clearSurveyResponses();
        check(!survey.validateResponses().isEmpty()
              && (survey.getSurveyResults()->getStringAttribute("surveyCompleted") == "no"
                  || survey.getSurveyResults()->getStringAttribute("surveyCompleted") == "0"),
              "Clearing a completed survey must reset its model and completion state");
        survey.onContinue = nullptr;
        return xml;
    };
    BasicSurveyComponent basic;
    auto editors = children<TextEditor>(basic);
    check(editors.size() == 3, "Basic survey must expose age, experience and previous tests");
    editors[0]->setText("25"); editors[1]->setText("0"); editors[2]->setText("2");
    choose(basic, 1002, "Don't know");
    choose(basic, 1003, "No");
    for (const auto* text : { "", "-1", "1.2", "ten", "2147483648", "18446744073709551617" }) {
        editors[0]->setText(text);
        check(basic.validateResponses().size() == 1, "Basic survey must reject missing, invalid and overflowing ages");
    }
    for (const auto* text : { "0000", "2147483647" }) {
        editors[0]->setText(text);
        check(basic.validateResponses().isEmpty(), "Basic survey must accept zero and the largest valid integer");
    }
    editors[0]->setText("25");
    auto basicXml = submit(basic);
    check(basicXml->getStringAttribute("surveyCompleted") == "yes"
          && basicXml->getStringAttribute("normalHearing") == "unknown"
          && basicXml->getIntAttribute("yearsOfExperience", -1) == 0
          && basicXml->getIntAttribute("numberOfPreviousListeningTests") == 2
          && basicXml->getStringAttribute("testTakenInLastYear") == "no",
          "Basic survey must retain historical XML names and unknown/zero answers");
    BsuSurveyComponent bsu;
    children<TextEditor>(bsu)[0]->setText("30");
    choose(bsu, 1001, "YES");
    choose(bsu, 1002, "YES");
    choose(bsu, 1003, "NO");
    choose(bsu, 1005, "Novel");
    choose(bsu, 1006, "TCOM");
    choose(bsu, 1007, "NO");
    check(bsu.validateResponses().isEmpty(), "Optional BSU training and protection fields may be unanswered");
    choose(bsu, 1006, "Other (enter at right):");
    choose(bsu, 1002, "NO");
    choose(bsu, 1003, "YES");
    check(bsu.validateResponses().size() == 3, "BSU must require conditional department, language and protection answers");
    choose(bsu, 1004, "YES");
    check(bsu.validateResponses().size() == 3, "Protection use must require its type");
    auto bsuEditors = children<TextEditor>(bsu);
    bsuEditors[1]->setText("Audio <&> research");
    bsuEditors[4]->setText("French");
    bsuEditors[6]->setText("earplugs");
    check(bsu.validateResponses().isEmpty(), "BSU must accept completed conditional answers");
    bsuEditors[2]->setText("-1");
    check(bsu.validateResponses().size() == 1, "Optional training years must be valid when supplied");
    bsuEditors[2]->clear();
    auto bsuXml = submit(bsu);
    check(bsuXml->getStringAttribute("surveyCompleted") == "1"
          && bsuXml->getStringAttribute("passed15dbTest") == "true"
          && bsuXml->getStringAttribute("listenerIsTrained") == "false"
          && bsuXml->getStringAttribute("studentDept") == "Audio <&> research"
          && bsuXml->getStringAttribute("nativeLanguage") == "French"
          && bsuXml->getStringAttribute("usedHearingProtection") == "true"
          && bsuXml->getStringAttribute("hearingProtectionType") == "earplugs"
          && bsuXml->getStringAttribute("isMusician") == "false"
          && bsuXml->getIntAttribute("yearsMusician", -1) == 0,
          "BSU must retain historical numeric completion, boolean and optional-year encodings");
}

void testPickerRepositoryRefresh() {
    const auto root = File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("test-picker-repository", {}, false);
    root.createDirectory();
    // The picker reads metadata; media availability is checked when launching.
    const auto first = root.getChildFile("a-testspec.xml"), second = root.getChildFile("z-testspec.xml");
    for (const auto& file : {second, first}) {
        XmlElement xml("test");
        xml.setAttribute("name", file == first ? "First" : "Second");
        xml.setAttribute("testType", "AB_Forced_Choice");
        xml.setAttribute("stimuliDirectory", root.getFullPathName());
        check(xml.writeTo(file), "Create picker metadata fixture");
    }
    root.getChildFile("bad-testspec.xml").replaceWithText("<bad");
    NewTestSelectComponent picker(root);
    check(picker.populateComboBoxWithTestNames(), "Picker must retain readable specs when another spec is malformed");
    check(picker.discoveryErrors().size() == 1, "Picker must expose repository errors for the shell to present");
    auto* combo = children<ComboBox>(picker)[0];
    check(combo->getNumItems() == 2 && picker.getSelectedTestFile() == first, "Picker must retain the sorted file identity for each item");
    combo->setSelectedId(2, dontSendNotification);
    check(picker.getSelectedTestFile() == second, "Selecting a name must return the matching repository file");
    root.deleteRecursively();
    check(!picker.populateComboBoxWithTestNames() && combo->getNumItems() == 0 && picker.getSelectedTestFile() == File(),
          "A failed refresh must clear stale selections and files");
}

void selectionAndNavigation() {
    StimulusSelector selector;
    selector.configure({ "A", "B" }, false);
    auto buttons = children<TextButton>(selector);
    check(buttons.size() == 2, "A/B must have only its two stimulus buttons");
    int selected = -2;
    selector.onSelect = [&](int index) { selected = index; selector.setSelection(index); };
    buttons[1]->onClick();
    check(selected == 1 && buttons[1]->getToggleState() && !buttons[0]->getToggleState(),
          "Selection callbacks must use presentation order and retain selected state");
    check(buttons[0]->isRegisteredForShortcut(KeyPress('1')) && buttons[1]->isRegisteredForShortcut(KeyPress('2')),
          "Number shortcuts must follow presentation order");
    selector.configure({ "1", "2", "3" }, true);
    buttons = children<TextButton>(selector);
    check(buttons.size() == 4, "Rating selector must have a separate reference");
    check(buttons[0]->isRegisteredForShortcut(KeyPress('`')), "Reference must keep its backtick shortcut");
    buttons[0]->onClick();
    check(selected == -1 && buttons[0]->getToggleState(), "Reference must have its own callback identity");
    selector.configure({ "A", "B" }, false);
    for (auto* button : children<TextButton>(selector))
        check(!button->isRegisteredForShortcut(KeyPress('`')), "A/B must not retain a reference shortcut");


}

void ratingBehaviorAndLayout() {
    RatingControls ratings({ "Quality of each\nstimulus compared\nto the Reference",
                             { "Excellent", "Good", "Fair", "Poor", "Bad" }, true, true });
    ratings.configure(3, { 0, 100, 1, 0, true, true });
    int moved = -1;
    ratings.onRatingMoved = [&](int index) { moved = index; };
    ratings.setResponse(1, 73, "saved comment");
    check(moved == -1 && ratings.score(1) == 73 && ratings.comment(1) == "saved comment",
          "Restoring a rating must preserve data without marking it moved");
    ratings.setSelection(1);
    check(ratings.adjust(1, KeyPress(KeyPress::upKey)) && moved == 1 && ratings.score(1) == 74,
          "Up must adjust only the selected rating and mark its presentation index");
    check(!ratings.adjust(0, KeyPress(KeyPress::upKey)) && ratings.score(0) == 0,
          "An unselected rating must not change");
    ratings.setSelection(-1);
    check(!ratings.adjust(1, KeyPress(KeyPress::upKey)), "Reference selection must disable all rating edits");

    StimulusSelector selector;
    selector.configure({ "1", "2", "3" }, true);
    Component view;
    view.setSize(900, 480);
    view.setVisible(true);
    view.addAndMakeVisible(selector);
    view.addAndMakeVisible(ratings);
    for (const auto size : { Point<int>(900, 420), Point<int>(1600, 800) }) {
        ratings.setSize(size.x, size.y);
        selector.setSize(TestViewLayout::ratingColumnsWidth(size.x), 84);
        auto sliders = children<Slider>(ratings);
        auto buttons = children<TextButton>(selector);
        for (int i = 0; i < sliders.size(); ++i) {
            check(sliders[i]->getX() == buttons[i + 1]->getX()
                  && sliders[i]->getWidth() == buttons[i + 1]->getWidth(),
                  "Each rating must remain aligned with its stimulus button");
        }
        for (auto* label : children<Label>(ratings)) {
            const int lines = StringArray::fromLines(label->getText()).size();
            const auto textArea = label->getBorderSize().subtractedFrom(label->getLocalBounds());
            check(textArea.getHeight() >= int(label->getFont().getHeight() * lines),
                  "Rating heading and scale labels must have room for all text lines");
        }
    }
    // The rating heading occupies the stimulus row's right column. Its empty
    // left side must not intercept clicks on the selector underneath it.
    selector.setBounds(0, 0, TestViewLayout::ratingColumnsWidth(900), 84);
    ratings.setBounds(0, 52, 900, 428);
    auto* stimulus = children<TextButton>(selector)[1];
    check(view.getComponentAt(selector.getPosition() + stimulus->getBounds().getCentre()) == stimulus,
          "The rating panel must not block clicks on stimulus buttons");

    RatingControls impairment({ "Impairment", { "Imperceptible", "Perceptible, but not annoying",
                                                "Slightly Annoying", "Annoying", "Very Annoying" }, false, true });
    impairment.configure(2, { 1, 5, 0.1, 5, true, true });
    impairment.setResponse(0, 3.4, {});
    impairment.setSelection(0);
    impairment.adjust(0, KeyPress(KeyPress::upKey));
    check(std::abs(impairment.score(0) - 3.5) < 0.00001, "BS.1116 must retain fractional rating steps");
    RatingControls demo({ "Quality", { "Excellent", "Good", "Fair", "Poor", "Bad" }, true, false });
    demo.configure(3, { 0, 100, 1, 0, true, false });
    check(children<CommentBox>(demo).isEmpty(), "Demo must not allocate unused comments");
}

void preferenceBehavior() {
    PreferenceControls preference;
    check(children<Slider>(preference).size() == 1, "Preference controls must allocate only the preference slider");
    auto* slider = children<Slider>(preference)[0];
    int moves = 0;
    preference.onPreferenceMoved = [&] { ++moves; };
    preference.restore(-1, "first", "second");
    check(preference.preference() == -1 && moves == 0, "An unanswered trial must remain unanswered");
    slider->setValue(0, sendNotificationSync);
    check(preference.preference() == 0 && moves == 1 && slider->getInterval() == 1,
          "First preference must become an explicit binary choice");
    preference.restore(1, "saved A", "saved B");
    check(preference.preference() == 1 && moves == 1
          && preference.comment(0) == "saved A" && preference.comment(1) == "saved B",
          "Restoring a preference must preserve both comments without marking a change");
    preference.restore(-1, {}, {});
    check(preference.preference() == -1 && slider->getInterval() == 0.5,
          "A new unanswered trial must reset the initial neutral slider position");
}

void preferenceLayout(const File& snapshotDirectory) {
    struct Preview : Component {
        void paint(Graphics& g) override { g.fillAll(Colours::darkgrey); }
    } view;
    StimulusSelector selector;
    PreferenceControls preference;
    view.setVisible(true);
    view.addAndMakeVisible(selector);
    view.addAndMakeVisible(preference);
    selector.configure({ "A", "B" }, false, StimulusSelector::Layout::preference);
    preference.restore(1, "Comment A", "Comment B");
    for (int width : { 900, 1920, 3840 }) {
        for (int height : { 228, 480 }) {
            view.setSize(width, height);
            selector.setBounds(TestViewLayout::preferenceColumns(width));
            preference.setBounds(0, 60, width, height - 60);
            auto buttons = children<TextButton>(selector);
            auto comments = children<CommentBox>(preference);
            for (int i = 0; i < 2; ++i) {
                check(buttons[i]->getWidth() <= 160, "A/B buttons must retain their 160px width cap");
                check(comments[i]->getX() == selector.getX() + buttons[i]->getX()
                      && comments[i]->getWidth() == buttons[i]->getWidth(),
                      "A/B comments must stay aligned with the capped buttons");
            }
            auto* slider = children<Slider>(preference)[0];
            check(slider->getWidth() <= 240, "Preference slider must span the capped button centres");
            bool captionFound = false, choiceFound = false;
            for (auto* label : children<Label>(preference)) {
                const bool isChoice = label->getText() == "B"
                    && label->getJustificationType() == Justification::centred;
                if (label->getText() == "Your selection:" || isChoice) {
                    check(preference.getLocalBounds().contains(label->getBounds()),
                          "The selection caption and chosen letter must remain inside the controls region");
                    check(label->getFont().getHeight() >= (isChoice ? 44.0f : 14.0f),
                          "The selection caption and chosen letter must remain readable");
                    if (label->getText() == "Your selection:") captionFound = true;
                    else choiceFound = true;
                }
            }
            check(captionFound && choiceFound, "A/B must display both its selection caption and chosen letter");
            if (snapshotDirectory != File() && width == 900) {
                snapshotDirectory.createDirectory();
                FileOutputStream output(snapshotDirectory.getChildFile("ab-" + String(height) + ".png"));
                output.setPosition(0);
                output.truncate();
                PNGImageFormat().writeImageToStream(view.createComponentSnapshot(view.getLocalBounds()), output);
            }
        }
    }
}

void windowSizing() {
    // Exercise the real window controller without opening a desktop window or
    // constructing MainComponent (which would initialise an audio device).
    auto* content = new Component();
    content->setSize(900, 600);
    MainDocumentWindow window("Window sizing regression", content, false);
    auto checkTitleButtons = [&] {
        check(window.getCloseButton() && window.getMinimiseButton(), "Window must retain Close and Minimise");
        for (auto* button : { window.getCloseButton(), window.getMinimiseButton(), window.getMaximiseButton() }) {
            if (!button) continue;
           #if JUCE_MAC
            check(button->getRight() < window.getWidth() / 2, "macOS title buttons must remain in the upper left");
           #else
            check(button->getX() > window.getWidth() / 2, "Title buttons must remain in the upper right");
           #endif
        }
    };
    checkTitleButtons();
    auto tooltips = children<TooltipWindow>(window);
    check(tooltips.size() == 1, "The application window must own one shared tooltip manager");
    tooltips[0]->displayTip({ 100, 100 }, "demo-stimulus.wav");
    check(tooltips[0]->isVisible(), "The shared manager must display demo stimulus tooltips");
    tooltips[0]->hideTip();
    const auto maximised = window.getBounds();
    check(!window.isResizable() && !window.isDraggable(), "Startup must retain the maximised window policy");

    // The constructor has queued a maximisation update. It must use the new
    // policy when dispatched after A/B has loaded and the subject has resized.
    window.setTestType(TEST_TYPE_AB);
    check(window.isResizable() && window.isDraggable() && window.getMaximiseButton() != nullptr,
          "Audio A/B must allow resizing, moving, and maximising");
    checkTitleButtons();
    const auto* display = Desktop::getInstance().getDisplays().getPrimaryDisplay();
    check(display != nullptr, "Window sizing tests require access to desktop display information");
    auto chosen = display->userArea.withSizeKeepingCentre(900, 600);
    window.setBoundsConstrained(chosen);
    chosen = window.getBounds();
    MessageManager::getInstance()->runDispatchLoopUntil(20);
    check(window.getBounds() == chosen, "A queued maximisation update must not override the subject's A/B window size");
    window.setTestType(TEST_TYPE_AVAB);
    check(window.isResizable() && window.getBounds() == chosen, "Video A/B must retain the subject's chosen size");
    checkTitleButtons();

    for (auto type : { TEST_TYPE_BS1116, TEST_TYPE_MUSHRA, TEST_TYPE_MUSHRA_STRICT, TEST_TYPE_MUSHRA_DEMO }) {
        window.setTestType(type);
        check(!window.isResizable() && !window.isDraggable() && window.getMaximiseButton() == nullptr,
              "Rating tests must retain their fixed maximised window policy");
        check(window.getBounds() == maximised, "Loading a rating test must maximise the window");
        window.setBounds(30, 30, 450, 350);
        MessageManager::getInstance()->runDispatchLoopUntil(20);
        check(window.getBounds() == maximised, "Rating windows must return to maximised bounds after resize events");
        checkTitleButtons();
    }
    window.setTestType(TEST_TYPE_AB);
    check(window.getBounds() == chosen, "Returning to A/B must restore its previous window size and position");
    window.setBoundsConstrained({ 30, 30, 300, 300 });
    check(window.getWidth() >= 800 && window.getHeight() >= 600,
          "A/B's minimum window size must leave room for its controls and transport");
    checkTitleButtons();
    window.setFullScreen(true);
    checkTitleButtons();
    window.setFullScreen(false);
    checkTitleButtons();
}

void videoLayout(const File& snapshotDirectory) {
    struct Preview : Component {
        void paint(Graphics& g) override { g.fillAll(Colours::darkgrey); }
    } view;
    struct Video : Component {
        void paint(Graphics& g) override {
            g.fillAll(Colours::black);
            g.setColour(Colours::white);
            g.drawText("Video", getLocalBounds(), Justification::centred);
        }
    } video;
    StimulusSelector selector;
    PreferenceControls preference(PreferenceControls::Layout::video);
    TextButton next { "Next >" };
    Label help;
    view.setVisible(true);
    view.addAndMakeVisible(video);
    view.addAndMakeVisible(selector);
    view.addAndMakeVisible(preference);
    TestViewLayout::label(view, help, "Use the buttons\nto select which\nstimulus to play.");
    view.addAndMakeVisible(next);
    selector.configure({ "A", "B" }, false, StimulusSelector::Layout::preference);
    preference.restore(1, "Comment A", "Comment B");
    for (const auto size : { Point<int>(800, 380), Point<int>(900, 480), Point<int>(1600, 900) }) {
        view.setSize(size.x, size.y);
        const auto layout = TestViewLayout::videoAB(view.getLocalBounds());
        video.setBounds(layout.video);
        selector.setBounds(layout.selector);
        help.setBounds(layout.buttonHelp);
        TestViewLayout::fitText(help, 14, 3);
        preference.setBounds(layout.preference);
        next.setBounds(layout.navigation.withTrimmedLeft(layout.navigation.getWidth() - 96));
        const auto nextBounds = next.getBounds();
        check(nextBounds.getBottom() == size.y, "Video Next button must anchor to the bottom of the view");
        bool foundChoice = false;
        for (auto* label : children<Label>(preference)) {
            if (label->getJustificationType() == Justification::centredBottom) {
                const auto choiceBounds = label->getBounds().translated(preference.getX(), preference.getY());
                check(choiceBounds.getBottom() == nextBounds.getBottom(),
                      "The video A/B choice must share the Next button's bottom baseline");
                check(!choiceBounds.intersects(nextBounds) && label->getFont().getHeight() >= 48,
                      "The video choice must stay readable without overlapping Next");
                const auto renderedChoice = label->createComponentSnapshot(label->getLocalBounds());
                bool inkAtBottom = false;
                for (int x = 0; x < renderedChoice.getWidth(); ++x)
                    inkAtBottom = inkAtBottom || renderedChoice.getPixelAt(x, renderedChoice.getHeight() - 1).getAlpha() > 0;
                check(inkAtBottom, "The visible choice glyph must align with Next, without font descent padding");
                auto* slider = children<Slider>(preference)[0];
                check(slider->getBottom() < label->getY(), "The preference slider must sit above the choice");
                foundChoice = true;
            }
        }
        check(foundChoice, "Video A/B must have a bottom-aligned choice label");
        check(view.getComponentAt(nextBounds.getCentre()) == &next,
              "Preference controls must not intercept clicks on Next");
        check(layout.video.getBottom() <= layout.selector.getY() - 8,
              "Video must not overlap the control rows");
        check(layout.selector.getY() == size.y - 220,
              "Video controls must use compact rows without a separate navigation strip");
        check(view.getLocalBounds().contains(layout.video), "Video must fit inside the available view");
        if (snapshotDirectory != File() && size.x == 900) {
            snapshotDirectory.createDirectory();
            FileOutputStream output(snapshotDirectory.getChildFile("video-ab.png"));
            output.setPosition(0);
            output.truncate();
            PNGImageFormat().writeImageToStream(view.createComponentSnapshot(view.getLocalBounds()), output);
        }
    }
    const auto smaller = TestViewLayout::videoAB({ 0, 0, 1600, 600 });
    const auto larger = TestViewLayout::videoAB({ 0, 0, 1600, 800 });
    check(larger.video.getHeight() - smaller.video.getHeight() == 200,
          "Extra window height must go to video rather than stretching the controls");
}
}

int main(int argc, char** argv) {
    ScopedJuceInitialiser_GUI gui;
    try {
        runApplicationTests();
        pageNavigation(argc > 1 ? File(argv[1]) : File());
        testManagerSaveAndDiscard();
        testManagerCellRestoration();
        surveyModelsAndReset();
        testPickerRepositoryRefresh();
        selectionAndNavigation();
        ratingBehaviorAndLayout();
        preferenceBehavior();
        preferenceLayout(argc > 1 ? File(argv[1]) : File());
        videoLayout(argc > 1 ? File(argv[1]) : File());
        windowSizing();
        std::cout << "Page navigation, test manager saving, selection, rating, preference, and window sizing regression checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
