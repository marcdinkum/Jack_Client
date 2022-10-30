//
// Created by Dean on 27/10/2022.
//
#pragma once

#include "effect.h"
#include "sine.h"

struct Tremolo : Effect {
    void prepareToPlay (double sampleRate) override {
        osc.prepareToPlay (sampleRate);
    }

    float output (float input) override {
        auto modSignal = osc.output();
        modSignal *= amp;
        modSignal += 1.0f - amp;
        return input * modSignal;
    }

    void setAmplitude (float amplitude) {
        amp = amplitude;
    }

    void setRate (float rate) {
        osc.setDelta (rate);
    }

private:
    Sine osc;
    float amp { 1.0f };
};