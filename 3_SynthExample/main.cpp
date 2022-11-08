
#include "../Jack/jack_module.h"
#include <cmath>
#include <iostream>
#include <thread>
#include <vector>


class Callback : public AudioCallback {
public:
    Callback (RMSAnalyzer& analyzer) : analyzer (analyzer) {}

    void process (AudioBuffer buffer) override {
        for (auto sample = 0; sample < buffer.numFrames; ++sample) {
            const auto inputSample = buffer.inputChannels[0][sample];
            analyzer.analyze (inputSample);

            for (auto channel = 0; channel < buffer.numOutputChannels; ++channel) {
                buffer.outputChannels[channel][sample] = inputSample;
            }
        }
    }

private:
    RMSAnalyzer& analyzer;
};

// ================================================================================

int main() {
    auto callback = Callback {  };
    auto jackModule = JackModule { callback };

    jackModule.init (2, 2);

    auto running = true;
    while (running) {

}