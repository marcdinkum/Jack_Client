//
// Created by Dean on 27/10/2022.
//
#pragma once

#include <cmath>

struct Sine {
    void prepareToPlay(double sampleRate){
        samplerate = sampleRate;
        resetPhase();
        setDelta(2);
    }

    float output() {
        phase += delta;
        if (phase > 1.0) phase -= 1.0f;
        return calculate();
    }
    float calculate() {
        return sin (phase * pi * 2.0f);
    }

    void setDelta (float frequency) {
        currentFrequency = frequency;
        this->delta = currentFrequency / samplerate;
    }
    double getFrequency() {
        return currentFrequency;
    }
    void resetPhase() {
        phase = 0;
    }

private:
    const float pi = acos (-1.0f);
    float samplerate { 0.0f };
    float phase { 0.0f };
    float currentFrequency { 0.0f };
    float delta { 0.0f };
};
