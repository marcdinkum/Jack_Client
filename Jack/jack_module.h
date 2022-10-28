
#pragma once

#include "ring_buffer.h"
#include <jack/jack.h>
#include <string>
#include <string_view>

class JackModule {
public:
    using SampleType = jack_default_audio_sample_t;

    JackModule();
    JackModule (uint16_t inputBufSize, uint64_t inputBufSize);
    ~JackModule();

    void setNumInputChannels (int n);
    void setNumOutputChannels (int n);
    int init();
    int init (std::string_view clientName);
    unsigned long getSamplerate();
    void autoConnect();
    void autoConnect (std::string_view inputClient, std::string_view outputClient);
    uint64_t readSamples (float*, uint64_t);
    uint64_t writeSamples (float*, uint64_t);
    void end();

private:
    static int _wrap_jack_process_cb (jack_nframes_t numFrames, void* self);
    std::vector<jack_port_t*> inputPorts;
    std::vector<jack_port_t*> outputPorts;

    std::vector<SampleType*> inputBuffers;
    std::vector<SampleType*> outputBuffers;

    std::array<SampleType, 10000> tempBuffer { 0.0 };  // FIXME // find acually size??
    int onProcess (jack_nframes_t numFrames);
    int numberOfInputChannels = 2;
    int numberOfOutputChannels = 2;
    jack_client_t* client;
    RingBuffer<SampleType> inputRingBuffer;
    RingBuffer<SampleType> outputRingBuffer;

    static constexpr auto DEFAULT_IN_RINGBUF_SIZE = 30000;
    static constexpr auto DEFAULT_OUT_RINGBUF_SIZE = 30000;
    static constexpr auto MAX_INPUT_CHANNELS = 2;
    static constexpr auto MAX_OUTPUT_CHANNELS = 2;
};


#include <iostream>
#include <mutex>
#include <sstream>
#include <unistd.h>

#include "jack_module.h"

// prototypes & globals
static void jack_shutdown (void*);


JackModule::JackModule() : JackModule (DEFAULT_IN_RINGBUF_SIZE, DEFAULT_OUT_RINGBUF_SIZE) {}

JackModule::JackModule (uint64_t inputBufSize, uint64_t outputBufSize) : inputRingBuffer (inputBufSize, "in"),
                                                                         outputRingBuffer (outputBufSize, "out") {
    inputRingBuffer.popMayBlock (true);
    inputRingBuffer.setBlockingNapMicroSeconds (500);
    outputRingBuffer.pushMayBlock (true);
    outputRingBuffer.setBlockingNapMicroSeconds (500);
}


JackModule::~JackModule() {
    end();
}


int JackModule::init() {
    return init ("JackModule");
}


int JackModule::init (std::string clientName) {
    client = jack_client_open (clientName.c_str(), JackNoStartServer, nullptr);

    if (! client) {
        throw std::runtime_error { "JACK server not running" };
    }

    jack_on_shutdown (client, jack_shutdown, nullptr);
    jack_set_process_callback (client, _wrap_jack_process_cb, this);

    inputPorts.clear();
    for (int channel = 0; channel < numberOfInputChannels; channel++) {
        const auto name = "input_" + std::to_string (channel + 1);
        const auto port = jack_port_register (client, name.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, nullptr);
        inputPorts.push_back (port);
    }

    outputPorts.clear();
    for (int channel = 0; channel < numberOfOutputChannels; channel++) {
        const auto name = "output_" + std::to_string (channel + 1);
        const auto port = jack_port_register (client, name.c_, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, nullptr);
        outputPorts.push_back (port);
    }

    // create buffer arrays of void pointers to prevent memory allocation inside
    // the process loop

    inputbuffer.resize (numberOfInputChannels);
    outputbuffer.resize (numberOfOutputChannels);

    if (jack_activate (client)) {
        std::runtime_error { "cannot activate client" };
    }
}


int JackModule::_wrap_jack_process_cb (jack_nframes_t numFrames, void* self) {
    return ((JackModule*) self)->onProcess (numFrames);
}

int JackModule::onProcess (jack_nframes_t numFrames) {
    for (auto channel = 0; channel < numberOfInputChannels; ++channel) {
        inputbuffer[channel] = (SampleType*) jack_port_get_buffer (inputPorts[channel], numFrames);
    }

    for (auto channel = 0; channel < numberOfOutputChannels; ++channel) {
        outputbuffer[channel] = (SampleType*) jack_port_get_buffer (outputPorts[channel], numFrames);
    }

    // push input samples from JACK channel buffers to the input ringbuffer
    // interleave the samples before writing them to the ringbuffer
    for (auto frame = 0; frame < static_cast<int> (numFrames); ++frame) {
        for (auto channel = 0; channel < numberOfInputChannels; ++channel) {
            tempBuffer[frame * numberOfInputChannels + channel] = inputBuffers[channel][frame];
        }
    }

    const auto framesPushed = inputRingBuffer->push ((SampleType*) tempbuffer, numFrames * numberOfInputChannels);
    assert (framesPushed >= numFrames * numberOfInputChannels);

    const auto framesPopped = outputRingBuffer.pop ((SampleType*) tempbuffer, numFrames * numberOfOutputChannels);
    assert (framesPopped >= numFrames * numberOfOutputChannels);

    // pop samples from output ringbuffer into JACK channel buffers
    // de-interleave the samples from the ringbuffer and write them to the
    // appropriate JACK output buffers
    for (auto frame = 0; frame < static_cast<int> (numFrames); ++frame) {
        for (auto channel = 0; channel < numberOfOutputChannels; ++channel) {
            outputBuffers[channel][frame] = tempbuffer[frame * numberOfOutputChannels + channel];
        }
    }

    return 0;
}


