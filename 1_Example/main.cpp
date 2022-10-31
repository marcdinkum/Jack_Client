
#include "jack_module.h"
#include "tremolo.h"
#include <array>
#include <iostream>

struct Callback : public AudioCallback {
    void prepare (int sampleRate) override {
        for (auto& tremolo : tremolos)
            tremolo.prepareToPlay (static_cast<double> (sampleRate));
    }

    void process (AudioBuffer buffer) override {
        auto [inputChannels, outputChannels, numInputChannels, numOutputChannels, numFrames] = buffer;

        for (auto channel = 0u; channel < numOutputChannels; ++channel) {
            for (auto sample = 0u; sample < numFrames; ++sample) {
                outputChannels[channel][sample] = tremolos[channel].output (inputChannels[0][sample]);
            }
        }
    }

    std::array<Tremolo, 2> tremolos;
};


int main() {
    // Your code goes here!

    auto callback = Callback {};
    auto jack_module = JackModule (callback);

    jack_module.init (2, 2);


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