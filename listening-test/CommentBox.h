//
//    Listening Test Application
//    Copyright(C) 2023 Netflix, Inc.


#pragma once

#include <JuceHeader.h>


class CommentBox : public juce::TextEditor
{
public:
    CommentBox(const String& s) : TextEditor(s) {}
protected:
    void escapePressed() override {
        giveAwayKeyboardFocus();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CommentBox)
};
