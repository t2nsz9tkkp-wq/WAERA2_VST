#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>
#include <cmath>


struct SampleBufferAsset {
    juce::String name { "Empty" };
    std::vector<float> samples;
    double sampleRate = 44100.0;

    float readAt(double pos, bool loop) const {
        if (samples.empty()) return 0.0f;
        int n = (int)samples.size();
        if (loop) pos = std::fmod(pos, (double)n);
        if (pos < 0.0 || pos >= (double)(n - 1)) return 0.0f;
        int i0 = (int)pos;
        float frac = (float)(pos - (double)i0);
        return samples[i0] + frac * (samples[i0 + 1] - samples[i0]);
    }
};

class WavetableBank {
public:
    SampleBufferAsset directSamples[2];
public:
    static constexpr int tableSize = 2048;
    static constexpr int defaultFrames = 64;

    struct Table {
        juce::String name;
        std::vector<std::vector<float>> frames;

        float sample(float wtPos, float phase) const {
            if (frames.empty()) return 0.0f;
            float fIdx = juce::jlimit(0.0f, 1.0f, wtPos) * (float)(frames.size() - 1);
            int f0 = (int)fIdx;
            int f1 = std::min(f0 + 1, (int)frames.size() - 1);
            float fFrac = fIdx - (float)f0;

            float p = phase - std::floor(phase);
            float sIdx = p * (float)tableSize;
            int i0 = (int)sIdx % tableSize;
            int i1 = (i0 + 1) % tableSize;
            float sFrac = sIdx - std::floor(sIdx);

            float v0 = frames[f0][i0] + sFrac * (frames[f0][i1] - frames[f0][i0]);
            float v1 = frames[f1][i0] + sFrac * (frames[f1][i1] - frames[f1][i0]);
            return v0 + fFrac * (v1 - v0);
        }
    };

    std::vector<Table> tables;

    WavetableBank() { buildFactoryTables(); }

    void buildFactoryTables() {
        tables.clear();
        // 0: Digi-Saw -> Pulse
        Table t0 { "Digi-Saw -> Pulse", std::vector<std::vector<float>>(defaultFrames, std::vector<float>(tableSize)) };
        for (int f = 0; f < defaultFrames; ++f) {
            float morph = (float)f / (float)(defaultFrames - 1);
            float pw = 0.5f - morph * 0.42f;
            for (int i = 0; i < tableSize; ++i) {
                float p = (float)i / (float)tableSize;
                t0.frames[f][i] = (2.0f * p - 1.0f) * (1.0f - morph) + ((p < pw) ? 0.9f : -0.9f) * morph;
            }
        }
        tables.push_back(std::move(t0));

        // 1: Gameboy Chip Morph
        Table t1 { "Gameboy Chip Morph", std::vector<std::vector<float>>(defaultFrames, std::vector<float>(tableSize)) };
        for (int f = 0; f < defaultFrames; ++f) {
            int steps = 4 + (int)(((float)f / (float)(defaultFrames - 1)) * 28.0f);
            for (int i = 0; i < tableSize; ++i) {
                float p = (float)i / (float)tableSize;
                float raw = std::sin(p * juce::MathConstants<float>::twoPi) + 0.5f * (p < 0.5f ? 1.0f : -1.0f);
                t1.frames[f][i] = std::round(raw * (float)steps) / (float)steps;
            }
        }
        tables.push_back(std::move(t1));

        // 2: Harinezumi FM Harmonics
        Table t2 { "Harinezumi FM Harmonics", std::vector<std::vector<float>>(defaultFrames, std::vector<float>(tableSize)) };
        for (int f = 0; f < defaultFrames; ++f) {
            float idx = ((float)f / (float)(defaultFrames - 1)) * 5.0f;
            for (int i = 0; i < tableSize; ++i) {
                float p = (float)i / (float)tableSize * juce::MathConstants<float>::twoPi;
                t2.frames[f][i] = std::sin(p + idx * std::sin(p * 3.0f)) * 0.85f;
            }
        }
        tables.push_back(std::move(t2));

        // 3: Pure Sub -> Folded Bass
        Table t3 { "Pure Sub -> Folded Bass", std::vector<std::vector<float>>(defaultFrames, std::vector<float>(tableSize)) };
        for (int f = 0; f < defaultFrames; ++f) {
            float drive = 1.0f + ((float)f / (float)(defaultFrames - 1)) * 4.5f;
            for (int i = 0; i < tableSize; ++i) {
                float p = (float)i / (float)tableSize * juce::MathConstants<float>::twoPi;
                t3.frames[f][i] = std::sin(std::sin(p) * drive);
            }
        }
        tables.push_back(std::move(t3));

        // 4: Acoustic Steinway Grand Piano (12-partial realistic piano harmonic spectrum + hammer strike morph)
        Table t4 { "Steinway Grand Piano", std::vector<std::vector<float>>(defaultFrames, std::vector<float>(tableSize)) };
        const float pWeights[12] = { 1.00f, 0.56f, 0.34f, 0.24f, 0.16f, 0.12f, 0.08f, 0.06f, 0.045f, 0.03f, 0.02f, 0.015f };
        for (int f = 0; f < defaultFrames; ++f) {
            float hammer = (float)f / (float)(defaultFrames - 1);
            for (int i = 0; i < tableSize; ++i) {
                float p = (float)i / (float)tableSize * juce::MathConstants<float>::twoPi;
                float sum = 0.0f;
                for (int h = 0; h < 12; ++h) {
                    float rolloff = std::pow(hammer, (float)h * 0.28f);
                    sum += std::sin(p * (float)(h + 1)) * pWeights[h] * rolloff;
                }
                t4.frames[f][i] = juce::jlimit(-1.0f, 1.0f, sum * 0.58f);
            }
        }
        tables.push_back(std::move(t4));

        // 5: Rhodes Suitcase Electric Piano (Warm fundamental + 7th/11th tine bark)
        Table t5 { "Rhodes Suitcase E-Piano", std::vector<std::vector<float>>(defaultFrames, std::vector<float>(tableSize)) };
        for (int f = 0; f < defaultFrames; ++f) {
            float tine = (float)f / (float)(defaultFrames - 1);
            for (int i = 0; i < tableSize; ++i) {
                float p = (float)i / (float)tableSize * juce::MathConstants<float>::twoPi;
                float s = std::sin(p) * 0.78f + std::sin(p * 2.0f) * 0.25f * tine + std::sin(p * 3.0f) * 0.12f * tine
                        + std::sin(p * 7.0f) * 0.24f * tine * tine + std::sin(p * 11.0f) * 0.10f * tine * tine;
                t5.frames[f][i] = juce::jlimit(-1.0f, 1.0f, s * 0.85f);
            }
        }
        tables.push_back(std::move(t5));

        // 6: Symphonic Bowed Strings (Rich odd/even bowed saw-formant harmonics)
        Table t6 { "Symphonic Bowed Strings", std::vector<std::vector<float>>(defaultFrames, std::vector<float>(tableSize)) };
        for (int f = 0; f < defaultFrames; ++f) {
            float bow = 0.2f + ((float)f / (float)(defaultFrames - 1)) * 0.8f;
            for (int i = 0; i < tableSize; ++i) {
                float p = (float)i / (float)tableSize * juce::MathConstants<float>::twoPi;
                float sum = 0.0f;
                for (int h = 1; h <= 16; ++h) {
                    float amp = (1.0f / (float)h) * std::pow(bow, (float)(h - 1) * 0.18f);
                    if (h == 3 || h == 5) amp *= 1.25f; // Violin bridge body resonance
                    sum += std::sin(p * (float)h) * amp;
                }
                t6.frames[f][i] = juce::jlimit(-1.0f, 1.0f, sum * 0.55f);
            }
        }
        tables.push_back(std::move(t6));

        // 7: Acoustic Nylon Guitar & Concert Harp
        Table t7 { "Acoustic Harp & Nylon", std::vector<std::vector<float>>(defaultFrames, std::vector<float>(tableSize)) };
        for (int f = 0; f < defaultFrames; ++f) {
            float pluck = (float)f / (float)(defaultFrames - 1);
            for (int i = 0; i < tableSize; ++i) {
                float p = (float)i / (float)tableSize * juce::MathConstants<float>::twoPi;
                float sum = 0.0f;
                for (int h = 1; h <= 10; ++h) {
                    float notch = std::sin((float)h * 0.65f);
                    sum += std::sin(p * (float)h) * (notch / (float)h) * std::pow(pluck, (float)(h - 1) * 0.22f);
                }
                t7.frames[f][i] = juce::jlimit(-1.0f, 1.0f, sum * 0.72f);
            }
        }
        tables.push_back(std::move(t7));
    }

