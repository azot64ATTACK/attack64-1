#include "PluginEditor.h"

Attack64Editor::Attack64Editor(Attack64Processor& p)
    : AudioProcessorEditor(&p)
{
    for (int t = 0; t < 10; ++t)
    {
        pages.push_back(std::make_unique<Page>(p.apvts, t));
        tabs.addTab(kTabs[t].name, juce::Colour(0xff232b33), pages.back().get(), false);
    }
    addAndMakeVisible(tabs);
    setSize(600, 300);
}

void Attack64Editor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1d21));
    g.setColour(juce::Colour(0xff5ef0d0));
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("ATTACK64", getLocalBounds().removeFromTop(34), juce::Justification::centred);
}

void Attack64Editor::resized()
{
    tabs.setBounds(getLocalBounds().withTrimmedTop(36).reduced(10));
}
