
#include "../Jack/jack_module.h"
#include <cmath>
#include <iostream>


class CustomCallback : public AudioCallback {
public:
    void process (AudioBuffer buffer) override {
        for (int i = 0; i < buffer.numFrames; ++i) {
            phase += frequency / sampleRate;
            // write sample to buffer at channel 0
            buffer.outputChannels[0][i] = sin (phase * pi * 2.0f);
        }
    }

private:
    const float pi = acos (-1);  //atan(1) * 4; <-- vak van Pieter.
    float phase = 0;
    float frequency = 440;
    float sampleRate = 48000;
};

// ================================================================================

int main() {
    auto callback = CustomCallback {};
    auto jackModule = JackModule { callback };

    jackModule.init (0, 1);

    bool running = true;
    while (running) {
        switch (std::cin.get()) {
            case 'q':
                running = false;
        }
    }
}