// Listening Test Application
// Copyright(C) 2017-2026 Netflix, Inc.
#include "TestViewControls.h"
#include "AppTheme.h"

TransportButton::TransportButton(Icon initialIcon) : icon(initialIcon) {
    setIcon(initialIcon);
}

void TransportButton::setIcon(Icon value) {
    icon = value;
    switch (icon) {
        case Icon::restart: setButtonText("Restart"); break;
        case Icon::rewind: setButtonText("Rewind"); break;
        case Icon::play: setButtonText("Play"); break;
        case Icon::pause: setButtonText("Pause"); break;
        case Icon::fastForward: setButtonText("Fast forward"); break;
    }
    repaint();
}

void TransportButton::paintButton(Graphics& g, bool highlighted, bool down) {
    getLookAndFeel().drawButtonBackground(g, *this,
        findColour(getToggleState() ? buttonOnColourId : buttonColourId), highlighted, down);
    g.setColour(findColour(getToggleState() ? textColourOnId : textColourOffId)
                    .withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f));

    // All triangles share the same geometry; compound icons repeat it.
    Path shape;
    auto triangle = [&](float x, bool left) {
        shape.addTriangle(x + (left ? 9.0f : 0.0f), 0.0f,
                          x + (left ? 0.0f : 9.0f), 6.0f,
                          x + (left ? 9.0f : 0.0f), 12.0f);
    };
    if (icon == Icon::pause) {
        shape.addRectangle(0.0f, 0.0f, 3.0f, 12.0f);
        shape.addRectangle(7.0f, 0.0f, 3.0f, 12.0f);
    } else {
        const bool left = icon == Icon::restart || icon == Icon::rewind;
        const float start = icon == Icon::restart ? 5.0f : 0.0f;
        if (icon == Icon::restart)
            shape.addRectangle(0.0f, 0.0f, 3.0f, 12.0f);
        triangle(start, left);
        if (icon != Icon::play)
            triangle(start + 11.0f, left);
    }
    const float scale = jmax(0.0f, jmin(1.0f, jmin((getWidth() - 12.0f) / shape.getBounds().getWidth(),
                                                 (getHeight() - 8.0f) / 12.0f)));
    const auto bounds = shape.getBounds();
    g.fillPath(shape, AffineTransform::scale(scale).translated(
        (getWidth() - bounds.getWidth() * scale) * 0.5f,
        (getHeight() - bounds.getHeight() * scale) * 0.5f));
}

namespace TestViewLayout {
int ratingColumnsWidth(int width) {
    return jmax(0, width - jlimit(180, 300, width / 3) - gap);
}

int preferenceHelpWidth(int width) { return jmin(160, jmax(0, width / 4)); }

Rectangle<int> preferenceColumns(int width) {
    const int margin = preferenceHelpWidth(width) + gap;
    // Restore A/B's original 160px button cap and half-button gap. Centre the
    // pair when space permits, leaving room for the instructions on the left.
    const int buttonWidth = jmin(160, jmax(0, width - margin) * 2 / 5);
    const int pairWidth = buttonWidth * 5 / 2;
    return { jmax(margin, (width - pairWidth) / 2), 0, pairWidth, buttonHeight };
}

Rectangle<int> preferenceColumn(int index, Rectangle<int> pairBounds) {
    return { pairBounds.getX() + index * pairBounds.getWidth() * 3 / 5, pairBounds.getY(),
             pairBounds.getWidth() * 2 / 5, pairBounds.getHeight() };
}

VideoABLayout videoAB(Rectangle<int> bounds) {
    VideoABLayout result;
    result.navigation = bounds.withHeight(buttonHeight).withY(bounds.getBottom() - buttonHeight)
        .removeFromRight(jmin(navigationWidth, bounds.getWidth()));
    // Stack upward from the shared choice/Next baseline. Only these compact
    // rows and one gap are reserved; all remaining space belongs to the video.
    const int controlsHeight = 52 + 8 + videoPreferenceHeight;
    auto controls = bounds.removeFromBottom(jmin(controlsHeight, bounds.getHeight()));
    result.buttonHelp = { controls.getX(), controls.getY(), preferenceHelpWidth(controls.getWidth()), 52 };
    result.selector = preferenceColumns(controls.getWidth()).translated(controls.getX(), controls.getY());
    controls.removeFromTop(jmin(60, controls.getHeight()));
    result.preference = controls;
    bounds.removeFromBottom(jmin(8, bounds.getHeight()));
    const int width = jmax(0, jmin(bounds.getWidth(), int(bounds.getHeight() * 16.0 / 9.0)));
    result.video = bounds.withSizeKeepingCentre(width, jmax(0, roundToInt(width * 9.0 / 16.0)));
    return result;
}

Rectangle<int> column(int index, int count, int width, int y, int height) {
    if (count <= 0) return {};
    const float buttonWidth = width / (1.0f + (count - 1) / AppTheme::relativeButtonSpacing);
    const int x = roundToInt(index * buttonWidth / AppTheme::relativeButtonSpacing);
    return { x, y, jmax(0, jmin(roundToInt(buttonWidth), width - x)), jmax(0, height) };
}

void label(Component& parent, Label& label, const String& text, Justification justification) {
    parent.addAndMakeVisible(label);
    label.setText(text, dontSendNotification);
    label.setJustificationType(justification);
    label.setEditable(false, false, false);
    label.setColour(Label::textColourId, AppTheme::lineColour);
}

void fitText(Label& label, float desiredHeight, int lines) {
    const auto area = label.getBorderSize().subtractedFrom(label.getLocalBounds());
    Font font(jmax(1.0f, jmin(desiredHeight, float(area.getHeight()) / jmax(1, lines))), Font::bold);
    float widestLine = 0;
    for (const auto& line : StringArray::fromLines(label.getText()))
        widestLine = jmax(widestLine, font.getStringWidthFloat(line));
    if (widestLine > area.getWidth() && widestLine > 0)
        font.setHeight(jmax(1.0f, font.getHeight() * jmax(0, area.getWidth()) / widestLine));
    label.setFont(font);
}
}

