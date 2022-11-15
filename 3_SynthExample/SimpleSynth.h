//
// Created by Dean on 08/11/2022.
//

#include "oscillator.h"

struct SimpleSynth {
    float output() {
        return sine.output();
    }
    void prepare (double sampleRate) {
        sine.setSampleRate (sampleRate);
    }

    void setPitch (float pitch) {
        sine.setFrequency (mtof (pitch));
    }

private:
    inline float mtof (float midiPitch) {
        return 440.0f * std::pow (2.0f, (midiPitch - 69.0f) / 12.0f);
    }

    Sine sine { 48000 };
};