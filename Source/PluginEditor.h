#pragma once
#include "PluginProcessor.h"

struct SkinTheme {
    juce::Colour bgDark { 0xff0a0c10 }, panelBg { 0xff13161d }, panelBorder { 0xff232934 };
    juce::Colour accentPrimary { 0xff00e5ff }, accentSecondary { 0xffff2a85 }, accentTertiary { 0xff2ee59d }, textMain { 0xffeef3f8 };
};

class SerumLookAndFeel : public juce::LookAndFeel_V4 {
public:
    SkinTheme theme;
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle, juce::Slider& slider) override;
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class WaveformDisplay : public juce::Component, public juce::FileDragAndDropTarget, private juce::Timer {
public:
    WaveformDisplay(WaeraAudioProcessor& proc, int oscIndex, const juce::String& onID, const juce::String& waveID, const juce::String& posID, SkinTheme& themeRef);
    std::function<void()> onSampleDropped;
    bool isInterestedInFileDrag(const juce::StringArray& files) override { return files.size() > 0 && files[0].endsWithIgnoreCase(".wav"); }
    void filesDropped(const juce::StringArray& files, int, int) override {
        processor.loadSampleIntoOsc(oscIdx, juce::File(files[0]));
        if (onSampleDropped) onSampleDropped();
        repaint();
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (wtPosID.isNotEmpty()) {
            if (auto* p = processor.apvts.getParameter(wtPosID))
                p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, 1.0f - (float)e.y / (float)getHeight()));
        }
    }
    void paint(juce::Graphics& g) override;
    void timerCallback() override { repaint(); }
private:
    WaeraAudioProcessor& processor;
    int oscIdx;
    juce::String onParamID, paramID, wtPosID;
    SkinTheme& theme;
};

class LFODisplay : public juce::Component, private juce::Timer {
public:
    LFODisplay(juce::AudioProcessorValueTreeState& apvts, int lfoIdx, SkinTheme& themeRef);
    void paint(juce::Graphics& g) override;
    void timerCallback() override {
        float rate = state.getRawParameterValue("lfo" + juce::String(idx) + "_rate")->load();
        phase = std::fmod(phase + rate * 0.025f, 1.0f);
        repaint();
    }
private:
    juce::AudioProcessorValueTreeState& state;
    int idx;
    float phase = 0.0f;
    SkinTheme& theme;
};

class ModSourceBadge : public juce::Component {
public:
    int sourceIndex = 1;
    juce::String labelText;
    juce::Colour badgeCol;
    ModSourceBadge(const juce::String& name, int srcIdx, juce::Colour col)
        : sourceIndex(srcIdx), labelText(name), badgeCol(col) {
        setRepaintsOnMouseActivity(true);
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(badgeCol.withAlpha(isMouseOver() ? 0.38f : 0.16f));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(badgeCol.withAlpha(isMouseOver() ? 1.0f : 0.75f));
        g.drawRoundedRectangle(r, 4.0f, 1.2f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(labelText, getLocalBounds(), juce::Justification::centred);
    }
    void mouseDown(const juce::MouseEvent&) override {
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
            juce::Image dragImg(juce::Image::ARGB, getWidth(), getHeight(), true);
            juce::Graphics ig(dragImg); paint(ig);
            container->startDragging("MOD_SRC:" + juce::String(sourceIndex), this, dragImg, true);
        }
    }
};

class ModTargetKnob : public juce::Slider, public juce::DragAndDropTarget {
public:
    int modDestIndex = 0;
    bool isDragHovered = false;
    std::function<void(int srcIdx, int dstIdx)> onModDropped;
    std::function<void(int dstIdx)> onModCleared;

    bool isInterestedInDragSource(const SourceDetails& d) override {
        return modDestIndex > 0 && d.description.toString().startsWith("MOD_SRC:");
    }
    void itemDragEnter(const SourceDetails&) override { isDragHovered = true; repaint(); }
    void itemDragExit(const SourceDetails&) override { isDragHovered = false; repaint(); }
    void itemDropped(const SourceDetails& d) override {
        isDragHovered = false;
        int src = d.description.toString().fromFirstOccurrenceOf("MOD_SRC:", false, false).getIntValue();
        if (onModDropped && src > 0) onModDropped(src, modDestIndex);
        repaint();
    }
    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isRightButtonDown() && modDestIndex > 0 && onModCleared) { onModCleared(modDestIndex); return; }
        juce::Slider::mouseDown(e);
    }
};