    int loadSingleWavFile(const juce::File& file, int targetOsc = 0) {
        juce::AudioFormatManager fm2; fm2.registerBasicFormats();
        if (std::unique_ptr<juce::AudioFormatReader> r2{fm2.createReaderFor(file)}) {
            int len = (int)std::min((juce::int64)441000, r2->lengthInSamples);
            juce::AudioBuffer<float> b2(1, len); r2->read(&b2, 0, len, 0, true, false);
            if (targetOsc >= 0 && targetOsc < 2) {
                directSamples[targetOsc].name = file.getFileNameWithoutExtension();
                directSamples[targetOsc].sampleRate = r2->sampleRate;
                directSamples[targetOsc].samples.assign(b2.getReadPointer(0), b2.getReadPointer(0) + len);
            }
        }
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
        if (reader == nullptr || reader->lengthInSamples < 64) return -1;
        int totalSamples = (int)std::min((juce::int64)(64 * tableSize), reader->lengthInSamples);
        int numFrames = std::max(1, totalSamples / tableSize);
        juce::AudioBuffer<float> buf(1, numFrames * tableSize); buf.clear();
        reader->read(&buf, 0, std::min(totalSamples, numFrames * tableSize), 0, true, false);
        Table custom { file.getFileNameWithoutExtension(), std::vector<std::vector<float>>(numFrames, std::vector<float>(tableSize, 0.0f)) };
        for (int f = 0; f < numFrames; ++f)
            for (int i = 0; i < tableSize; ++i)
                custom.frames[f][i] = buf.getSample(0, (f * tableSize + i) % totalSamples);
        tables.push_back(std::move(custom));
        return (int)tables.size() - 1;
    }

    void loadExternalWavetablesFromFolder(const juce::File& wtDir) {
        auto files = wtDir.findChildFiles(juce::File::findFiles, false, "*.wav");
        files.sort();
        for (auto& file : files) loadSingleWavFile(file);
    }
};
