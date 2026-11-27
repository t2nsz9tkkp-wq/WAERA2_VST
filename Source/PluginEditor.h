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
    WaveformDisplay(BlitzAudioProcessor& proc, int oscIndex, const juce::String& onID, const juce::String& waveID, const juce::String& posID, SkinTheme& themeRef);
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
    BlitzAudioProcessor& processor;
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


class StereoMeterDisplay : public juce::Component, private juce::Timer {
public:
    StereoMeterDisplay(BlitzAudioProcessor& proc, SkinTheme& themeRef) : processor(proc), theme(themeRef) { startTimerHz(30); }
    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xff05070b)); g.fillRoundedRectangle(b, 5.0f);
        g.setColour(theme.panelBorder.brighter(0.15f)); g.drawRoundedRectangle(b, 5.0f, 1.0f);

        float rawL = juce::jlimit(0.0f, 1.2f, processor.meterLevelL.load());
        float rawR = juce::jlimit(0.0f, 1.2f, processor.meterLevelR.load());
        // Convert linear amplitude to perceptual dB meter scale (-48 dB to 0 dB)
        auto toMeter = [](float amp) {
            if (amp <= 0.0001f) return 0.0f;
            float db = 20.0f * std::log10(amp);
            return juce::jlimit(0.0f, 1.0f, (db + 48.0f) / 48.0f);
        };
        float mL = toMeter(rawL), mR = toMeter(rawR);
        peakHoldL = (mL >= peakHoldL) ? mL : std::max(0.0f, peakHoldL - 0.015f);
        peakHoldR = (mR >= peakHoldR) ? mR : std::max(0.0f, peakHoldR - 0.015f);
        if (rawL >= 0.98f) clipHoldL = 24; else if (clipHoldL > 0) --clipHoldL;
        if (rawR >= 0.98f) clipHoldR = 24; else if (clipHoldR > 0) --clipHoldR;

        float topY = b.getY() + 9.0f;
        float botY = b.getBottom() - 12.0f;
        float h = botY - topY;
        float colW = 8.0f;
        float xL = b.getX() + 5.0f, xR = b.getRight() - 5.0f - colW;

        // Clip warning LEDs at top
        g.setColour(clipHoldL > 0 ? juce::Colour(0xffff2a4b) : juce::Colours::white.withAlpha(0.08f));
        g.fillRoundedRectangle(xL, b.getY() + 3.0f, colW, 3.5f, 1.0f);
        g.setColour(clipHoldR > 0 ? juce::Colour(0xffff2a4b) : juce::Colours::white.withAlpha(0.08f));
        g.fillRoundedRectangle(xR, b.getY() + 3.0f, colW, 3.5f, 1.0f);

        // Recessed bar wells
        g.setColour(juce::Colours::black.withAlpha(0.65f));
        g.fillRoundedRectangle(xL, topY, colW, h, 2.0f);
        g.fillRoundedRectangle(xR, topY, colW, h, 2.0f);

        // Live gradient level bars
        juce::ColourGradient grad(theme.accentTertiary, 0, botY, theme.accentSecondary, 0, topY, false);
        grad.addColour(0.72, theme.accentPrimary);
        g.setGradientFill(grad);
        if (mL > 0.005f) g.fillRoundedRectangle(xL, botY - mL * h, colW, mL * h, 2.0f);
        if (mR > 0.005f) g.fillRoundedRectangle(xR, botY - mR * h, colW, mR * h, 2.0f);

        // Floating peak-hold ticks
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        if (peakHoldL > 0.02f) g.fillRect(xL, botY - peakHoldL * h, colW, 1.5f);
        if (peakHoldR > 0.02f) g.fillRect(xR, botY - peakHoldR * h, colW, 1.5f);

        // dB scale ticks between bars
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        for (float frac : { 0.25f, 0.5f, 0.75f }) {
            float ty = botY - frac * h;
            g.drawHorizontalLine((int)ty, xL + colW + 1.5f, xR - 1.5f);
        }

        // L / R labels at bottom
        g.setColour(theme.textMain.withAlpha(0.65f));
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.drawText("L", (int)xL - 1, (int)botY + 1, (int)colW + 2, 10, juce::Justification::centred);
        g.drawText("R", (int)xR - 1, (int)botY + 1, (int)colW + 2, 10, juce::Justification::centred);
    }
    void timerCallback() override { repaint(); }
