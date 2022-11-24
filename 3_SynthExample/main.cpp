#define USE_PORT_AUDIO_BACKEND
#include "../Jack/jack_module.h"
#include "SimpleSynth.h"


class Callback : public AudioCallback {
public:
    Callback (SimpleSynth& synth) : synth (synth) {}

    void prepare (int sampleRate) override {
        synth.prepare (sampleRate);
        synth.setPitch(60);
    }

    void process (AudioBuffer buffer) override {
        for (auto sample = 0; sample < buffer.numFrames; ++sample) {
            for (auto channel = 0; channel < buffer.numOutputChannels; ++channel) {
                buffer.outputChannels[channel][sample] = synth.output();
            }
        }
    }

private:
    SimpleSynth& synth;
};

// ================================================================================

int main() {
    auto simpleSynth = SimpleSynth {};
    auto callback = Callback { simpleSynth };
    auto jackModule = AudioBackend { callback };

    jackModule.init (0, 2);

    auto running = true;
    while (running) {
    }
}