#include "PluginProcessor.h"
#include "PluginEditor.h"

BlitzVoice::BlitzVoice(juce::AudioProcessorValueTreeState& apvts, WavetableBank& wtBank)
    : state(apvts), bank(wtBank) {}

void BlitzVoice::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate = sampleRate;
    env1.setSampleRate(sampleRate); env2.setSampleRate(sampleRate); env3.setSampleRate(sampleRate);
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)samplesPerBlock, 1 };
    filter1L.prepare(spec); filter1R.prepare(spec); filter2L.prepare(spec); filter2R.prepare(spec);
}

float BlitzVoice::applyWarp(int mode, float p, float amt, float modSig) {
    switch (mode) {
        case 1: return p + modSig * amt * 0.45f;
        case 2: return p * (1.0f + amt * 3.0f);
        case 3: return p < 0.5f ? (p * (1.0f - amt * 0.7f)) : (0.5f + (p - 0.5f) * (1.0f + amt * 0.7f));
        default: return p;
    }
}

void BlitzVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) {
    noteVelocity = velocity;
    targetFreq = (float)juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    float glide = state.getRawParameterValue("glide")->load();
    if (glide <= 0.001f || !isVoiceActive()) currentFreq = targetFreq;
    zapPitchSemis = state.getRawParameterValue("pitchEnv")->load();

    env1.setParameters({ state.getRawParameterValue("att")->load(), state.getRawParameterValue("dec")->load(), state.getRawParameterValue("sus")->load(), state.getRawParameterValue("rel")->load() });
    env2.setParameters({ state.getRawParameterValue("env2_att")->load(), state.getRawParameterValue("env2_dec")->load(), state.getRawParameterValue("env2_sus")->load(), state.getRawParameterValue("env2_rel")->load() });
    env3.setParameters({ state.getRawParameterValue("env3_att")->load(), state.getRawParameterValue("env3_dec")->load(), state.getRawParameterValue("env3_sus")->load(), state.getRawParameterValue("env3_rel")->load() });
    env1.noteOn(); env2.noteOn(); env3.noteOn();
    lfo1.phase = lfo2.phase = lfo3.phase = lfo4.phase = 0.0f;
    samplePosA = samplePosB = grainPhase = 0.0;
}

void BlitzVoice::stopNote(float, bool allowTailOff) {
    env1.noteOff(); env2.noteOff(); env3.noteOff();
    if (!allowTailOff || !env1.isActive()) clearCurrentNote();
}

void BlitzVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) {
    if (!isVoiceActive() || bank.tables.empty()) return;

    bool onA = state.getRawParameterValue("oscA_on")->load() > 0.5f;
    bool onB = state.getRawParameterValue("oscB_on")->load() > 0.5f;
    bool onC = state.getRawParameterValue("oscC_on")->load() > 0.5f;

    int tblA = juce::jlimit(0, (int)bank.tables.size() - 1, (int)state.getRawParameterValue("oscA_wave")->load());
    int tblB = juce::jlimit(0, (int)bank.tables.size() - 1, (int)state.getRawParameterValue("oscB_wave")->load());
    int tblC = juce::jlimit(0, (int)bank.tables.size() - 1, (int)state.getRawParameterValue("oscC_wave")->load());

    float baseWtA = state.getRawParameterValue("oscA_wtPos")->load(), baseWtB = state.getRawParameterValue("oscB_wtPos")->load();
    int warpModeA = (int)state.getRawParameterValue("oscA_warpMode")->load();
    float baseWarpA = state.getRawParameterValue("oscA_warpAmt")->load();
    int uniVoices = juce::jlimit(1, 7, (int)state.getRawParameterValue("oscA_unison")->load());
    float detune = state.getRawParameterValue("oscA_detune")->load();

    float octA = state.getRawParameterValue("oscA_oct")->load(), octB = state.getRawParameterValue("oscB_oct")->load(), octC = state.getRawParameterValue("oscC_oct")->load();
    float semiB = state.getRawParameterValue("oscB_semi")->load();
    float lvlA = onA ? state.getRawParameterValue("oscA_lvl")->load() : 0.0f;
    float lvlB = onB ? state.getRawParameterValue("oscB_lvl")->load() : 0.0f;
    float lvlC = onC ? state.getRawParameterValue("oscC_lvl")->load() : 0.0f;

    float baseCut1 = state.getRawParameterValue("cutoff")->load(), reso1 = state.getRawParameterValue("reso")->load(), envAmt1 = state.getRawParameterValue("envAmt")->load();
    int flt1Type = (int)state.getRawParameterValue("flt1_type")->load();
    float baseCut2 = state.getRawParameterValue("flt2_cutoff")->load(), reso2 = state.getRawParameterValue("flt2_reso")->load();
    int flt2Type = (int)state.getRawParameterValue("flt2_type")->load(), flt2Route = (int)state.getRawParameterValue("flt2_route")->load();

    std::array<float, 4> lfoRates { state.getRawParameterValue("lfo1_rate")->load(), state.getRawParameterValue("lfo2_rate")->load(), state.getRawParameterValue("lfo3_rate")->load(), state.getRawParameterValue("lfo4_rate")->load() };
    std::array<int, 4> lfoShapes { (int)state.getRawParameterValue("lfo1_shape")->load(), (int)state.getRawParameterValue("lfo2_shape")->load(), (int)state.getRawParameterValue("lfo3_shape")->load(), (int)state.getRawParameterValue("lfo4_shape")->load() };
    std::array<float, 4> macros { state.getRawParameterValue("macro1")->load(), state.getRawParameterValue("macro2")->load(), state.getRawParameterValue("macro3")->load(), state.getRawParameterValue("macro4")->load() };

    std::array<int, 8> modSrc {}, modDst {}; std::array<float, 8> modAmt {};
    for (int m = 0; m < 8; ++m) {
        juce::String pre = "mod" + juce::String(m + 1);
        modSrc[m] = (int)state.getRawParameterValue(pre + "_src")->load();
        modDst[m] = (int)state.getRawParameterValue(pre + "_dst")->load();
        modAmt[m] = state.getRawParameterValue(pre + "_amt")->load();
    }

    float baseDiode = state.getRawParameterValue("diode")->load() * 0.01f;
    float glide = state.getRawParameterValue("glide")->load(), master = state.getRawParameterValue("master")->load();
    float glideCoeff = glide > 0.001f ? (1.0f - std::exp(-1.0f / (float)(glide * currentSampleRate))) : 1.0f;

    auto setFType = [](juce::dsp::StateVariableTPTFilter<float>& f, int t) {
        f.setType(t == 1 ? juce::dsp::StateVariableTPTFilterType::highpass : (t == 2 ? juce::dsp::StateVariableTPTFilterType::bandpass : juce::dsp::StateVariableTPTFilterType::lowpass));
    };
    setFType(filter1L, flt1Type); setFType(filter1R, flt1Type);
    setFType(filter2L, flt2Type); setFType(filter2R, flt2Type);

    for (int s = 0; s < numSamples; ++s) {
        float e1 = env1.getNextSample(), e2 = env2.getNextSample(), e3 = env3.getNextSample();
        std::array<float, 12> srcVals { 0.0f, lfo1.tick(currentSampleRate, lfoRates[0], lfoShapes[0]), lfo2.tick(currentSampleRate, lfoRates[1], lfoShapes[1]),
                                            lfo3.tick(currentSampleRate, lfoRates[2], lfoShapes[2]), lfo4.tick(currentSampleRate, lfoRates[3], lfoShapes[3]),
                                            e2, e3, macros[0], macros[1], macros[2], macros[3], noteVelocity };

        std::array<float, 10> dstMod {};
        for (int m = 0; m < 8; ++m)
            if (modSrc[m] > 0 && modDst[m] > 0 && modDst[m] < 10)
                dstMod[modDst[m]] += srcVals[modSrc[m]] * modAmt[m];

        currentFreq += (targetFreq - currentFreq) * glideCoeff;
        zapPitchSemis *= 0.9985f;
        float pitchMult = std::pow(2.0f, (zapPitchSemis + dstMod[6] * 24.0f) / 12.0f);

        float wtPosA = juce::jlimit(0.0f, 1.0f, baseWtA + dstMod[1]);
        float wtPosB = juce::jlimit(0.0f, 1.0f, baseWtB + dstMod[2]);
        float warpA = juce::jlimit(0.0f, 1.0f, baseWarpA + dstMod[3]);

        float baseFA = currentFreq * pitchMult * std::pow(2.0f, octA);
        float baseFB = currentFreq * pitchMult * std::pow(2.0f, octB + (semiB / 12.0f));
        float baseFC = currentFreq * std::pow(2.0f, octC);

        float sigB = 0.0f;
        if (onB || (onA && warpModeA == 1)) {
            phaseB = std::fmod(phaseB + baseFB / (float)currentSampleRate, 1.0f);
            sigB = bank.tables[tblB].sample(wtPosB, phaseB);
        }

        float sumAL = 0.0f, sumAR = 0.0f;
        if (onA && lvlA > 0.001f) {
            for (int u = 0; u < uniVoices; ++u) {
                float spread = (uniVoices == 1) ? 0.0f : (((float)u / (float)(uniVoices - 1)) * 2.0f - 1.0f);
                phaseA[u] = std::fmod(phaseA[u] + (baseFA * std::pow(2.0f, (spread * detune) / 1200.0f)) / (float)currentSampleRate, 1.0f);
                float sampleU = bank.tables[tblA].sample(wtPosA, applyWarp(warpModeA, phaseA[u], warpA, sigB));
                float pan = 0.5f + spread * 0.42f;
                sumAL += sampleU * (1.0f - pan); sumAR += sampleU * pan;
            }
            float normUni = (1.0f / std::sqrt((float)uniVoices)) * 1.4f;
            sumAL *= normUni; sumAR *= normUni;
        }

        float sigC = 0.0f;
        if (onC && lvlC > 0.001f) {
            phaseC = std::fmod(phaseC + baseFC / (float)currentSampleRate, 1.0f);
            sigC = bank.tables[tblC].sample(0.0f, phaseC);
        }

        float mixL = sumAL * lvlA + (onB ? sigB * lvlB * 0.7f : 0.0f) + sigC * lvlC * 0.7f;
        float mixR = sumAR * lvlA + (onB ? sigB * lvlB * 0.7f : 0.0f) + sigC * lvlC * 0.7f;

        float cut1 = juce::jlimit(40.0f, 18000.0f, baseCut1 + e1 * envAmt1 + dstMod[4] * 10000.0f);
        float q1 = juce::jlimit(0.5f, 12.0f, reso1 + dstMod[8] * 8.0f);
        filter1L.setCutoffFrequency(cut1); filter1L.setResonance(q1);
        filter1R.setCutoffFrequency(cut1); filter1R.setResonance(q1);
        float f1L = filter1L.processSample(0, mixL * 0.35f);
        float f1R = filter1R.processSample(0, mixR * 0.35f);
        if (flt1Type == 3) { // Metallic Comb Filter
            int dSmp = juce::jlimit(8, 511, (int)(currentSampleRate / cut1));
            int rIdx = (combIdx - dSmp + 512) & 511;
            float fb = juce::jlimit(0.1f, 0.88f, reso1 / 13.0f);
            f1L = mixL * 0.35f + combBufL[rIdx] * fb; f1R = mixR * 0.35f + combBufR[rIdx] * fb;
            combBufL[combIdx] = f1L; combBufR[combIdx] = f1R; combIdx = (combIdx + 1) & 511;
        } else if (flt1Type == 4) { // Vowel Formant Resonance
            f1L = std::tanh(f1L * (1.0f + reso1 * 0.35f)) * 0.85f;
            f1R = std::tanh(f1R * (1.0f + reso1 * 0.35f)) * 0.85f;
        }

        float outL = f1L, outR = f1R;
        if (flt2Route > 0) {
            float cut2 = juce::jlimit(40.0f, 18000.0f, baseCut2 + dstMod[5] * 10000.0f);

filter2L.setCutoffFrequency(cut2); filter2L.setResonance(juce::jlimit(0.5f, 12.0f, reso2));
filter2R.setCutoffFrequency(cut2); filter2R.setResonance(juce::jlimit(0.5f, 12.0f, reso2));
if (flt2Route == 1) { outL = filter2L.processSample(0, f1L); outR = filter2R.processSample(0, f1R); }
else { outL = (f1L + filter2L.processSample(0, mixL * 0.35f)) * 0.65f; outR = (f1R + filter2R.processSample(0, mixR * 0.35f)) * 0.65f; }
}
float diode = juce::jlimit(0.0f, 1.0f, baseDiode + dstMod[7]);
if (diode > 0.01f) {
float k = diode * 18.0f;
outL = outL > 0.0f ? std::tanh(outL * (1.0f + k)) : outL / (1.0f + std::abs(outL) * k * 0.45f);
outR = outR > 0.0f ? std::tanh(outR * (1.0f + k)) : outR / (1.0f + std::abs(outR) * k * 0.45f);
}
float envGain = e1 * noteVelocity * master * juce::jlimit(0.0f, 1.5f, 1.0f + dstMod[9]);
if (outputBuffer.getNumChannels() >= 2) { outputBuffer.addSample(0, startSample + s, outL * envGain); outputBuffer.addSample(1, startSample + s, outR * envGain); }
else outputBuffer.addSample(0, startSample + s, (outL + outR) * 0.5f * envGain);
if (!env1.isActive()) { clearCurrentNote(); break; }
}
}
BlitzAudioProcessor::BlitzAudioProcessor()
: AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
apvts(*this, nullptr, "PARAMETERS", createParameterLayout()) {
ensureDefaultAssetsExist(); reloadWavetables();
for (int i = 0; i < 8; ++i) synth.addVoice(new BlitzVoice(apvts, wavetableBank));
synth.addSound(new BlitzSound());
}
void BlitzAudioProcessor::loadSampleIntoOsc(int oscIdx, const juce::File& wavFile) {
    int newIdx = wavetableBank.loadSingleWavFile(wavFile, oscIdx);
    if (newIdx >= 0) {
        juce::String id = (oscIdx == 0) ? "oscA_wave" : (oscIdx == 1 ? "oscB_wave" : "oscC_wave");
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1((float)newIdx));
    }
}

