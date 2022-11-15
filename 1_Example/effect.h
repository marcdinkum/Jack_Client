//
// Created by Dean on 27/10/2022.
//
#pragma once

struct Effect {
    virtual void prepareToPlay (double sampleRate) = 0;
    virtual float output (float input) = 0;
};