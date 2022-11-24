
#define ENABLE_JACK_BACKEND
// #define ENABLE_PORT_AUDIO_BACKEND

#include "../Backend/audio_backend.h"
#include <cmath>
#include <iostream>
#include <thread>
#include <vector>

// ================================================================================

class SampleHistory {
public:
    void setSize (const int size) {
        buffer.resize (size, 0.0);
    }

    void write (const float sample) {
        buffer[writePosition] = sample;
        writePosition = getPositionAfter (writePosition);
    }

    float lookBack (int numSamples) const {
        return buffer[subtractFromPosition (writePosition, numSamples)];
    }

private:
    std::vector<float> buffer {};
    int writePosition = 0;

    int subtractFromPosition (int position, int toSubtract) const {
        const auto newPos = position - toSubtract;
        if (newPos < 0) {
            return newPos + (int) buffer.size();
        }
        return newPos;
    }

    int getPositionAfter (int position) const {
        return (position + 1) % (int) buffer.size();
    }
};

// ================================================================================

class RMSAnalyzer {
public:
    explicit RMSAnalyzer (int batchSize) : batchSize (batchSize) {
        sampleHistory.setSize (batchSize + 1);
    }

    void analyze (float input) {
        sampleHistory.write (input);

        auto sum = 0.0f;
        for (auto i = 0; i < batchSize; ++i) {
            const auto sample = sampleHistory.lookBack (i);
            sum += sample * sample;
        }

        const auto rms = amplitudeToDecibels (std::sqrt (sum / (float) batchSize));
        currentValue.store (rms);
    }

    float getCurrentValue() const {
        return currentValue.load();
    }

private:
    SampleHistory sampleHistory;
    int batchSize;
    std::atomic<float> currentValue;

    static float amplitudeToDecibels (float gain) {
        constexpr float MINUS_INFINITY_DB = -100.0;
        if (std::abs (gain) > 0.0) {
            return std::max (MINUS_INFINITY_DB, std::log10 (std::abs (gain)) * 20.0f);
        } else {
            return MINUS_INFINITY_DB;
        }
    }
};

// ================================================================================

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
    auto analyzer = RMSAnalyzer { 256 };
    auto callback = Callback { analyzer };
    auto backend = AudioBackend { callback };

    backend.init (2, 2);

    while (true) {
        std::cout << "rms: " << analyzer.getCurrentValue() << "db\n";
        std::this_thread::sleep_for (std::chrono::seconds { 1 });
    }
}