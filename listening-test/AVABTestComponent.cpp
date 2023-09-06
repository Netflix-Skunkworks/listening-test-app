//
//    Listening Test Application
//    Copyright(C) 2017  Netflix, Inc.

#include "AVABTestComponent.h"


AVABTestComponent::AVABTestComponent(AudioPlayer &aP, TestLauncher &tL) : ABTestComponent(aP, tL) {

    addAndMakeVisible(aP.getVideoComponent());

}

void AVABTestComponent::resized() {
    ABTestComponent::resized();
    
    // Move all test controls to the bottom to accommodate the video player
    
    // Stack slider, buttons, and labels tightly above
    headerLabel.setTopLeftPosition(headerLabel.getX(), getHeight() - headerLabel.getHeight());
    preferenceSlider.setTopLeftPosition(preferenceSlider.getX(), headerLabel.getY());
    sliderLeftLabel.setTopLeftPosition(sliderLeftLabel.getX(), preferenceSlider.getY());
    sliderRightLabel.setTopLeftPosition(sliderRightLabel.getX(), preferenceSlider.getY());
    
    commentsLabel.setTopLeftPosition(commentsLabel.getX(), headerLabel.getY() - commentsLabel.getHeight()-8);
    commentBoxes[0]->setTopLeftPosition(commentBoxes[0]->getX(), commentsLabel.getY());
    commentBoxes[1]->setTopLeftPosition(commentBoxes[1]->getX(), commentsLabel.getY());

    buttonLabel.setTopLeftPosition(buttonLabel.getX(), commentsLabel.getY() - buttonLabel.getHeight()-8);
    stimuliButtons[0]->setTopLeftPosition(stimuliButtons[0]->getX(), buttonLabel.getY());
    stimuliButtons[1]->setTopLeftPosition(stimuliButtons[1]->getX(), stimuliButtons[0]->getY());

    // Relocate 'your selection' to space above 'next' button
    yourSelectionLabel.setFont(Font(14, Font::bold));
    yourSelectionLabel.setBounds(nextButton.getX(), stimuliButtons[0]->getY(),
                                 nextButton.getWidth(), yourSelectionLabel.getFont().getHeight());
    preferenceLabel.setFont(Font(48, Font::bold));
    preferenceLabel.setBounds(nextButton.getX(), yourSelectionLabel.getBottom(),
                              nextButton.getWidth(), preferenceLabel.getFont().getHeight());

    // Make the video as big as possible in the remaining space
    int maxVideoPlayerWidth = getWidth();
    int maxVideoPlayerHeight = stimuliButtons[0]->getY() - 16;

    int maxVideoPlayerWidthHeight = maxVideoPlayerWidth * 9.0f / 16;
    int maxVideoPlayerHeightWidth = maxVideoPlayerHeight * 16.0f / 9;

    int videoPlayerWidth = maxVideoPlayerWidth;
    int videoPlayerHeight = maxVideoPlayerHeight;
    if (maxVideoPlayerWidthHeight > maxVideoPlayerHeight) {
        videoPlayerHeight = maxVideoPlayerHeight;
        videoPlayerWidth = maxVideoPlayerHeightWidth;
    } else if (maxVideoPlayerHeightWidth > maxVideoPlayerWidth) {
        videoPlayerHeight = maxVideoPlayerWidthHeight;
        videoPlayerWidth = maxVideoPlayerWidth;
    }

    audioPlayer.getVideoComponent()->setBounds((getWidth() - videoPlayerWidth) / 2, 0, videoPlayerWidth,
                                               videoPlayerHeight);

}