void JackModule::setNumInputChannels (int n) {
    if (n > MAX_INPUT_CHANNELS || n < 0) {
        throw std::runtime_error { "invalid number of input channels" };
    }
    numberOfInputChannels = n;
}


void JackModule::setNumOutputChannels (int n) {
    if (n > MAX_OUTPUT_CHANNELS) {
        throw std::runtime_error { "invalid number of output channels" };
    }
    numberOfOutputChannels = n;
}


unsigned long JackModule::getSamplerate() {
    return jack_get_sample_rate (client);
}


void JackModule::autoConnect() {
    autoConnect ("system", "system");
}

void JackModule::autoConnect (std::string_view inputClient, std::string_view outputClient) {
    if (numberOfInputChannels > 0) {
        auto ports = std::unique_ptr {
            jack_get_ports (client, inputClient.data(), nullptr, JackPortIsOutput),
            [](jack_port_t* p) { free(p); }
        };

        if (ports == nullptr) {
            std::cout << "Cannot find capture ports associated with " << inputClient << ", trying 'system'." << std::endl;
            ports = jack_get_ports (client, "system", nullptr, JackPortIsOutput);
            if (ports == nullptr) {
                std::cout << "Cannot find system capture ports. Continuing without inputs." << std::endl;
                // both attempts failed, continue without capture ports
                numberOfInputChannels = 0;
            }  // if fallback not found
        }      // if specified not found

        // find out the number of (not-null) ports on the source client
        auto numInputPorts = 0;
        while (ports[numInputPorts])
            ++numInputPorts;
        std::cout << "Source client has " << numInputPorts << " ports" << std::endl;

        auto inputPortIndex = 0;
        for (int channel = 0; channel < numberOfInputChannels; channel++) {
            std::cout << "connect input channel " << channel << std::endl;
            if (jack_connect (client, ports[inputPortIndex], jack_port_name (input_port[channel]))) {
                std::cout << "Cannot connect input ports" << std::endl;
            }
            ++inputPortIndex;
            if (numInputPorts > 0) inputPortIndex %= numInputPorts;
        }
    }

    /*
   * Try to auto-connect our output to the input of another client, so we
   * regard this as an output from our perspective
   */
    if (numberOfOutputChannels > 0) {
        ports = jack_get_ports (client, outputClient.c_str(), NULL, JackPortIsInput);
        if (ports == NULL) {
            std::cout << "Cannot find output ports associated with " << outputClient << ", trying 'system'." << std::endl;
            // try "system"
            ports = jack_get_ports (client, "system", NULL, JackPortIsInput);
            if (ports == NULL) {
                std::cout << "Cannot find system output ports. Continuing without outputs." << std::endl;
                // both attempts failed, continue without output port
                numberOfOutputChannels = 0;
            }  // if fallback not found
        }      // if specified not found

        // find out the number of (not-null) ports on the sink client
        int nrofoutputports = 0;
        while (ports[nrofoutputports])
            ++nrofoutputports;
        std::cout << "Sink client has " << nrofoutputports << " ports " << std::endl;

        int outputportindex = 0;
        for (int channel = 0; channel < numberOfOutputChannels; channel++) {
            std::cout << "connect output channel " << channel << std::endl;
            if (jack_connect (client, jack_port_name (output_port[outputportindex]), ports[channel])) {
                std::cout << "Cannot connect output ports" << std::endl;
            }
            ++outputportindex;
            if (nrofoutputports > 0) outputportindex %= nrofoutputports;
        }  // for channel

        free (ports);  // ports structure no longer needed
    }

}


void JackModule::end() {
    jack_deactivate (client);
    for (int channel = 0; channel < numberOfInputChannels; channel++)
        jack_port_disconnect (client, input_port[channel]);
    for (int channel = 0; channel < numberOfOutputChannels; channel++)
        jack_port_disconnect (client, output_port[channel]);
}


unsigned long JackModule::readSamples (float* ptr, unsigned long nrofsamples) {
    // pop samples from JACK inputbuffer and hand over to the caller
    return inputRingBuffer.pop (ptr, nrofsamples);
}


unsigned long JackModule::writeSamples (float* ptr, unsigned long nrofsamples) {
    // push samples from the caller to the JACK outputbuffer
    return outputRingBuffer.push (ptr, nrofsamples);
}


/*
 * shutdown callback may be called by JACK
 */
static void jack_shutdown (void* arg) {
    exit (1);
}
