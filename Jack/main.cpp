//
// Created by Dean on 29/10/2022.
//

#include "jack_module.h"

struct Callback : AudioCallback {

    void process(AudioBuffer buffer) override {
        auto [inputChannels**, outputChannels**, numInputChannels, numOutputChannels, numFrames] = buffer;

        for(auto channel = 0u; channel < numOutputChannels; ++channel){
            for(auto sample = 0u; sample < numFrames; ++sample){
                output[channel][sample] = input[channel][sample];
            }
        }
    }
};


int main() {
    // Your code goes here!

    auto jack_module = JackModule();


    jack_module.setNumInputChannels (4);
    jack_module.setNumOutputChannels (2);

    jack_module.init();
    jack_module.autoConnect();


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