class EnvelopeDisplay : public juce::Component, private juce::Timer {
public:
    EnvelopeDisplay(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix, juce::Colour col, SkinTheme& themeRef);
    void paint(juce::Graphics& g) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void timerCallback() override { repaint(); }
private:
    juce::AudioProcessorValueTreeState& state;
    juce::String pre;
    juce::Colour lineCol;
    SkinTheme& theme;
};

class FilterCurveDisplay : public juce::Component, private juce::Timer {
public:
    FilterCurveDisplay(juce::AudioProcessorValueTreeState& apvts, SkinTheme& themeRef);
    void paint(juce::Graphics& g) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void timerCallback() override { repaint(); }
private:
    juce::AudioProcessorValueTreeState& state;
    SkinTheme& theme;
};

class WaeraAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, private juce::Timer {
public:
    WaeraAudioProcessorEditor(WaeraAudioProcessor&);
    ~WaeraAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
private:
    WaeraAudioProcessor& audioProcessor;
    SerumLookAndFeel serumLAF;
    int activeTab = 0;

    juce::TextButton tabOscBtn { "OSC & ENVS" }, tabFxBtn { "FX RACK" }, tabModBtn { "MOD MATRIX" }, tabArpBtn { "ARP & MACROS" };
    juce::ComboBox bankSelector, presetSelector, skinSelector, oscAWaveBox, oscBWaveBox, oscCWaveBox, oscAWarpBox, oscAEngBox, oscBEngBox, noiseTypeBox, flt1TypeBox, flt2TypeBox, flt2RouteBox;
    juce::ToggleButton oscAOnBtn { "ON" }, oscBOnBtn { "ON" }, oscCOnBtn { "ON" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> oscAOnAtt, oscBOnAtt, oscCOnAtt;

    juce::TextButton prevPresetBtn { "<" }, nextPresetBtn { ">" }, savePresetBtn { "SAVE .WAERA" }, rescanBtn { "RESCAN" }, openFolderBtn { "ASSETS" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oscAWaveAtt, oscBWaveAtt, oscCWaveAtt, oscAWarpAtt, oscAEngAtt, oscBEngAtt, noiseTypeAtt, flt1TypeAtt, flt2TypeAtt, flt2RouteAtt;
    std::unique_ptr<juce::FileChooser> fileChooser;

    WaveformDisplay scopeA, scopeB, scopeC;
    EnvelopeDisplay env1Graph, env2Graph;
    FilterCurveDisplay filterGraph;
    std::array<std::unique_ptr<LFODisplay>, 4> lfoScopes;

    std::vector<std::unique_ptr<ModSourceBadge>> dragBadges;

    struct KnobUnit {
        int tabPage = 0;
        ModTargetKnob slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::vector<std::unique_ptr<KnobUnit>> knobs;

    std::array<juce::ComboBox, 4> lfoShapeBoxes;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>, 4> lfoShapeAtts;
    std::array<juce::ComboBox, 8> modSrcBoxes, modDstBoxes;
    std::array<juce::Slider, 8> modAmtSliders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>, 8> modSrcAtts, modDstAtts;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 8> modAmtAtts;

    juce::ToggleButton arpOnBtn { "ARPEGGIATOR ACTIVE" };
    juce::ComboBox arpModeBox, arpRateBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> arpOnAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> arpModeAtt, arpRateAtt;

    void addKnob(int tab, const juce::String& paramID, const juce::String& name, int colorGroup, int modDest = 0);
    void assignModRoute(int srcIdx, int dstIdx);
    void clearModRoute(int dstIdx);
    void syncModRings();
    void switchTab(int newTab);
    void stepPreset(int delta);
    void refreshWavetableMenus();
    void refreshBanksAndSkins();
    void refreshPresetsForCurrentBank(bool loadFirst = false);
    void applySkinFile(const juce::File& skinFile);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaeraAudioProcessorEditor)
};
