#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class Attack64Editor : public juce::AudioProcessorEditor {
public:
    explicit Attack64Editor(Attack64Processor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Одна вкладка: 4 маленькие ручки с подписями
    struct Page : public juce::Component {
        Page(juce::AudioProcessorValueTreeState& apvts, int tab) {
            for (int i = 0; i < 4; ++i) {
                auto& k = knobs[i];
                k.label.setText(kTabs[tab].params[i].name, juce::dontSendNotification);
                k.label.setJustificationType(juce::Justification::centred);
                k.slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
                k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
                addAndMakeVisible(k.label);
                addAndMakeVisible(k.slider);
                k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    apvts, paramId(tab, i), k.slider);
            }
        }

        void resized() override {
            auto area = getLocalBounds().reduced(6);
            const int colW = area.getWidth() / 4;
            for (auto& k : knobs) {
                auto col = area.removeFromLeft(colW).reduced(4);
                k.label.setBounds(col.removeFromTop(18));
                k.slider.setBounds(col);
            }
        }

        struct Knob {
            juce::Label label;
            juce::Slider slider;
            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        };
        Knob knobs[4];
    };

    // pages объявлены раньше tabs, чтобы уничтожаться после них
    std::vector<std::unique_ptr<Page>> pages;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Attack64Editor)
};