void BlitzAudioProcessor::reloadWavetables() {
wavetableBank.buildFactoryTables();
wavetableBank.loadExternalWavetablesFromFolder(getRootUserDirectory().getChildFile("Wavetables"));
}
juce::AudioProcessorValueTreeState::ParameterLayout BlitzAudioProcessor::createParameterLayout() {
std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
auto addF = [&](const char* id, const char* nm, float mn, float mx, float df) { p.push_back(std::make_unique<juce::AudioParameterFloat>(id, nm, mn, mx, df)); };
auto addI = [&](const char* id, const char* nm, int mn, int mx, int df) { p.push_back(std::make_unique<juce::AudioParameterInt>(id, nm, mn, mx, df)); };
auto addB = [&](const char* id, const char* nm, bool df) { p.push_back(std::make_unique<juce::AudioParameterBool>(id, nm, df)); };
addB("oscA_on", "Osc A On", true); addB("oscB_on", "Osc B On", true); addB("oscC_on", "Osc C On", true);
p.push_back(std::make_unique<juce::AudioParameterChoice>("oscA_engMode", "Osc A Mode", juce::StringArray{"Wavetable", "Granular", "Sample", "Spectral"}, 0));
    addI("oscA_wave", "Osc A WT", 0, 64, 0); addF("oscA_oct", "Osc A Oct", -2.0f, 2.0f, 0.0f); addF("oscA_detune", "Osc A Detune", 0.0f, 50.0f, 18.0f);
addF("oscA_lvl", "Osc A Level", 0.0f, 1.0f, 0.80f); addF("oscA_wtPos", "Osc A WT Pos", 0.0f, 1.0f, 0.25f); addI("oscA_unison", "Osc A Unison", 1, 7, 3);
p.push_back(std::make_unique<juce::AudioParameterChoice>("oscA_warpMode", "Warp Mode", juce::StringArray{ "Warp: Off", "Warp: FM (Osc B)", "Warp: Hard Sync", "Warp: PWM Bend" }, 0));
addF("oscA_warpAmt", "Warp Amt", 0.0f, 1.0f, 0.0f);
p.push_back(std::make_unique<juce::AudioParameterChoice>("oscB_engMode", "Osc B Mode", juce::StringArray{"Wavetable", "Granular", "Sample", "Spectral"}, 0));
    addI("oscB_wave", "Osc B WT", 0, 64, 1); addF("oscB_oct", "Osc B Oct", -2.0f, 2.0f, 1.0f); addF("oscB_semi", "Osc B Semi", -12.0f, 12.0f, 0.0f);
addF("oscB_lvl", "Osc B Level", 0.0f, 1.0f, 0.55f); addF("oscB_wtPos", "Osc B WT Pos", 0.0f, 1.0f, 0.40f);
addI("oscC_wave", "Osc C WT", 0, 64, 3); addF("oscC_oct", "Osc C Oct", -2.0f, 1.0f, -1.0f); addF("glide", "Glide", 0.0f, 0.3f, 0.04f); addF("oscC_lvl", "Osc C Level", 0.0f, 1.0f, 0.50f);
    p.push_back(std::make_unique<juce::AudioParameterChoice>("noise_type", "Noise Type", juce::StringArray{"White", "Pink", "Vinyl Crackle", "Hammer Click"}, 0));
    addF("noise_lvl", "Noise Level", 0.0f, 1.0f, 0.0f);
p.push_back(std::make_unique<juce::AudioParameterFloat>("cutoff", "Cutoff 1", juce::NormalisableRange<float>(80.0f, 16000.0f, 1.0f, 0.3f), 6800.0f));
addF("reso", "Reso 1", 0.5f, 12.0f, 3.2f); addF("envAmt", "Filter Env", 0.0f, 8000.0f, 2600.0f);
p.push_back(std::make_unique<juce::AudioParameterChoice>("flt1_type", "Filter 1 Type", juce::StringArray{ "LP 24", "HP 24", "BP 12", "Comb", "Formant" }, 0));
p.push_back(std::make_unique<juce::AudioParameterFloat>("flt2_cutoff", "Cutoff 2", juce::NormalisableRange<float>(80.0f, 16000.0f, 1.0f, 0.3f), 2400.0f));
addF("flt2_reso", "Reso 2", 0.5f, 12.0f, 2.5f);
p.push_back(std::make_unique<juce::AudioParameterChoice>("flt2_type", "Filter 2 Type", juce::StringArray{ "LP 24", "HP 24", "BP 12", "Comb", "Formant" }, 1));
p.push_back(std::make_unique<juce::AudioParameterChoice>("flt2_route", "Filter 2 Route", juce::StringArray{ "Flt 2: Off", "Flt 2: Series", "Flt 2: Parallel" }, 0));
addF("att", "Attack", 0.002f, 0.5f, 0.004f); addF("dec", "Decay", 0.05f, 2.0f, 0.25f); addF("sus", "Sustain", 0.0f, 1.0f, 0.72f); addF("rel", "Release", 0.01f, 1.5f, 0.05f);
addF("env2_att", "Env2 Att", 0.002f, 1.0f, 0.01f); addF("env2_dec", "Env2 Dec", 0.05f, 2.0f, 0.4f); addF("env2_sus", "Env2 Sus", 0.0f, 1.0f, 0.0f); addF("env2_rel", "Env2 Rel", 0.01f, 1.5f, 0.1f);
addF("env3_att", "Env3 Att", 0.002f, 1.0f, 0.15f); addF("env3_dec", "Env3 Dec", 0.05f, 2.0f, 0.8f); addF("env3_sus", "Env3 Sus", 0.0f, 1.0f, 0.5f); addF("env3_rel", "Env3 Rel", 0.01f, 1.5f, 0.3f);
juce::StringArray lfoShapes { "Sine", "Triangle", "Ramp Up", "Saw Down", "Square", "S&H Rand" };
for (int i = 1; i <= 4; ++i) {
addF(("lfo" + juce::String(i) + "_rate").toRawUTF8(), ("LFO " + juce::String(i) + " Rate").toRawUTF8(), 0.1f, 20.0f, (float)i * 1.5f);
p.push_back(std::make_unique<juce::AudioParameterChoice>("lfo" + juce::String(i) + "_shape", "LFO Shape", lfoShapes, i - 1));
}
juce::StringArray modSrcs { "Off", "LFO 1", "LFO 2", "LFO 3", "LFO 4", "ENV 2", "ENV 3", "Macro 1", "Macro 2", "Macro 3", "Macro 4", "Velocity" };
juce::StringArray modDsts { "Off", "Osc A WT Pos", "Osc B WT Pos", "Osc A Warp", "Filter 1 Cutoff", "Filter 2 Cutoff", "Master Pitch", "Diode Drive", "Filter 1 Reso", "Master Amp" };
for (int m = 1; m <= 8; ++m) {
juce::String pre = "mod" + juce::String(m);
p.push_back(std::make_unique<juce::AudioParameterChoice>(pre + "_src", "Mod Src", modSrcs, 0));
p.push_back(std::make_unique<juce::AudioParameterChoice>(pre + "_dst", "Mod Dst", modDsts, 0));
addF((pre + "_amt").toRawUTF8(), "Mod Amt", -1.0f, 1.0f, 0.0f);
}
addF("diode", "Diode Drive", 0.0f, 100.0f, 38.0f); addF("pitchEnv", "Zap Pitch", -24.0f, 24.0f, 0.0f);
addF("crush_bits", "Bit Depth", 2.0f, 16.0f, 16.0f); addF("crush_down", "Downsample", 1.0f, 32.0f, 1.0f);
addF("chorus_mix", "Hyper Chorus", 0.0f, 1.0f, 0.0f); addF("chorus_rate", "Chorus Rate", 0.1f, 5.0f, 1.2f);
addF("delayMix", "Bounce Delay", 0.0f, 0.6f, 0.22f); addF("reverb_mix", "Hall Reverb", 0.0f, 0.8f, 0.0f); addF("reverb_size", "Reverb Size", 0.1f, 1.0f, 0.65f);
addF("ott_amt", "OTT Punch", 0.0f, 1.0f, 0.0f); addF("eq_low", "EQ Low", -12.0f, 12.0f, 0.0f); addF("eq_mid", "EQ Mid", -12.0f, 12.0f, 0.0f); addF("eq_high", "EQ High", -12.0f, 12.0f, 0.0f);
    addF("master", "Master Out", 0.0f, 1.0f, 0.75f);
addB("arp_on", "Arp Enable", false);
p.push_back(std::make_unique<juce::AudioParameterChoice>("arp_mode", "Arp Mode", juce::StringArray{ "Up", "Down", "Up/Down", "Random" }, 0));
p.push_back(std::make_unique<juce::AudioParameterChoice>("arp_rate", "Arp Rate", juce::StringArray{ "1/8", "1/16", "1/32" }, 1));
addF("arp_gate", "Arp Gate", 0.1f, 0.95f, 0.65f);
for (int i = 1; i <= 4; ++i) addF(("macro" + juce::String(i)).toRawUTF8(), ("Macro " + juce::String(i)).toRawUTF8(), 0.0f, 1.0f, 0.0f);
return { p.begin(), p.end() };
}
void BlitzAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
synth.setCurrentPlaybackSampleRate(sampleRate);
for (int i = 0; i < synth.getNumVoices(); ++i)
if (auto* v = dynamic_cast<BlitzVoice*>(synth.getVoice(i))) v->prepareToPlay(sampleRate, samplesPerBlock);
juce::dsp::ProcessSpec monoSpec { sampleRate, (juce::uint32)samplesPerBlock, 1 }, stereoSpec { sampleRate, (juce::uint32)samplesPerBlock, 2 };
delayL.prepare(monoSpec); delayR.prepare(monoSpec); delayL.setDelay((float)(sampleRate * 0.214)); delayR.setDelay((float)(sampleRate * 0.285));
chorus.prepare(stereoSpec); chorus.setCentreDelay(7.0f); chorus.setFeedback(0.25f); reverb.prepare(stereoSpec);
}
bool BlitzAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const { return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
void BlitzAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
buffer.clear();
arp.processMidi(midiMessages, buffer.getNumSamples(), getSampleRate(), apvts.getRawParameterValue("arp_on")->load() > 0.5f,
(int)apvts.getRawParameterValue("arp_mode")->load(), (int)apvts.getRawParameterValue("arp_rate")->load(), apvts.getRawParameterValue("arp_gate")->load());
synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
if (buffer.getNumChannels() < 2) return;
auto *Lch = buffer.getWritePointer(0), *Rch = buffer.getWritePointer(1);
int numS = buffer.getNumSamples();
float bits = apvts.getRawParameterValue("crush_bits")->load();
int down = (int)apvts.getRawParameterValue("crush_down")->load();
if (bits < 15.9f || down > 1) {
float levels = std::pow(2.0f, bits);
for (int i = 0; i < numS; ++i) {
if (++crushCounter >= down) { crushCounter = 0; crushHoldL = std::round(Lch[i] * levels) / levels; crushHoldR = std::round(Rch[i] * levels) / levels; }
Lch[i] = crushHoldL; Rch[i] = crushHoldR;
}
}
float cMix = apvts.getRawParameterValue("chorus_mix")->load();
if (cMix > 0.01f) {
chorus.setMix(cMix); chorus.setRate(apvts.getRawParameterValue("chorus_rate")->load()); chorus.setDepth(0.35f);
juce::dsp::AudioBlock<float> block(buffer); chorus.process(juce::dsp::ProcessContextReplacing<float>(block));
}
float ott = apvts.getRawParameterValue("ott_amt")->load();
if (ott > 0.01f) {
for (int i = 0; i < numS; ++i) {
auto squash = [&](float x) { float s = x >= 0.0f ? 1.0f : -1.0f; float a = std::abs(x); return s * (a * (1.0f - ott) + std::min(1.0f, std::pow(a + 1e-5f, 0.55f) * 0.85f) * ott); };
Lch[i] = squash(Lch[i]); Rch[i] = squash(Rch[i]);
}
}
float dMix = apvts.getRawParameterValue("delayMix")->load();
if (dMix > 0.001f) {
for (int i = 0; i < numS; ++i) {
float dL = delayL.popSample(0), dR = delayR.popSample(0);
delayL.pushSample(0, Lch[i] + dR * 0.38f); delayR.pushSample(0, Rch[i] + dL * 0.38f);
Lch[i] += dL * dMix; Rch[i] += dR * dMix;
}
}
float rMix = apvts.getRawParameterValue("reverb_mix")->load();
if (rMix > 0.01f) {
juce::dsp::Reverb::Parameters rp; rp.roomSize = apvts.getRawParameterValue("reverb_size")->load(); rp.wetLevel = rMix; rp.dryLevel = 1.0f; rp.damping = 0.35f; rp.width = 1.0f;
reverb.setParameters(rp); juce::dsp::AudioBlock<float> block(buffer); reverb.process(juce::dsp::ProcessContextReplacing<float>(block));
    }
    // Real 3-Band Master EQ (Low Shelf 140Hz, Mid Peak 1.2kHz, High Shelf 6.5kHz)
    float eqL = apvts.getRawParameterValue("eq_low")->load();
    float eqM = apvts.getRawParameterValue("eq_mid")->load();
    float eqH = apvts.getRawParameterValue("eq_high")->load();
    if (std::abs(eqL) > 0.05f || std::abs(eqM) > 0.05f || std::abs(eqH) > 0.05f) {
        double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
        *eqLowL.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, 140.0f, 0.707f, juce::Decibels::decibelsToGain(eqL));
        *eqLowR.coefficients = *eqLowL.coefficients;
        *eqMidL.coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, 1200.0f, 0.9f, juce::Decibels::decibelsToGain(eqM));
        *eqMidR.coefficients = *eqMidL.coefficients;
        *eqHighL.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, 6500.0f, 0.707f, juce::Decibels::decibelsToGain(eqH));
        *eqHighR.coefficients = *eqHighL.coefficients;
        auto* pL = buffer.getWritePointer(0);
        auto* pR = (buffer.getNumChannels() > 1) ? buffer.getWritePointer(1) : pL;
        for (int s = 0; s < buffer.getNumSamples(); ++s) {
            pL[s] = eqHighL.processSample(eqMidL.processSample(eqLowL.processSample(pL[s])));
            if (buffer.getNumChannels() > 1) pR[s] = eqHighR.processSample(eqMidR.processSample(eqLowR.processSample(pR[s])));
        }
    }

    // Live Post-FX Stereo Peak Metering & Header Oscilloscope Feed
    int nSmp = buffer.getNumSamples();
    float peakL = buffer.getMagnitude(0, 0, nSmp);
    float peakR = (buffer.getNumChannels() > 1) ? buffer.getMagnitude(1, 0, nSmp) : peakL;
    meterLevelL.store(std::max(peakL, meterLevelL.load() * 0.85f));
    meterLevelR.store(std::max(peakR, meterLevelR.load() * 0.85f));
    const float* rdL = buffer.getReadPointer(0);
    int wIdx = scopeWriteIdx.load();
    for (int s = 0; s < nSmp; s += 2) {
        scopeRing[(size_t)(wIdx & 255)] = rdL[s];
        ++wIdx;
    }
    scopeWriteIdx.store(wIdx);
}

