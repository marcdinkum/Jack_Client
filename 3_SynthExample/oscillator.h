//
// Created by Dean on 08/11/2022.
//
#include <cmath>

struct Oscillator {
    Oscillator (float sampleRate) : sampleRate (sampleRate) {
    }

    float output() {
        phase += delta;
        if (phase > 1.0) phase -= 1.0;

        return calculate();
    }

    void setFrequency (float frequency) {
        delta = frequency / sampleRate;
    }
    void setSampleRate (float Fs) {
        sampleRate = Fs;
    }

    virtual float calculate() = 0;


protected:
    const float pi = acos (-1.0f);
    float sampleRate { 0 };
    float phase { 0 };
    float delta { 0 };
};

struct Sine : public Oscillator {
    Sine (float sampleRate) : Oscillator (sampleRate) {
    }
    float calculate() override {
        return sin (phase * pi * 2.0f);
    }
};