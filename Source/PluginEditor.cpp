#include "PluginEditor.h"

void SerumLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) {
    float side = (float)juce::jmin(width, height) - 6.0f;
    auto bounds = juce::Rectangle<float>((float)x + ((float)width - side) * 0.5f, (float)y + ((float)height - side) * 0.5f - 2.0f, side, side);
    auto radius = side * 0.5f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto centre = bounds.getCentre();
    int group = slider.getProperties()["colorGroup"];
    float modAmt = slider.getProperties()["modAmt"];
    auto* mKnob = dynamic_cast<ModTargetKnob*>(&slider);
    bool hovered = (mKnob != nullptr && mKnob->isDragHovered);
    bool isTarget = (mKnob != nullptr && mKnob->modDestIndex > 0);

    juce::Colour ringCol = (group == 1) ? theme.accentSecondary : (group == 2 ? theme.accentTertiary : theme.accentPrimary);

    // 1. Precision hardware radial tick marks around dial
    for (int t = 0; t <= 10; ++t) {
        float a = rotaryStartAngle + ((float)t / 10.0f) * (rotaryEndAngle - rotaryStartAngle);
        float rIn = radius + 0.5f, rOut = radius + (t % 5 == 0 ? 3.5f : 2.0f);
        g.setColour(juce::Colours::white.withAlpha(t % 5 == 0 ? 0.22f : 0.09f));
        g.drawLine(centre.x + std::sin(a) * rIn, centre.y - std::cos(a) * rIn,
                   centre.x + std::sin(a) * rOut, centre.y - std::cos(a) * rOut, 1.0f);
    }

    // 2. Soft ambient drop shadow
    g.setColour(juce::Colours::black.withAlpha(0.75f));
    g.fillEllipse(bounds.translated(0.0f, 2.5f).reduced(3.0f));

    // 3. Recessed track & multi-layer neon LED value arc
    juce::Path bgArc, valArc, p;
    bgArc.addCentredArc(centre.x, centre.y, radius - 3.5f, radius - 3.5f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(0xff06080c));
    g.strokePath(bgArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    valArc.addCentredArc(centre.x, centre.y, radius - 3.5f, radius - 3.5f, 0.0f, rotaryStartAngle, toAngle, true);
    g.setColour(ringCol.withAlpha(0.18f));
    g.strokePath(valArc, juce::PathStrokeType(8.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(ringCol);
    g.strokePath(valArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 4. Modulation sweep ring
    if (std::abs(modAmt) > 0.005f || hovered) {
        juce::Path modArc;
        float endMod = juce::jlimit(rotaryStartAngle, rotaryEndAngle, toAngle + modAmt * (rotaryEndAngle - rotaryStartAngle));
        if (hovered) endMod = rotaryEndAngle;
        modArc.addCentredArc(centre.x, centre.y, radius - 0.5f, radius - 0.5f, 0.0f, std::min(toAngle, endMod), std::max(toAngle, endMod), true);
        g.setColour(hovered ? juce::Colours::white : theme.accentPrimary);
        g.strokePath(modArc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 5. Machined anodized aluminum cap with specular rim
    auto capBounds = bounds.reduced(7.0f);
    juce::ColourGradient capGrad(theme.panelBg.brighter(0.45f), capBounds.getX(), capBounds.getY(),
                                 juce::Colour(0xff080a0e), capBounds.getRight(), capBounds.getBottom(), false);
    g.setGradientFill(capGrad); g.fillEllipse(capBounds);
    g.setColour(juce::Colours::black.withAlpha(0.95f)); g.drawEllipse(capBounds, 1.5f);
    g.setColour(juce::Colours::white.withAlpha(0.16f)); g.drawEllipse(capBounds.reduced(1.0f), 0.9f);

    if (isTarget) {
        g.setColour(std::abs(modAmt) > 0.005f ? theme.accentPrimary : juce::Colours::white.withAlpha(0.22f));
        g.fillEllipse(bounds.getRight() - 5.0f, bounds.getY(), 4.5f, 4.5f);
    }

    // 6. Glowing indicator needle
    p.addRoundedRectangle(-1.3f, -radius + 8.0f, 2.6f, radius * 0.42f, 1.3f);
    p.applyTransform(juce::AffineTransform::rotation(toAngle).translated(centre.x, centre.y));
    g.setColour(juce::Colours::white.interpolatedWith(ringCol, 0.45f)); g.fillPath(p);
}

void SerumLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float, float, const juce::Slider::SliderStyle, juce::Slider&) {
    float cy = (float)y + (float)height * 0.5f;
    float trackX = (float)x + 6.0f, trackW = (float)width - 12.0f;
    g.setColour(juce::Colour(0xff06080c));
    g.fillRoundedRectangle(trackX, cy - 3.0f, trackW, 6.0f, 3.0f);
    float midX = trackX + trackW * 0.5f;
    g.setColour(theme.accentPrimary);
    g.fillRoundedRectangle(std::min(midX, sliderPos), cy - 3.0f, std::abs(sliderPos - midX), 6.0f, 3.0f);
    g.setColour(juce::Colours::white);
    g.fillEllipse(sliderPos - 5.5f, cy - 5.5f, 11.0f, 11.0f);
}

void SerumLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox&) {
    auto r = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height);
    g.setColour(juce::Colour(0xff080a0f)); g.fillRoundedRectangle(r, 5.0f);
    g.setColour(theme.panelBorder.brighter(0.22f)); g.drawRoundedRectangle(r.reduced(0.5f), 5.0f, 1.0f);
    juce::Path arrow;
    float ax = (float)width - 12.0f, ay = (float)height * 0.5f - 2.0f;
    arrow.addTriangle(ax, ay, ax + 6.0f, ay, ax + 3.0f, ay + 4.0f);
    g.setColour(theme.accentPrimary); g.fillPath(arrow);
}

void SerumLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&, bool isOver, bool isDown) {
    auto r = button.getLocalBounds().toFloat().reduced(0.5f);
    bool activeTabBtn = button.getProperties()["isActiveTab"];
    juce::Colour base = activeTabBtn ? theme.accentPrimary.withAlpha(0.20f) : juce::Colour(0xff0b0e14);
    if (isDown) base = base.brighter(0.22f); else if (isOver) base = base.brighter(0.12f);
    g.setColour(base); g.fillRoundedRectangle(r, 6.0f);
    g.setColour(activeTabBtn ? theme.accentPrimary : theme.panelBorder.brighter(0.16f));
    g.drawRoundedRectangle(r, 6.0f, activeTabBtn ? 1.4f : 1.0f);
}

void SerumLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool, bool) {
    bool on = button.getToggleState();
    auto r = button.getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(on ? theme.accentTertiary.withAlpha(0.22f) : juce::Colour(0xff080a0f));
    g.fillRoundedRectangle(r, 5.0f);
    g.setColour(on ? theme.accentTertiary : theme.panelBorder);
    g.drawRoundedRectangle(r, 5.0f, 1.0f);
    g.setColour(on ? theme.accentTertiary : theme.textMain.withAlpha(0.42f));
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}

WaveformDisplay::WaveformDisplay(BlitzAudioProcessor& proc, int oscIndex, const juce::String& onID, const juce::String& waveID, const juce::String& posID, SkinTheme& themeRef)
    : processor(proc), oscIdx(oscIndex), onParamID(onID), paramID(waveID), wtPosID(posID), theme(themeRef) {
    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    startTimerHz(24);
}

void WaveformDisplay::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat();
    juce::ColourGradient screenGrad(juce::Colour(0xff040609), 0, 0, juce::Colour(0xff090c12), 0, b.getHeight(), false);
    g.setGradientFill(screenGrad); g.fillRoundedRectangle(b, 6.0f);
    g.setColour(juce::Colours::white.withAlpha(0.03f));
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine((int)(b.getHeight() * i * 0.25f), 2.0f, b.getWidth() - 2.0f);
    for (int i = 1; i < 8; ++i) g.drawVerticalLine((int)(b.getWidth() * i / 8.0f), 2.0f, b.getHeight() - 2.0f);
    g.setColour(theme.panelBorder.brighter(0.12f)); g.drawRoundedRectangle(b, 6.0f, 1.0f);

    bool isOn = processor.apvts.getRawParameterValue(onParamID)->load() > 0.5f;
    float midY = b.getCentreY();
    if (!isOn || processor.wavetableBank.tables.empty()) {
        g.setColour(theme.textMain.withAlpha(0.28f));
        g.drawLine(12.0f, midY, b.getWidth() - 12.0f, midY, 2.0f);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText("BYPASSED // DROP .WAV", b.toNearestInt(), juce::Justification::centred);
        return;
    }

    int tblIdx = juce::jlimit(0, (int)processor.wavetableBank.tables.size() - 1, (int)processor.apvts.getRawParameterValue(paramID)->load());
    float wtPos = wtPosID.isNotEmpty() ? processor.apvts.getRawParameterValue(wtPosID)->load() : 0.0f;

    if (wtPosID.isNotEmpty()) {
        for (int slice = 12; slice >= 1; --slice) {
            float slicePos = (float)slice / 12.0f;
            float xOff = (float)slice * 1.9f;
            float yOff = -(float)slice * 2.1f + 13.0f;
            float alpha = 0.03f + (1.0f - std::abs(slicePos - wtPos)) * 0.14f;
            juce::Path sp;
            for (int px = 6; px < getWidth() - 26; px += 3) {
                float phase = std::fmod(((float)(px - 6) / (float)(getWidth() - 32)) * 2.0f, 1.0f);
                float v = processor.wavetableBank.tables[tblIdx].sample(slicePos, phase);
                float py = midY + yOff - v * (b.getHeight() * 0.25f);
                if (px == 6) sp.startNewSubPath((float)px + xOff, py); else sp.lineTo((float)px + xOff, py);
            }
            g.setColour(theme.accentPrimary.withAlpha(alpha));
            g.strokePath(sp, juce::PathStrokeType(1.0f));
        }
    }

    juce::Path wavePath, fillPath;
    fillPath.startNewSubPath(4.0f, midY);
    for (int px = 4; px < getWidth() - 4; ++px) {
        float phase = std::fmod(((float)(px - 4) / (float)(getWidth() - 8)) * 2.0f, 1.0f);
        float v = processor.wavetableBank.tables[tblIdx].sample(wtPos, phase);
        float py = midY - v * (b.getHeight() * 0.36f);
if (px == 4) wavePath.startNewSubPath((float)px, py); else wavePath.lineTo((float)px, py);
fillPath.lineTo((float)px, py);
}
fillPath.lineTo((float)getWidth() - 4.0f, midY); fillPath.closeSubPath();
juce::ColourGradient fillGrad(theme.accentPrimary.withAlpha(0.24f), 0, b.getY(), theme.accentPrimary.withAlpha(0.02f), 0, b.getBottom(), false);
g.setGradientFill(fillGrad); g.fillPath(fillPath);
g.setColour(theme.accentPrimary.withAlpha(0.35f)); g.strokePath(wavePath, juce::PathStrokeType(4.5f));
g.setColour(juce::Colours::white.interpolatedWith(theme.accentPrimary, 0.55f)); g.strokePath(wavePath, juce::PathStrokeType(1.8f));
}
LFODisplay::LFODisplay(juce::AudioProcessorValueTreeState& apvts, int lfoIdx, SkinTheme& themeRef)
: state(apvts), idx(lfoIdx), theme(themeRef) { startTimerHz(30); }
void LFODisplay::paint(juce::Graphics& g) {
auto b = getLocalBounds().toFloat();
g.setColour(juce::Colour(0xff040609)); g.fillRoundedRectangle(b, 6.0f);
g.setColour(theme.panelBorder); g.drawRoundedRectangle(b, 6.0f, 1.0f);
int shape = (int)state.getRawParameterValue("lfo" + juce::String(idx) + "_shape")->load();
auto eval = [&](float p) {
if (shape == 0) return std::sin(p * juce::MathConstants<float>::twoPi);
if (shape == 1) return 4.0f * std::abs(p - 0.5f) - 1.0f;
if (shape == 2) return 2.0f * p - 1.0f;
if (shape == 3) return 1.0f - 2.0f * p;
if (shape == 4) return p < 0.5f ? 0.85f : -0.85f;
return std::sin(std::floor(p * 8.0f) * 3.1f) * 0.8f;
};
juce::Path p;
for (int px = 4; px < getWidth() - 4; ++px) {
float xNorm = (float)(px - 4) / (float)(getWidth() - 8);
float y = b.getCentreY() - eval(xNorm) * (b.getHeight() * 0.36f);
if (px == 4) p.startNewSubPath((float)px, y); else p.lineTo((float)px, y);
}
g.setColour(theme.accentPrimary); g.strokePath(p, juce::PathStrokeType(2.0f));
float dotX = 4.0f + phase * (float)(getWidth() - 8);
float dotY = b.getCentreY() - eval(phase) * (b.getHeight() * 0.36f);
g.setColour(theme.accentSecondary); g.fillEllipse(dotX - 4.0f, dotY - 4.0f, 8.0f, 8.0f);
}
EnvelopeDisplay::EnvelopeDisplay(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix, juce::Colour col, SkinTheme& themeRef)
: state(apvts), pre(prefix), lineCol(col), theme(themeRef) {
setMouseCursor(juce::MouseCursor::CrosshairCursor);
startTimerHz(20);
}
void EnvelopeDisplay::mouseDrag(const juce::MouseEvent& e) {
float xN = juce::jlimit(0.0f, 1.0f, (float)e.x / (float)getWidth());
float yN = juce::jlimit(0.0f, 1.0f, 1.0f - (float)e.y / (float)getHeight());
if (auto* pd = state.getParameter(pre.isEmpty() ? "dec" : pre + "_dec")) pd->setValueNotifyingHost(xN);
if (auto* ps = state.getParameter(pre.isEmpty() ? "sus" : pre + "_sus")) ps->setValueNotifyingHost(yN);
}
void EnvelopeDisplay::paint(juce::Graphics& g) {
auto b = getLocalBounds().toFloat();
g.setColour(juce::Colour(0xff040609)); g.fillRoundedRectangle(b, 6.0f);
g.setColour(juce::Colours::white.withAlpha(0.03f));
g.drawHorizontalLine((int)b.getCentreY(), 2.0f, b.getWidth() - 2.0f);
g.setColour(theme.panelBorder); g.drawRoundedRectangle(b, 6.0f, 1.0f);
juce::Colour activeCol = pre.isEmpty() ? theme.accentTertiary : theme.accentPrimary;
float a = state.getRawParameterValue(pre.isEmpty() ? "att" : pre + "_att")->load();
float d = state.getRawParameterValue(pre.isEmpty() ? "dec" : pre + "_dec")->load();
float s = state.getRawParameterValue(pre.isEmpty() ? "sus" : pre + "_sus")->load();
float r = state.getRawParameterValue(pre.isEmpty() ? "rel" : pre + "_rel")->load();
float pad = 8.0f, w = b.getWidth() - pad * 2.0f, h = b.getHeight() - pad * 2.0f;
float x0 = pad, y0 = b.getBottom() - pad;
float xA = x0 + juce::jlimit(0.04f, 0.30f, a * 0.6f) * w;
float xD = xA + juce::jlimit(0.06f, 0.35f, d * 0.25f) * w;
float yS = y0 - s * h, xS = xD + 0.18f * w;
float xR = std::min(b.getRight() - pad, xS + juce::jlimit(0.05f, 0.25f, r * 0.2f) * w);
juce::Path p, fill;
p.startNewSubPath(x0, y0); p.lineTo(xA, pad); p.lineTo(xD, yS); p.lineTo(xS, yS); p.lineTo(xR, y0);
fill = p; fill.lineTo(x0, y0); fill.closeSubPath();
g.setColour(activeCol.withAlpha(0.16f)); g.fillPath(fill);
g.setColour(activeCol); g.strokePath(p, juce::PathStrokeType(2.0f));
for (auto pt : { juce::Point<float>(xA, pad), {xD, yS}, {xS, yS} }) {
g.setColour(juce::Colours::white); g.fillEllipse(pt.x - 3.5f, pt.y - 3.5f, 7.0f, 7.0f);
g.setColour(activeCol); g.drawEllipse(pt.x - 3.5f, pt.y - 3.5f, 7.0f, 7.0f, 1.5f);
}
}
FilterCurveDisplay::FilterCurveDisplay(juce::AudioProcessorValueTreeState& apvts, SkinTheme& themeRef)
: state(apvts), theme(themeRef) {
setMouseCursor(juce::MouseCursor::CrosshairCursor);
startTimerHz(20);
}
void FilterCurveDisplay::mouseDrag(const juce::MouseEvent& e) {
float xN = juce::jlimit(0.0f, 1.0f, (float)e.x / (float)getWidth());
float yN = juce::jlimit(0.0f, 1.0f, 1.0f - (float)e.y / (float)getHeight());
if (auto* pc = state.getParameter("cutoff")) pc->setValueNotifyingHost(xN);
if (auto* pr = state.getParameter("reso")) pr->setValueNotifyingHost(yN);
}
void FilterCurveDisplay::paint(juce::Graphics& g) {
auto b = getLocalBounds().toFloat();
g.setColour(juce::Colour(0xff040609)); g.fillRoundedRectangle(b, 6.0f);
g.setColour(theme.panelBorder); g.drawRoundedRectangle(b, 6.0f, 1.0f);
float cutNorm = std::log10(juce::jlimit(80.0f, 16000.0f, state.getRawParameterValue("cutoff")->load()) / 80.0f) / std::log10(200.0f);
float resNorm = (state.getRawParameterValue("reso")->load() - 0.5f) / 11.5f;
int type = (int)state.getRawParameterValue("flt1_type")->load();
juce::Path p, fill; float baseH = b.getHeight() * 0.45f, midY = b.getHeight() * 0.58f;
fill.startNewSubPath(2.0f, b.getBottom() - 2.0f);
for (int px = 2; px < getWidth() - 2; ++px) {
float xNorm = (float)px / (float)getWidth();
float diff = (type == 1) ? (cutNorm - xNorm) : (xNorm - cutNorm);
float slope = (type == 2) ? -std::abs(xNorm - cutNorm) * 14.0f : (diff > 0.0f ? -diff * 16.0f : 0.0f);
float peak = resNorm * 0.42f * std::exp(-std::pow((xNorm - cutNorm) * 14.0f, 2.0f));
float y = juce::jlimit(6.0f, b.getHeight() - 4.0f, midY - (slope * 0.18f + peak) * baseH);
if (px == 2) p.startNewSubPath((float)px, y); else p.lineTo((float)px, y);
fill.lineTo((float)px, y);
}
fill.lineTo((float)getWidth() - 2.0f, b.getBottom() - 2.0f); fill.closeSubPath();
g.setColour(theme.accentSecondary.withAlpha(0.20f)); g.fillPath(fill);
g.setColour(theme.accentSecondary); g.strokePath(p, juce::PathStrokeType(2.0f));
}
BlitzAudioProcessorEditor::BlitzAudioProcessorEditor(BlitzAudioProcessor& p)
: AudioProcessorEditor(&p), audioProcessor(p),
scopeA(p, 0, "oscA_on", "oscA_wave", "oscA_wtPos", serumLAF.theme),
scopeB(p, 1, "oscB_on", "oscB_wave", "oscB_wtPos", serumLAF.theme),
scopeC(p, 2, "oscC_on", "oscC_wave", "", serumLAF.theme),
env1Graph(p.apvts, "", serumLAF.theme.accentTertiary, serumLAF.theme),
env2Graph(p.apvts, "env2", serumLAF.theme.accentPrimary, serumLAF.theme),
filterGraph(p.apvts, serumLAF.theme), stereoMeter(p, serumLAF.theme), headerScope(p, serumLAF.theme) {
setLookAndFeel(&serumLAF);
serumLAF.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
serumLAF.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
for (auto* b : { &tabOscBtn, &tabFxBtn, &tabModBtn, &tabArpBtn }) addAndMakeVisible(b);
tabOscBtn.onClick = [this] { switchTab(0); }; tabFxBtn.onClick = [this] { switchTab(1); };
tabModBtn.onClick = [this] { switchTab(2); }; tabArpBtn.onClick = [this] { switchTab(3); };
addAndMakeVisible(bankSelector); addAndMakeVisible(prevPresetBtn); addAndMakeVisible(presetSelector); addAndMakeVisible(nextPresetBtn);
addAndMakeVisible(savePresetBtn); addAndMakeVisible(rescanBtn); addAndMakeVisible(skinSelector); addAndMakeVisible(openFolderBtn);
bankSelector.onChange = [this] { audioProcessor.currentBankName = bankSelector.getText(); refreshPresetsForCurrentBank(true); };
prevPresetBtn.onClick = [this] { stepPreset(-1); }; nextPresetBtn.onClick = [this] { stepPreset(1); };
presetSelector.onChange = [this] {
audioProcessor.currentBankName = bankSelector.getText();
audioProcessor.currentPresetName = presetSelector.getText();
auto file = audioProcessor.getRootUserDirectory().getChildFile("Banks").getChildFile(audioProcessor.currentBankName).getChildFile(audioProcessor.currentPresetName + ".blitz");
if (file.existsAsFile()) { audioProcessor.loadPresetFromFile(file); syncModRings(); }
audioProcessor.getRootUserDirectory().getChildFile("last_session.txt").replaceWithText(audioProcessor.currentBankName + juce::String::charToString(10) + audioProcessor.currentPresetName + juce::String::charToString(10) + audioProcessor.currentSkinName);
};
skinSelector.onChange = [this] {
audioProcessor.currentSkinName = skinSelector.getText();
auto file = audioProcessor.getRootUserDirectory().getChildFile("Skins").getChildFile(audioProcessor.currentSkinName + ".json");
if (file.existsAsFile()) applySkinFile(file);
audioProcessor.getRootUserDirectory().getChildFile("last_session.txt").replaceWithText(audioProcessor.currentBankName + juce::String::charToString(10) + audioProcessor.currentPresetName + juce::String::charToString(10) + audioProcessor.currentSkinName);
};
savePresetBtn.onClick = [this] {
auto userDir = audioProcessor.getRootUserDirectory().getChildFile("Banks").getChildFile("User Custom");
fileChooser = std::make_unique<juce::FileChooser>("Save Blitz-NPC Preset (.blitz)", userDir.getChildFile("My Patch.blitz"), "*.blitz");
fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
[this](const juce::FileChooser& fc) {
if (fc.getResult() != juce::File()) {
if (auto xml = audioProcessor.apvts.copyState().createXml()) xml->writeTo(fc.getResult().withFileExtension(".blitz"));
refreshBanksAndSkins();
}
});
};
rescanBtn.onClick = [this] { audioProcessor.reloadWavetables(); refreshWavetableMenus(); refreshBanksAndSkins(); };
openFolderBtn.onClick = [this] { audioProcessor.getRootUserDirectory().revealToUser(); };
addAndMakeVisible(oscAOnBtn); addAndMakeVisible(oscBOnBtn); addAndMakeVisible(oscCOnBtn);
oscAOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "oscA_on", oscAOnBtn);
oscBOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "oscB_on", oscBOnBtn);
oscCOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "oscC_on", oscCOnBtn);
scopeA.onSampleDropped = [this] { refreshWavetableMenus(); };
scopeB.onSampleDropped = [this] { refreshWavetableMenus(); };
scopeC.onSampleDropped = [this] { refreshWavetableMenus(); };
for (auto* box : { &oscAWaveBox, &oscBWaveBox, &oscCWaveBox }) addAndMakeVisible(box);
refreshWavetableMenus();
oscAWaveAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "oscA_wave", oscAWaveBox);
oscBWaveAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "oscB_wave", oscBWaveBox);
oscCWaveAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "oscC_wave", oscCWaveBox);
oscAEngBox.addItemList({"WT","Granular","Sample","Spectral"}, 1); addAndMakeVisible(oscAEngBox);
oscAEngAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "oscA_engMode", oscAEngBox);
oscBEngBox.addItemList({"WT","Granular","Sample","Spectral"}, 1); addAndMakeVisible(oscBEngBox);
oscBEngAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "oscB_engMode", oscBEngBox);
noiseTypeBox.addItemList({"White","Pink","Vinyl","Hammer"}, 1); addAndMakeVisible(noiseTypeBox);
noiseTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "noise_type", noiseTypeBox);
oscAWarpBox.addItemList({ "Warp: Off", "Warp: FM (Osc B)", "Warp: Hard Sync", "Warp: PWM Bend" }, 1); addAndMakeVisible(oscAWarpBox);
oscAWarpAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "oscA_warpMode", oscAWarpBox);
flt1TypeBox.addItemList({ "LP 24", "HP 24", "BP 12", "Comb", "Formant" }, 1); addAndMakeVisible(flt1TypeBox);
flt1TypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "flt1_type", flt1TypeBox);
flt2TypeBox.addItemList({ "LP 24", "HP 24", "BP 12", "Comb", "Formant" }, 1); addAndMakeVisible(flt2TypeBox);
flt2TypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "flt2_type", flt2TypeBox);
flt2RouteBox.addItemList({ "Flt 2: Off", "Flt 2: Series", "Flt 2: Parallel" }, 1); addAndMakeVisible(flt2RouteBox);
flt2RouteAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "flt2_route", flt2RouteBox);
addAndMakeVisible(scopeA); addAndMakeVisible(scopeB); addAndMakeVisible(scopeC);
addAndMakeVisible(env1Graph); addAndMakeVisible(env2Graph); addAndMakeVisible(filterGraph); addAndMakeVisible(stereoMeter); addAndMakeVisible(headerScope);
const char* bNames[] = { "LFO 1", "LFO 2", "LFO 3", "LFO 4", "ENV 2", "M1", "M2", "M3" };
int bSrcs[] = { 1, 2, 3, 4, 5, 7, 8, 9 };
for (int i = 0; i < 8; ++i) {
auto b = std::make_unique<ModSourceBadge>(bNames[i], bSrcs[i], i < 4 ? serumLAF.theme.accentPrimary : serumLAF.theme.accentTertiary);
addAndMakeVisible(*b);
dragBadges.push_back(std::move(b));
}
addKnob(0, "oscA_wtPos", "WT POS", 0, 1); addKnob(0, "oscA_unison", "UNISON", 0); addKnob(0, "oscA_detune", "DETUNE", 0);
addKnob(0, "oscA_warpAmt", "WARP", 1, 3); addKnob(0, "oscA_oct", "OCTAVE", 0); addKnob(0, "oscA_lvl", "LEVEL", 0);
addKnob(0, "oscB_wtPos", "WT POS", 0, 2); addKnob(0, "oscB_oct", "OCTAVE", 0); addKnob(0, "oscB_semi", "SEMI", 0); addKnob(0, "oscB_lvl", "LEVEL", 0);
addKnob(0, "oscC_oct", "OCTAVE", 2); addKnob(0, "glide", "GLIDE", 2); addKnob(0, "oscC_lvl", "SUB LVL", 2);
addKnob(0, "cutoff", "CUT 1", 1, 4); addKnob(0, "reso", "RES 1", 1); addKnob(0, "envAmt", "ENV MOD", 1);
addKnob(0, "flt2_cutoff", "CUT 2", 1, 5); addKnob(0, "flt2_reso", "RES 2", 1); addKnob(0, "flt1_drive", "DRIVE", 1);
addKnob(0, "att", "ATTACK", 2); addKnob(0, "dec", "DECAY", 2); addKnob(0, "sus", "SUSTAIN", 2); addKnob(0, "rel", "RELEASE", 2);
addKnob(0, "env2_att", "ATTACK", 0); addKnob(0, "env2_dec", "DECAY", 0); addKnob(0, "env2_sus", "SUSTAIN", 0); addKnob(0, "env2_rel", "RELEASE", 0);
addKnob(0, "pitchEnv", "ZAP PITCH", 0, 6); addKnob(0, "master", "MASTER", 2); addKnob(0, "noise_lvl", "NOISE LVL", 2);
addKnob(1, "crush_bits", "BIT DEPTH", 1); addKnob(1, "crush_down", "DOWNSAMPLE", 1);
addKnob(1, "diode", "DIODE CLASH", 1, 7);
addKnob(1, "chorus_mix", "HYPER MIX", 0); addKnob(1, "chorus_rate", "HYPER RATE", 0);
addKnob(1, "delayMix", "BOUNCE DLY", 0);
addKnob(1, "reverb_mix", "REVERB MIX", 2); addKnob(1, "reverb_size", "ROOM SIZE", 2);
addKnob(1, "ott_amt", "OTT PUNCH", 1); addKnob(1, "eq_low", "EQ LOW", 0); addKnob(1, "eq_mid", "EQ MID", 0); addKnob(1, "eq_high", "EQ HIGH", 0);
juce::StringArray shapes { "Sine", "Triangle", "Ramp Up", "Saw Down", "Square", "S&H Rand" };
for (int i = 0; i < 4; ++i) {
lfoScopes[i] = std::make_unique<LFODisplay>(audioProcessor.apvts, i + 1, serumLAF.theme);
addChildComponent(*lfoScopes[i]);
lfoShapeBoxes[i].addItemList(shapes, 1); addChildComponent(lfoShapeBoxes[i]);
lfoShapeAtts[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "lfo" + juce::String(i + 1) + "_shape", lfoShapeBoxes[i]);
addKnob(2, "lfo" + juce::String(i + 1) + "_rate", "RATE (HZ)", 0);
}
juce::StringArray modSrcs { "Off", "LFO 1", "LFO 2", "LFO 3", "LFO 4", "ENV 2", "ENV 3", "Macro 1", "Macro 2", "Macro 3", "Macro 4", "Velocity" };
juce::StringArray modDsts { "Off", "Osc A WT Pos", "Osc B WT Pos", "Osc A Warp", "Filter 1 Cutoff", "Filter 2 Cutoff", "Master Pitch", "Diode Drive" };
for (int m = 0; m < 8; ++m) {
juce::String pre = "mod" + juce::String(m + 1);
modSrcBoxes[m].addItemList(modSrcs, 1); addChildComponent(modSrcBoxes[m]);
modDstBoxes[m].addItemList(modDsts, 1); addChildComponent(modDstBoxes[m]);
modAmtSliders[m].setSliderStyle(juce::Slider::LinearHorizontal); modAmtSliders[m].setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 20);
addChildComponent(modAmtSliders[m]);
modSrcAtts[m] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, pre + "_src", modSrcBoxes[m]);
modDstAtts[m] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, pre + "_dst", modDstBoxes[m]);
modAmtAtts[m] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, pre + "_amt", modAmtSliders[m]);
}
addChildComponent(arpOnBtn); arpOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "arp_on", arpOnBtn);
arpModeBox.addItemList({ "Up", "Down", "Up/Down", "Random" }, 1); addChildComponent(arpModeBox);
arpModeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "arp_mode", arpModeBox);
arpRateBox.addItemList({ "1/8", "1/16", "1/32" }, 1); addChildComponent(arpRateBox);
arpRateAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "arp_rate", arpRateBox);
addKnob(3, "arp_gate", "ARP GATE", 1);
for (int i = 1; i <= 4; ++i) addKnob(3, "macro" + juce::String(i), "MACRO " + juce::String(i), 2);
refreshBanksAndSkins();
syncModRings();
switchTab(0);
startTimerHz(15);
setSize(1280, 740);
}
BlitzAudioProcessorEditor::~BlitzAudioProcessorEditor() { stopTimer(); setLookAndFeel(nullptr); }
void BlitzAudioProcessorEditor::timerCallback() { syncModRings(); }
void BlitzAudioProcessorEditor::syncModRings() {
std::array<float, 8> dstTotals {};
for (int m = 0; m < 8; ++m) {
int src = modSrcBoxes[m].getSelectedId() - 1;
int dst = modDstBoxes[m].getSelectedId() - 1;
float amt = (float)modAmtSliders[m].getValue();
if (src > 0 && dst > 0 && dst < 8) dstTotals[dst] += amt;
}
for (auto& k : knobs) {
int d = k->slider.modDestIndex;
float val = (d > 0 && d < 8) ? dstTotals[d] : 0.0f;
if ((float)k->slider.getProperties()["modAmt"] != val) {
k->slider.getProperties().set("modAmt", val);
k->slider.repaint();
}
}
}
void BlitzAudioProcessorEditor::assignModRoute(int srcIdx, int dstIdx) {
int targetSlot = -1;
for (int m = 0; m < 8; ++m) {
int s = modSrcBoxes[m].getSelectedId() - 1;
int d = modDstBoxes[m].getSelectedId() - 1;
if (s == srcIdx && d == dstIdx) { targetSlot = m; break; }
if (targetSlot == -1 && (s <= 0 || d <= 0)) targetSlot = m;
}
if (targetSlot >= 0) {
modSrcBoxes[targetSlot].setSelectedId(srcIdx + 1, juce::sendNotificationSync);
modDstBoxes[targetSlot].setSelectedId(dstIdx + 1, juce::sendNotificationSync);
modAmtSliders[targetSlot].setValue(0.55, juce::sendNotificationSync);
}
syncModRings();
}
void BlitzAudioProcessorEditor::clearModRoute(int dstIdx) {
for (int m = 0; m < 8; ++m) {
if (modDstBoxes[m].getSelectedId() - 1 == dstIdx) {
modSrcBoxes[m].setSelectedId(1, juce::sendNotificationSync);
modDstBoxes[m].setSelectedId(1, juce::sendNotificationSync);
modAmtSliders[m].setValue(0.0, juce::sendNotificationSync);
}
}
syncModRings();
}
void BlitzAudioProcessorEditor::switchTab(int newTab) {
activeTab = newTab;
tabOscBtn.getProperties().set("isActiveTab", activeTab == 0);
tabFxBtn.getProperties().set("isActiveTab", activeTab == 1);
tabModBtn.getProperties().set("isActiveTab", activeTab == 2);
tabArpBtn.getProperties().set("isActiveTab", activeTab == 3);
bool t0 = (activeTab == 0), t2 = (activeTab == 2), t3 = (activeTab == 3);
for (auto* c : std::array<juce::Component*, 19>{ &oscAEngBox, &oscBEngBox, &noiseTypeBox, &oscAOnBtn, &oscBOnBtn, &oscCOnBtn, &oscAWaveBox, &oscBWaveBox, &oscCWaveBox, &oscAWarpBox, &flt1TypeBox, &flt2TypeBox, &flt2RouteBox, &scopeA, &scopeB, &scopeC, &env1Graph, &env2Graph, &filterGraph })
c->setVisible(t0);
for (int i = 0; i < 4; ++i) { lfoShapeBoxes[i].setVisible(t2); lfoScopes[i]->setVisible(t2); }
for (int m = 0; m < 8; ++m) { modSrcBoxes[m].setVisible(t2); modDstBoxes[m].setVisible(t2); modAmtSliders[m].setVisible(t2); }
arpOnBtn.setVisible(t3); arpModeBox.setVisible(t3); arpRateBox.setVisible(t3);
for (auto& k : knobs) { bool vis = (k->tabPage == activeTab); k->slider.setVisible(vis); k->label.setVisible(vis); }
repaint();
}
void BlitzAudioProcessorEditor::refreshWavetableMenus() {
juce::StringArray names;
for (auto& t : audioProcessor.wavetableBank.tables) names.add(t.name);
for (auto* box : { &oscAWaveBox, &oscBWaveBox, &oscCWaveBox }) {
int sel = box->getSelectedId(); box->clear(juce::dontSendNotification); box->addItemList(names, 1);
box->setSelectedId(sel > 0 ? sel : 1, juce::dontSendNotification);
}
}
void BlitzAudioProcessorEditor::stepPreset(int delta) {
int count = presetSelector.getNumItems();
if (count > 0) presetSelector.setSelectedItemIndex((presetSelector.getSelectedItemIndex() + delta + count) % count, juce::sendNotification);
}
void BlitzAudioProcessorEditor::addKnob(int tab, const juce::String& paramID, const juce::String& name, int colorGroup, int modDest) {
auto k = std::make_unique<KnobUnit>();
k->tabPage = tab; k->slider.modDestIndex = modDest;
k->slider.onModDropped = [this](int s, int d) { assignModRoute(s, d); };
k->slider.onModCleared = [this](int d) { clearModRoute(d); };
k->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
k->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 14);
    k->slider.setNumDecimalPlacesToDisplay(2);
