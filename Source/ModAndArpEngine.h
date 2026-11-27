#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <array>

struct LFOEngine {
    float phase = 0.0f;
    float shValue = 0.0f;

    float tick(double sr, float rateHz, int shape, bool resetPhase = false) {
        if (resetPhase) phase = 0.0f;
        float prev = phase;
        phase += rateHz / (float)sr;
        if (phase >= 1.0f) {
            phase -= std::floor(phase);
            shValue = ((float)std::rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        }
        switch (shape) {
            case 0: return std::sin(phase * juce::MathConstants<float>::twoPi); // Sine
            case 1: return 4.0f * std::abs(phase - 0.5f) - 1.0f;                    // Triangle
            case 2: return 2.0f * phase - 1.0f;                                     // Ramp Up
            case 3: return 1.0f - 2.0f * phase;                                     // Saw Down
            case 4: return phase < 0.5f ? 1.0f : -1.0f;                           // Square
            default: return shValue;                                                // S&H Random
        }
    }
};

struct SimpleArpeggiator {
    std::vector<int> heldNotes;
    int currentStep = 0;
    int lastPlayedNote = -1;
    double sampleCounter = 0.0;

    void processMidi(juce::MidiBuffer& midi, int numSamples, double sr, bool enabled, int mode, int rateIdx, float gate) {
        if (!enabled) {
            heldNotes.clear();
            lastPlayedNote = -1;
            return;
        }
        juce::MidiBuffer out;
        for (const auto meta : midi) {
            auto msg = meta.getMessage();
            if (msg.isNoteOn()) {
                if (std::find(heldNotes.begin(), heldNotes.end(), msg.getNoteNumber()) == heldNotes.end())
                    heldNotes.push_back(msg.getNoteNumber());
                std::sort(heldNotes.begin(), heldNotes.end());
            } else if (msg.isNoteOff()) {
                heldNotes.erase(std::remove(heldNotes.begin(), heldNotes.end(), msg.getNoteNumber()), heldNotes.end());
            } else {
                out.addEvent(msg, meta.samplePosition);
            }
        }
        if (heldNotes.empty()) {
            if (lastPlayedNote != -1) {
                out.addEvent(juce::MidiMessage::noteOff(1, lastPlayedNote), 0);
                lastPlayedNote = -1;
            }
            midi.swapWith(out);
            return;
        }

        double div = (rateIdx == 0) ? 0.25 : (rateIdx == 1 ? 0.125 : 0.0625); // 1/8, 1/16, 1/32 at 140 BPM
        double stepSamples = sr * (60.0 / 140.0) * (div * 4.0);
        double gateSamples = stepSamples * juce::jlimit(0.1f, 0.95f, gate);

        for (int s = 0; s < numSamples; ++s) {
            if (sampleCounter == 0.0) {
                if (lastPlayedNote != -1) out.addEvent(juce::MidiMessage::noteOff(1, lastPlayedNote), s);
                int idx = 0;
                int n = (int)heldNotes.size();
                if (mode == 0) idx = currentStep % n; // Up
                else if (mode == 1) idx = (n - 1) - (currentStep % n); // Down
                else if (mode == 2) { int p = std::max(1, n * 2 - 2); int k = currentStep % p; idx = k < n ? k : (p - k); } // UpDown
                else idx = std::rand() % n; // Random

                lastPlayedNote = heldNotes[idx];
                out.addEvent(juce::MidiMessage::noteOn(1, lastPlayedNote, (juce::uint8)110), s);
                currentStep++;
            } else if (sampleCounter >= gateSamples && lastPlayedNote != -1) {
                out.addEvent(juce::MidiMessage::noteOff(1, lastPlayedNote), s);
                lastPlayedNote = -1;
            }
            sampleCounter += 1.0;
            if (sampleCounter >= stepSamples) sampleCounter = 0.0;
        }
        midi.swapWith(out);
    }
};