juce::File BlitzAudioProcessor::getRootUserDirectory() {
    auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Blitz-NPC");
    dir.getChildFile("Banks/Factory Blitz").createDirectory(); dir.getChildFile("Banks/User Custom").createDirectory();
    dir.getChildFile("Skins").createDirectory(); dir.getChildFile("Wavetables").createDirectory();
    return dir;
}
void BlitzAudioProcessor::savePresetToFile(const juce::String& bankName, const juce::String& presetName) {
    auto bankDir = getRootUserDirectory().getChildFile("Banks").getChildFile(bankName); bankDir.createDirectory();
    if (auto xml = apvts.copyState().createXml()) xml->writeTo(bankDir.getChildFile(presetName + ".blitz"));
}
void BlitzAudioProcessor::loadPresetFromFile(const juce::File& presetFile) {
    if (auto xml = juce::XmlDocument::parse(presetFile)) {
        if (xml->hasTagName(apvts.state.getType())) {
            currentBankName = presetFile.getParentDirectory().getFileName();
            currentPresetName = presetFile.getFileNameWithoutExtension();
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
            getRootUserDirectory().getChildFile("last_session.txt").replaceWithText(currentBankName + "\n" + currentPresetName + "\n" + currentSkinName);
        }
    }
}
void BlitzAudioProcessor::ensureDefaultAssetsExist() {
    auto root = getRootUserDirectory(), skinsDir = root.getChildFile("Skins");
    auto s1 = skinsDir.getChildFile("Serum Cyber Cyan.json");
    if (!s1.existsAsFile()) s1.replaceWithText(R"({"bgDark":"ff0d1015","panelBg":"ff151921","panelBorder":"ff28303d","accentPrimary":"ff00e5ff","accentSecondary":"ffff2a85","accentTertiary":"ff2ee59d","textMain":"ffeaf0f8"})");
    auto s2 = skinsDir.getChildFile("Harinezumi Neon Pink.json");
    if (!s2.existsAsFile()) s2.replaceWithText(R"({"bgDark":"ff130d17","panelBg":"ff1e1524","panelBorder":"ff3e274a","accentPrimary":"ffff3399","accentSecondary":"ff00ffcc","accentTertiary":"ffffcc00","textMain":"fffff0f8"})");
}
void BlitzAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    std::unique_ptr<juce::XmlElement> xml(apvts.copyState().createXml()); copyXmlToBinary(*xml, destData);
}
void BlitzAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}
juce::AudioProcessorEditor* BlitzAudioProcessor::createEditor() { return new BlitzAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BlitzAudioProcessor(); }
