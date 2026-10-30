// Listening Test Application
// Copyright(C) 2017-2026 Netflix, Inc.
#pragma once

#include <JuceHeader.h>
#include "CommentBox.h"
#include "../session/Trial.h"
#include <functional>

class TransportButton : public TextButton {
public:
    enum class Icon { restart, rewind, play, pause, fastForward };
    explicit TransportButton(Icon);
    void setIcon(Icon);
    void paintButton(Graphics&, bool highlighted, bool down) override;
private:
    Icon icon;
};

namespace TestViewLayout {
constexpr int gap = 20;
constexpr int buttonHeight = 32;
constexpr int navigationWidth = 200;
int ratingColumnsWidth(int width);
int preferenceHelpWidth(int width);
Rectangle<int> preferenceColumns(int width);
Rectangle<int> preferenceColumn(int index, Rectangle<int> pairBounds);
struct VideoABLayout {
    Rectangle<int> video, selector, buttonHelp, preference, navigation;
};
constexpr int videoChoiceHeight = 52;
constexpr int videoPreferenceHeight = 40 + 8 + 52 + 8 + videoChoiceHeight;
VideoABLayout videoAB(Rectangle<int> bounds);
Rectangle<int> column(int index, int count, int width, int y, int height);
void label(Component& parent, Label&, const String&, Justification = Justification::topLeft);
void fitText(Label&, float desiredHeight, int lines = 1);
}

// Presentation indices are UI positions; -1 denotes the separate reference.
class StimulusSelector : public Component {
public:
    enum class Layout { rating, preference };
    void configure(const StringArray& names, bool hasReference, Layout = Layout::rating);
    void setSelection(int presentationIndex);
    void setTooltip(int presentationIndex, const String&);
    void resized() override;
    std::function<void(int)> onSelect;
private:
    OwnedArray<TextButton> buttons;
    std::unique_ptr<TextButton> reference;
    Layout layout = Layout::rating;
};

struct RatingScale {
    String heading;
    StringArray labels;
    bool labelsBetweenTicks = false;
    bool comments = true;
};

class RatingControls : public Component {
public:
    explicit RatingControls(RatingScale);
    void configure(int count, Methodology);
    void setSelection(int presentationIndex);
    void setResponse(int index, double score, const String& comment);
    double score(int index) const;
    String comment(int index) const;
    bool adjust(int presentationIndex, const KeyPress&);
    void resized() override;
    void paint(Graphics&) override;
    std::function<void(int)> onRatingMoved;
private:
    RatingScale scale;
    Methodology policy {};
    Label heading;
    OwnedArray<Label> labels;
    OwnedArray<Slider> sliders;
    OwnedArray<CommentBox> comments;
    Array<Line<float>> dividers;
};

// The same preference controls are laid out in either an audio or a video region.
class PreferenceControls : public Component {
public:
    enum class Layout { audio, video };
    explicit PreferenceControls(Layout = Layout::audio);
    void restore(int preferredPresentation, const String& commentA, const String& commentB);
    int preference() const;
    String comment(int index) const;
    void resized() override;
    std::function<void()> onPreferenceMoved;
private:
    class ChoiceLabel : public Label {
        void paint(Graphics&) override;
    };
    void updateChoice();
    const Layout layout;
    Slider slider;
    Label heading, selection, left, right, notes;
    ChoiceLabel choice;
    CommentBox commentA { "commentA" }, commentB { "commentB" };
};