k->slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
k->slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
k->slider.getProperties().set("colorGroup", colorGroup);
addChildComponent(k->slider);
k->label.setText(name, juce::dontSendNotification);
k->label.setJustificationType(juce::Justification::centred);
k->label.setFont(juce::FontOptions(9.5f, juce::Font::bold));
k->label.setColour(juce::Label::textColourId, serumLAF.theme.textMain.withAlpha(0.78f));
addChildComponent(k->label);
k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, paramID, k->slider);
knobs.push_back(std::move(k));
}
void BlitzAudioProcessorEditor::refreshBanksAndSkins() {
auto root = audioProcessor.getRootUserDirectory();
auto cfg = root.getChildFile("last_session.txt");
if (cfg.existsAsFile()) {
juce::StringArray lines;
lines.addLines(cfg.loadFileAsString());
        if (lines.size() >= 1 && lines[0].trim().isNotEmpty() && audioProcessor.currentPresetName.isEmpty()) audioProcessor.currentBankName = lines[0].trim();
        if (lines.size() >= 2 && lines[1].trim().isNotEmpty() && audioProcessor.currentPresetName.isEmpty()) audioProcessor.currentPresetName = lines[1].trim();
        if (lines.size() >= 3 && lines[2].trim().isNotEmpty()) audioProcessor.currentSkinName = lines[2].trim();
}
bankSelector.clear(juce::dontSendNotification);
auto banks = root.getChildFile("Banks").findChildFiles(juce::File::findDirectories, false); banks.sort();
int idx = 1, matchBank = 1;
for (auto& dir : banks) { if (dir.getFileName() == audioProcessor.currentBankName) matchBank = idx; bankSelector.addItem(dir.getFileName(), idx++); }
if (bankSelector.getNumItems() > 0) { bankSelector.setSelectedId(matchBank, juce::dontSendNotification); audioProcessor.currentBankName = bankSelector.getText(); refreshPresetsForCurrentBank(false); }
skinSelector.clear(juce::dontSendNotification);
auto skins = root.getChildFile("Skins").findChildFiles(juce::File::findFiles, false, "*.json"); skins.sort();
idx = 1; int matchSkin = 1;
for (auto& sk : skins) { if (sk.getFileNameWithoutExtension() == audioProcessor.currentSkinName) matchSkin = idx; skinSelector.addItem(sk.getFileNameWithoutExtension(), idx++); }
if (skinSelector.getNumItems() > 0) {
skinSelector.setSelectedId(matchSkin, juce::dontSendNotification);
auto skFile = root.getChildFile("Skins").getChildFile(skinSelector.getText() + ".json");
if (skFile.existsAsFile()) applySkinFile(skFile);
}
}
void BlitzAudioProcessorEditor::refreshPresetsForCurrentBank(bool loadFirst) {
auto prevPreset = loadFirst ? juce::String() : audioProcessor.currentPresetName;
auto bankDir = audioProcessor.getRootUserDirectory().getChildFile("Banks").getChildFile(bankSelector.getText());
presetSelector.clear(juce::dontSendNotification);
auto files = bankDir.findChildFiles(juce::File::findFiles, false, "*.blitz"); files.sort();
int idx = 1, matchIdx = 1;
for (auto& f : files) { if (f.getFileNameWithoutExtension() == prevPreset) matchIdx = idx; presetSelector.addItem(f.getFileNameWithoutExtension(), idx++); }
if (presetSelector.getNumItems() > 0) { presetSelector.setSelectedId(matchIdx, loadFirst ? juce::sendNotificationSync : juce::dontSendNotification); audioProcessor.currentPresetName = presetSelector.getText(); }
}
void BlitzAudioProcessorEditor::applySkinFile(const juce::File& skinFile) {
auto parsed = juce::JSON::parse(skinFile.loadFileAsString());
if (auto* obj = parsed.getDynamicObject()) {
auto readCol = [&](const juce::String& k, juce::Colour fb) {
if (!obj->hasProperty(k)) return fb;
auto hex = obj->getProperty(k).toString().trim().trimCharactersAtStart("#");
if (hex.length() == 6) hex = "ff" + hex;
return juce::Colour::fromString(hex);
};
serumLAF.theme.bgDark = readCol("bgDark", serumLAF.theme.bgDark); serumLAF.theme.panelBg = readCol("panelBg", serumLAF.theme.panelBg);
serumLAF.theme.panelBorder = readCol("panelBorder", serumLAF.theme.panelBorder); serumLAF.theme.accentPrimary = readCol("accentPrimary", serumLAF.theme.accentPrimary);
serumLAF.theme.accentSecondary = readCol("accentSecondary", serumLAF.theme.accentSecondary); serumLAF.theme.accentTertiary = readCol("accentTertiary", serumLAF.theme.accentTertiary);
serumLAF.theme.textMain = readCol("textMain", serumLAF.theme.textMain);
serumLAF.setColour(juce::ComboBox::textColourId, serumLAF.theme.textMain);
serumLAF.setColour(juce::TextButton::textColourOffId, serumLAF.theme.textMain);
serumLAF.setColour(juce::Label::textColourId, serumLAF.theme.textMain);
serumLAF.setColour(juce::Slider::textBoxTextColourId, serumLAF.theme.textMain);
sendLookAndFeelChange();
repaint();
}
}
void BlitzAudioProcessorEditor::paint(juce::Graphics& g) {
auto& t = serumLAF.theme;
g.fillAll(t.bgDark);
// Top studio header bar
juce::ColourGradient hdrGrad(t.panelBg.brighter(0.18f), 0.0f, 0.0f, t.bgDark.darker(0.35f), 0.0f, 88.0f, false);
g.setGradientFill(hdrGrad); g.fillRect(0, 0, getWidth(), 88);
g.setColour(t.panelBorder); g.drawHorizontalLine(88, 0.0f, (float)getWidth());
// Brand Logo + Subtitle Badge
g.setColour(t.accentPrimary); g.setFont(juce::FontOptions(21.0f, juce::Font::bold));
g.drawText("BLITZ-NPC", 18, 8, 150, 24, juce::Justification::centredLeft);
g.setColour(t.textMain.withAlpha(0.45f)); g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
g.drawText("PRO SPECTRAL & WAVETABLE SYNTH", 18, 29, 152, 14, juce::Justification::centredLeft);
// Clean Mod Drag Label aligned left of LFO badges
g.setColour(t.textMain.withAlpha(0.55f)); g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
g.drawText("MOD SOURCES:", 655, 51, 95, 22, juce::Justification::centredRight);
auto drawPanel = [&](juce::Rectangle<int> r, const juce::String& title, juce::Colour badgeCol, int knobWellY = 0) {
juce::ColourGradient cardGrad(t.panelBg.brighter(0.05f), (float)r.getX(), (float)r.getY(),
t.panelBg.darker(0.15f), (float)r.getX(), (float)r.getBottom(), false);
g.setGradientFill(cardGrad); g.fillRoundedRectangle(r.toFloat(), 8.0f);
g.setColour(t.panelBorder); g.drawRoundedRectangle(r.toFloat(), 8.0f, 1.2f);
g.setColour(juce::Colours::white.withAlpha(0.07f));
g.drawHorizontalLine(r.getY() + 1, (float)r.getX() + 8.0f, (float)r.getRight() - 8.0f);
auto hdr = r.removeFromTop(28);
g.setColour(juce::Colours::black.withAlpha(0.35f)); g.fillRoundedRectangle(hdr.toFloat(), 7.0f);
g.setColour(badgeCol); g.fillEllipse((float)hdr.getX() + 10.0f, (float)hdr.getCentreY() - 3.5f, 7.0f, 7.0f);
g.setColour(t.textMain.withAlpha(0.92f)); g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
g.drawText(title, hdr.getX() + 23, hdr.getY(), hdr.getWidth() - 75, 28, juce::Justification::centredLeft);
if (knobWellY > 0) {
auto well = juce::Rectangle<float>((float)r.getX() + 8.0f, (float)knobWellY, (float)r.getWidth() - 16.0f, (float)(r.getBottom() - knobWellY - 8));
g.setColour(juce::Colours::black.withAlpha(0.24f)); g.fillRoundedRectangle(well, 6.0f);
g.setColour(juce::Colours::white.withAlpha(0.03f)); g.drawRoundedRectangle(well, 6.0f, 1.0f);
}
};
if (activeTab == 0) {
drawPanel({ 14, 98, 368, 376 }, "OSC A // 3D WAVETABLE & SPECTRAL", t.accentPrimary, 282);
drawPanel({ 394, 98, 290, 376 }, "OSC B // HARMONIC LAYER", t.accentPrimary, 282);
drawPanel({ 696, 98, 256, 376 }, "OSC C // SUB & GLIDE", t.accentTertiary, 282);
drawPanel({ 964, 98, 302, 376 }, "DUAL MULTIMODE FILTERS", t.accentSecondary, 224);
drawPanel({ 14, 486, 502, 240 }, "ENV 1 // AMP & FILTER 1", t.accentTertiary, 612);
drawPanel({ 528, 486, 502, 240 }, "ENV 2 // MODULATION", t.accentPrimary, 612);
drawPanel({ 1042, 486, 224, 240 }, "NOISE & MASTER", t.accentSecondary, 560);
} else if (activeTab == 1) {
drawPanel({ 14, 98, 404, 304 }, "1. BITCRUSHER & DOWNSAMPLER", t.accentSecondary, 180);
drawPanel({ 434, 98, 412, 304 }, "2. DIODE CLASH DISTORTION", t.accentSecondary, 180);
drawPanel({ 862, 98, 404, 304 }, "3. HYPER STEREO CHORUS", t.accentPrimary, 180);
drawPanel({ 14, 418, 404, 308 }, "4. STEREO BOUNCE DELAY", t.accentPrimary, 500);
drawPanel({ 434, 418, 412, 308 }, "5. HALL REVERB", t.accentTertiary, 500);
drawPanel({ 862, 418, 404, 308 }, "6. OTT PUNCH & MASTER EQ", t.accentSecondary, 500);
} else if (activeTab == 2) {
drawPanel({ 14, 98, 490, 628 }, "LFO 1 - 4 // SHAPE & RATE", t.accentPrimary, 0);
drawPanel({ 520, 98, 746, 628 }, "8-SLOT MODULATION MATRIX (SOURCE -> DESTINATION)", t.accentSecondary, 0);
} else {
drawPanel({ 14, 98, 616, 628 }, "16-STEP DIGICORE ARPEGGIATOR", t.accentSecondary, 0);
drawPanel({ 646, 98, 620, 628 }, "PERFORMANCE MACROS (1 - 4)", t.accentTertiary, 0);
}
}
void BlitzAudioProcessorEditor::resized() {
headerScope.setBounds(172, 10, 72, 30);
    bankSelector.setBounds(252, 12, 195, 26); prevPresetBtn.setBounds(453, 12, 26, 26); presetSelector.setBounds(482, 12, 245, 26); nextPresetBtn.setBounds(730, 12, 26, 26);
savePresetBtn.setBounds(764, 12, 106, 26); rescanBtn.setBounds(876, 12, 82, 26); skinSelector.setBounds(964, 12, 190, 26); openFolderBtn.setBounds(1154, 12, 74, 26);
stereoMeter.setBounds(1234, 8, 34, 72);
tabOscBtn.setBounds(16, 48, 132, 30); tabFxBtn.setBounds(154, 48, 132, 30); tabModBtn.setBounds(292, 48, 132, 30); tabArpBtn.setBounds(430, 48, 132, 30);
for (int i = 0; i < (int)dragBadges.size(); ++i) dragBadges[i]->setBounds(754 + i * 59, 50, 52, 24);
oscAOnBtn.setBounds(330, 101, 44, 22); oscBOnBtn.setBounds(632, 101, 44, 22); oscCOnBtn.setBounds(900, 101, 44, 22);
oscAEngBox.setBounds(26, 132, 82, 24); oscAWaveBox.setBounds(112, 132, 130, 24); oscAWarpBox.setBounds(246, 132, 124, 24);
oscBEngBox.setBounds(406, 132, 84, 24); oscBWaveBox.setBounds(494, 132, 178, 24);
oscCWaveBox.setBounds(708, 132, 232, 24);
flt1TypeBox.setBounds(1168, 132, 86, 24); flt2RouteBox.setBounds(976, 336, 142, 24); flt2TypeBox.setBounds(1124, 336, 130, 24);
scopeA.setBounds(26, 162, 344, 114); scopeB.setBounds(406, 162, 266, 114); scopeC.setBounds(708, 162, 232, 114);
filterGraph.setBounds(976, 132, 186, 86); env1Graph.setBounds(26, 522, 478, 86); env2Graph.setBounds(540, 522, 478, 86);
auto placeKnob = [&](int kIdx, int x, int y, int w = 86) {
if (kIdx >= 0 && kIdx < (int)knobs.size()) {
knobs[kIdx]->label.setBounds(x, y, w, 14);
knobs[kIdx]->slider.setBounds(x, y + 14, w, 74);
}
};
placeKnob(0, 32, 288, 104); placeKnob(1, 146, 288, 104); placeKnob(2, 260, 288, 104);
placeKnob(3, 32, 378, 104); placeKnob(4, 146, 378, 104); placeKnob(5, 260, 378, 104);
placeKnob(6, 422, 288, 114); placeKnob(7, 546, 288, 114); placeKnob(8, 422, 378, 114); placeKnob(9, 546, 378, 114);
placeKnob(10, 704, 324, 78); placeKnob(11, 786, 324, 78); placeKnob(12, 868, 324, 78);
placeKnob(13, 976, 228, 88); placeKnob(14, 1072, 228, 88); placeKnob(15, 1168, 228, 88);
placeKnob(16, 976, 372, 88); placeKnob(17, 1072, 372, 88); placeKnob(18, 1168, 372, 88);
for (int i = 0; i < 4; ++i) { placeKnob(19 + i, 34 + i * 118, 618, 106); placeKnob(23 + i, 548 + i * 118, 618, 106); }
noiseTypeBox.setBounds(1054, 526, 100, 24);
placeKnob(29, 1162, 512, 96);
placeKnob(27, 1054, 614, 96); placeKnob(28, 1158, 614, 96);
placeKnob(30, 86, 195, 110); placeKnob(31, 236, 195, 110); placeKnob(32, 580, 195, 120);
placeKnob(33, 934, 195, 110); placeKnob(34, 1084, 195, 110); placeKnob(35, 156, 515, 120);
placeKnob(36, 506, 515, 110); placeKnob(37, 656, 515, 110);
placeKnob(38, 880, 515, 86); placeKnob(39, 974, 515, 86); placeKnob(40, 1068, 515, 86); placeKnob(41, 1162, 515, 86);
for (int i = 0; i < 4; ++i) {
int y = 142 + i * 142;
lfoScopes[i]->setBounds(28, y + 6, 165, 86);
lfoShapeBoxes[i].setBounds(206, y + 34, 155, 28);
placeKnob(42 + i, 380, y + 4, 105);
}
for (int m = 0; m < 8; ++m) {
int y = 148 + m * 68;
modSrcBoxes[m].setBounds(540, y, 165, 28);
modDstBoxes[m].setBounds(718, y, 185, 28);
modAmtSliders[m].setBounds(916, y, 330, 28);
}
arpOnBtn.setBounds(44, 155, 220, 32); arpModeBox.setBounds(44, 205, 180, 30); arpRateBox.setBounds(240, 205, 140, 30);
placeKnob(46, 430, 175, 115);
for (int i = 0; i < 4; ++i) placeKnob(47 + i, 675 + i * 142, 240, 115);
}
