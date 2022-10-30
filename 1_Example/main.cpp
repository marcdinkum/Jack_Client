//
// Created by Dean on 29/10/2022.
//

#include "jack_module.h"
#include "sine.h"

struct Callback : public AudioCallback {
    void prepare (uint64_t sampleRate) override {
        sine.prepareToPlay ((double) sampleRate);
        sine.setDelta (50);
    }

    void process (AudioBuffer buffer) noexcept override {
        auto [inputChannels, outputChannels, numInputChannels, numOutputChannels, numFrames] = buffer;

        for (auto sample = 0u; sample < numFrames; ++sample) {
            const auto sineSample = sine.output() * 0.01;
            for (auto channel = 0u; channel < numOutputChannels; ++channel) {
                outputChannels[channel][sample] = sineSample;
            }
        }
    }

    Sine sine;
};


int main() {
    // Your code goes here!

    auto jack_module = JackModule();

    auto callback = Callback {};

    jack_module.setNumInputChannels (4);
    jack_module.setNumOutputChannels (2);

    jack_module.init();
    jack_module.autoConnect();
    jack_module.setAudioCallback (callback);


    auto running = true;
    while (running) {
        switch (std::cin.get()) {
            case 'q':
                running = false;
                break;
        }
    }


    return 1 + 1 - 2;
}