void StimulusSelector::configure(const StringArray& names, bool hasReference, Layout arrangement) {
    layout = arrangement;
    buttons.clear();
    reference.reset();
    auto setup = [this](TextButton& button, int index, Colour colour) {
        addAndMakeVisible(button);
        button.setColour(TextButton::buttonColourId, colour);
        button.setColour(TextButton::buttonOnColourId, AppTheme::activeButtonColour);
        button.onClick = [this, &button, index] {
            if (!button.isEnabled()) return;
            button.toFront(true);
            if (onSelect) onSelect(index);
        };
    };
    if (hasReference) {
        reference.reset(new TextButton("Reference"));
        setup(*reference, -1, AppTheme::referenceButtonColour);
        reference->addShortcut(KeyPress::createFromDescription("`"));
       #if JUCE_WINDOWS
        // GetAsyncKeyState needs the virtual-key code for the key left of 1.
        reference->addShortcut(KeyPress(0xC0));
       #endif
    }
    const String shortcuts[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=" };
    for (int i = 0; i < names.size(); ++i) {
        auto* button = buttons.add(new TextButton(names[i]));
        setup(*button, i, AppTheme::stimuliButtonColour);
        if (i < 12) button->addShortcut(KeyPress::createFromDescription(shortcuts[i]));
    }
    resized();
}

void StimulusSelector::setSelection(int index) {
    if (reference) reference->setToggleState(index == -1, dontSendNotification);
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setToggleState(i == index, dontSendNotification);
}

void StimulusSelector::setTooltip(int index, const String& text) {
    if (isPositiveAndBelow(index, buttons.size())) buttons[index]->setTooltip(text);
}

void StimulusSelector::resized() {
    if (reference) reference->setBounds(0, 0, getWidth(), TestViewLayout::buttonHeight);
    const int y = reference ? TestViewLayout::buttonHeight + TestViewLayout::gap : 0;
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setBounds(layout == Layout::preference
            ? TestViewLayout::preferenceColumn(i, { 0, y, getWidth(), TestViewLayout::buttonHeight })
            : TestViewLayout::column(i, buttons.size(), getWidth(), y, TestViewLayout::buttonHeight));
}

RatingControls::RatingControls(RatingScale settings) : scale(std::move(settings)) {
    // The heading shares the selector's row. Empty panel space must let
    // stimulus clicks through while sliders and comment editors remain active.
    setInterceptsMouseClicks(false, true);
    TestViewLayout::label(*this, heading, scale.heading);
    for (const auto& text : scale.labels) {
        auto* label = labels.add(new Label());
        TestViewLayout::label(*this, *label, text, Justification::centredLeft);
        label->setFont(Font(15));
    }
}

void RatingControls::configure(int count, Methodology methodology) {
    policy = methodology;
    sliders.clear();
    comments.clear();
    for (int i = 0; i < count; ++i) {
        auto* slider = sliders.add(new Slider("rating" + String(i)));
        addAndMakeVisible(slider);
        slider->setRange(policy.minimum, policy.maximum, policy.interval);
        slider->setValue(policy.defaultScore, dontSendNotification);
        slider->setSliderStyle(Slider::LinearVertical);
        slider->setTextBoxStyle(Slider::TextBoxBelow, false, 80, 20);
        slider->setColour(Slider::thumbColourId, Colours::grey);
        slider->onValueChange = [this, i] { if (onRatingMoved) onRatingMoved(i); };
        if (scale.comments) {
            auto* comment = comments.add(new CommentBox("comment" + String(i)));
            addAndMakeVisible(comment);
        }
    }
    setSelection(-1);
    resized();
}

void RatingControls::setSelection(int index) {
    for (int i = 0; i < sliders.size(); ++i) {
        const bool active = i == index;
        sliders[i]->setEnabled(active);
        sliders[i]->setAlpha(active ? 1.0f : AppTheme::disabledSliderAlpha);
        sliders[i]->setTextBoxIsEditable(active);
    }
}

void RatingControls::setResponse(int index, double value, const String& text) {
    sliders[index]->setValue(value, dontSendNotification);
    if (scale.comments) comments[index]->setText(text, false);
}

double RatingControls::score(int index) const { return sliders[index]->getValue(); }
String RatingControls::comment(int index) const { return scale.comments ? comments[index]->getText() : String(); }

bool RatingControls::adjust(int index, const KeyPress& key) {
    if (!isPositiveAndBelow(index, sliders.size()) || !sliders[index]->isEnabled()) return false;
    auto& slider = *sliders[index];
    double delta;
    if (key == KeyPress::upKey) delta = policy.interval;
    else if (key == KeyPress::downKey) delta = -policy.interval;
    else if (key == KeyPress::pageUpKey) delta = policy.maximum / 5.0;
    else if (key == KeyPress::pageDownKey) delta = -policy.maximum / 5.0;
    else return false;
    slider.setValue(slider.getValue() + delta, sendNotificationSync);
    return true;
}

void RatingControls::resized() {
    using namespace TestViewLayout;
    dividers.clear();
    const int columnsWidth = ratingColumnsWidth(getWidth());
    const int labelX = columnsWidth + gap;
    // The heading shares the row occupied by stimulus buttons in the selector.
    heading.setBounds(labelX, 0, getWidth() - labelX, buttonHeight + gap);
    fitText(heading, 18, StringArray::fromLines(scale.heading).size());
    const int sliderY = buttonHeight + gap;
    const int commentHeight = scale.comments ? buttonHeight : 0;
    const int sliderHeight = jmax(0, getHeight() - sliderY - commentHeight);
    for (int i = 0; i < sliders.size(); ++i) {
        sliders[i]->setBounds(column(i, sliders.size(), columnsWidth, sliderY, sliderHeight));
        if (scale.comments)
            comments[i]->setBounds(column(i, sliders.size(), columnsWidth,
                                          getHeight() - commentHeight + 8, commentHeight - 8));
    }
    if (sliders.isEmpty()) return;
    auto& first = *sliders[0];
    const double step = (policy.maximum - policy.minimum) / (scale.labelsBetweenTicks ? labels.size() : labels.size() - 1);
    for (int i = 0; i < labels.size(); ++i) {
        const double value = policy.maximum - i * step;
        const int y = first.getY() + roundToInt(first.getPositionOfValue(value));
        const int labelHeight = scale.labelsBetweenTicks
            ? jmax(1, roundToInt(std::abs(first.getPositionOfValue(value - step) - first.getPositionOfValue(value))))
            : buttonHeight;
        labels[i]->setBounds(labelX, scale.labelsBetweenTicks ? y : y - labelHeight / 2,
                             getWidth() - labelX, labelHeight);
        fitText(*labels[i], 15);
        if (!scale.labelsBetweenTicks || i > 0)
            dividers.add(Line<float>(0, float(y), float(columnsWidth), float(y)));
    }
    repaint();
}

void RatingControls::paint(Graphics& g) {
    g.setColour(AppTheme::lineColour);
    for (const auto& line : dividers) g.drawLine(line, 2.0f);
}

void PreferenceControls::ChoiceLabel::paint(Graphics& g) {
    if (getJustificationType() != Justification::centredBottom) {
        Label::paint(g);
        return;
    }
    // A/B capitals have no descenders. Align their visible glyphs to the bottom
    // rather than leaving the font's descent padding above the Next baseline.
    GlyphArrangement glyphs;
    glyphs.addLineOfText(getFont(), getText(), 0, 0);
    Path outline;
    glyphs.createPath(outline);
    const auto ink = outline.getBounds();
    g.setColour(findColour(Label::textColourId));
    glyphs.draw(g, AffineTransform::translation(getWidth() * 0.5f - ink.getCentreX(),
                                               getHeight() - ink.getBottom()));
}

PreferenceControls::PreferenceControls(Layout arrangement) : layout(arrangement) {
    using namespace TestViewLayout;
    label(*this, notes, "Notes\n(optional)");
    label(*this, heading, "Use the slider\nto choose your\npreferred stimulus.");
    label(*this, selection, "Your selection:");
    label(*this, choice, "", Justification::centred);
    if (layout == Layout::video) {
        choice.setJustificationType(Justification::centredBottom);
        choice.setBorderSize(BorderSize<int>(0));
    }
    label(*this, left, "A", Justification::centredRight);
    label(*this, right, "B", Justification::centredLeft);
    addAndMakeVisible(commentA);
    addAndMakeVisible(commentB);
    addAndMakeVisible(slider);
    slider.setSliderStyle(Slider::LinearHorizontal);
    slider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    restore(-1, {}, {});
    slider.onValueChange = [this] {
        updateChoice();
        if (onPreferenceMoved) onPreferenceMoved();
    };
}

int PreferenceControls::preference() const {
    const double value = slider.getValue();
    return value == 0 ? 0 : value == 1 ? 1 : -1;
}

String PreferenceControls::comment(int index) const { return index == 0 ? commentA.getText() : commentB.getText(); }

void PreferenceControls::updateChoice() {
    const int selected = preference();
    choice.setText(selected == 0 ? "A" : selected == 1 ? "B" : "", dontSendNotification);
    TestViewLayout::fitText(choice, jmin(layout == Layout::video ? 48.0f : 240.0f, float(choice.getHeight())));
    if (selected >= 0) slider.setRange(0, 1, 1);
}

void PreferenceControls::restore(int preferred, const String& a, const String& b) {
    slider.setRange(0, 1, 0.5);
    slider.setValue(preferred < 0 ? 0.5 : double(preferred), dontSendNotification);
    updateChoice();
    commentA.setText(a, false);
    commentB.setText(b, false);
}

void PreferenceControls::resized() {
    using namespace TestViewLayout;
    const int helpWidth = preferenceHelpWidth(getWidth());
    const auto columns = preferenceColumns(getWidth());
    notes.setBounds(0, 0, helpWidth, 40);
    fitText(notes, 14, 2);
    auto a = preferenceColumn(0, columns.withHeight(40));
    auto b = preferenceColumn(1, columns.withHeight(40));
    commentA.setBounds(a);
    commentB.setBounds(b);
    heading.setBounds(0, 48, helpWidth, 52);
    fitText(heading, 14, 3);
    const int sliderX = a.getCentreX();
    slider.setBounds(sliderX, 56, b.getCentreX() - sliderX, buttonHeight);
    left.setBounds(slider.getX() - 16, slider.getY(), 16, buttonHeight);
    right.setBounds(slider.getRight(), slider.getY(), 16, buttonHeight);
    if (layout == Layout::video)
        choice.setBounds(slider.getX(), getHeight() - videoChoiceHeight, slider.getWidth(), videoChoiceHeight);
    else
        choice.setBounds(slider.getX(), 104, slider.getWidth(), jmax(0, getHeight() - 104));
    selection.setBounds(0, choice.getBounds().getCentreY() - 12, helpWidth, 24);
    fitText(selection, 14);
    fitText(choice, jmin(layout == Layout::video ? 48.0f : 240.0f, float(choice.getHeight())));
}
