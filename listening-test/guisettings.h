//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#ifndef GUISETTINGS_H
#define GUISETTINGS_H

#define min(a, b)((a)<(b)?(a):(b))
#define max(a, b)((a)>(b)?(a):(b))

const int SLIDER_MAXVALUE = 100;
const int TIMER_CALLBACK_INTERVAL_MS = 100;

#define    activeButtonColour        Colours::hotpink

#define referenceButtonColour    Colours::darkgreen
#define stimuliButtonColour     Colours::darkslateblue

#define    LINE_BG_COLOUR    Colour(0xff2a5ca5)
#define    LINE_COLOUR       Colour(238,238,209)

const int DEFAULT_HEIGHT = 600;
const int DEFAULT_WIDTH = 900;

// vertical alignment setup
const int Y_SPACING = 40; // space between elements
const int X_SPACING = 10;

const int REF_BUTTON_Y = 0;

const int BUTTON_Y = REF_BUTTON_Y + Y_SPACING;
const int BUTTON_H = 32;
const int BUTTON_W = 96;

const int SLIDER_Y = BUTTON_Y + Y_SPACING;
const int SLIDER_H = 400 - BUTTON_H;

// Horizontal alignment setup
const int BUTTON_X = 0;
const int SLIDER_X = 0;
const int REF_BUTTON_W = 600;
const int LINE_W = REF_BUTTON_W; // Horizontal lines that separate ratings regions in sliders
const float REL_BUTTON_SPACING = 0.75f;
const int LABEL_X = SLIDER_X + LINE_W + X_SPACING * 2;
const int LABEL_W = 100;

const float SLIDER_DISABLED_ALPHA = 0.4f;

const Colour transp((uint8) 0, (uint8) 0, (uint8) 0, (uint8) 0);

// Useful utility for sizing a label to match the text contained therein
static inline void setupLabel(Label &l, int x, int y) {
    l.setBorderSize(BorderSize<int>(0));
    
    int lines = 0;
    int w = 0;
    int i = 0;
    String text = l.getText();
    while(i < text.length()) {
        lines++;
        int j = text.indexOfChar(i, '\n');
        if (j == -1) {
            j = text.length();
        }
        w = max(w, l.getFont().getStringWidth(text.substring(i,j)));
        i = j+1;
    }
    
    l.setBounds(x, y, w, l.getFont().getHeight() * lines);
}

#endif