private:
    BlitzAudioProcessor& processor;
    SkinTheme& theme;
    float peakHoldL = 0.0f, peakHoldR = 0.0f;
    int clipHoldL = 0, clipHoldR = 0;
};


class LiveHeaderScope : public juce::Component, private juce::Timer {
public:
    LiveHeaderScope(BlitzAudioProcessor& proc, SkinTheme& themeRef) : processor(proc), theme(themeRef) { startTimerHz(30); }
    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xff05070b)); g.fillRoundedRectangle(b, 5.0f);
        g.setColour(theme.panelBorder.brighter(0.15f)); g.drawRoundedRectangle(b, 5.0f, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawHorizontalLine((int)b.getCentreY(), b.getX() + 3.0f, b.getRight() - 3.0f);

        juce::Path p;
        int startIdx = processor.scopeWriteIdx.load();
        float midY = b.getCentreY();
        for (int i = 0; i < 128; ++i) {
            float smp = juce::jlimit(-1.0f, 1.0f, processor.scopeRing[(size_t)((startIdx + i * 2) & 255)]);
            float x = b.getX() + 4.0f + ((float)i / 127.0f) * (b.getWidth() - 8.0f);
            float y = midY - smp * (b.getHeight() * 0.42f);
            if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
        }
        g.setColour(theme.accentPrimary.withAlpha(0.28f)); g.strokePath(p, juce::PathStrokeType(3.5f));
        g.setColour(theme.accentPrimary); g.strokePath(p, juce::PathStrokeType(1.5f));
    }
    void timerCallback() override { repaint(); }
private:
    BlitzAudioProcessor& processor;
    SkinTheme& theme;
};

class BlitzAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, private juce::Timer {
public:
    BlitzAudioProcessorEditor(BlitzAudioProcessor&);
    ~BlitzAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
private:
    BlitzAudioProcessor& audioProcessor;
    SerumLookAndFeel serumLAF;
    int activeTab = 0;

    juce::TextButton tabOscBtn { "OSC & ENVS" }, tabFxBtn { "FX RACK" }, tabModBtn { "MOD MATRIX" }, tabArpBtn { "ARP & MACROS" };
    juce::ComboBox bankSelector, presetSelector, skinSelector, oscAWaveBox, oscBWaveBox, oscCWaveBox, oscAWarpBox, oscAEngBox, oscBEngBox, noiseTypeBox, flt1TypeBox, flt2TypeBox, flt2RouteBox;
    juce::ToggleButton oscAOnBtn { "ON" }, oscBOnBtn { "ON" }, oscCOnBtn { "ON" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> oscAOnAtt, oscBOnAtt, oscCOnAtt;

    juce::TextButton prevPresetBtn { "<" }, nextPresetBtn { ">" }, savePresetBtn { "SAVE .BLITZ" }, rescanBtn { "RESCAN" }, openFolderBtn { "ASSETS" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oscAWaveAtt, oscBWaveAtt, oscCWaveAtt, oscAWarpAtt, oscAEngAtt, oscBEngAtt, noiseTypeAtt, flt1TypeAtt, flt2TypeAtt, flt2RouteAtt;
    std::unique_ptr<juce::FileChooser> fileChooser;

    WaveformDisplay scopeA, scopeB, scopeC;
    EnvelopeDisplay env1Graph, env2Graph;
    FilterCurveDisplay filterGraph;
    StereoMeterDisplay stereoMeter;
    LiveHeaderScope headerScope;
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BlitzAudioProcessorEditor)
};
