/**********************************************************************
 *          Copyright (c) 2022, Hogeschool voor de Kunsten Utrecht
 *                      Hilversum, the Netherlands
 *                          All rights reserved
 ***********************************************************************
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.
 *  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 *
 *  File name     : jack_module.h
 *  System name   : jack_module
 *
 *  Description   : C++ abstraction for JACK Audio Connection Kit
 *
 *
 *  Authors       : Marc Groenewegen,
 *                  Wouter Ensink,
 *                  Daan Schrier
 *  E-mails       : marc.groenewegen@hku.nl,
 *                  wouter.ensink@student.hku.nl,
 *                  Daan@Daansdotorg.wordpress.com
 *
 **********************************************************************/


#pragma once

#include "ring_buffer.h"
#include <array>
#include <iostream>
#include <jack/jack.h>
#include <sstream>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>


class JackModule {
public:
    using SampleType = jack_default_audio_sample_t;

    JackModule();
    JackModule (uint64_t inputBufSize, uint64_t outputBufSize);
    ~JackModule();

    void setNumInputChannels (int n);
    void setNumOutputChannels (int n);
    void init();
    void init (std::string_view clientName);

    uint64_t getSampleRate() {
        return jack_get_sample_rate (client);
    }

    void autoConnect() {
        autoConnect ("system", "system");
    }

    void autoConnect (std::string_view inputClient, std::string_view outputClient);

    uint64_t readSamples (float* target, uint64_t numSamples) {
        return inputRingBuffer.pop (target, numSamples);
    }

    uint64_t writeSamples (float* source, uint64_t numSamples) {
        return outputRingBuffer.pop (source, numSamples);
    }

    void end(){
        jack_deactivate (client);

        for (auto port : inputPorts)
            jack_port_disconnect (client, port);

        for (auto port : outputPorts)
            jack_port_disconnect (client, port);
    }

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


void JackModule::init() {
    return init ("JackModule");
}


void JackModule::init (std::string_view clientName) {
    client = jack_client_open (clientName.data(), JackNoStartServer, nullptr);

    if (! client) {
        throw std::runtime_error { "JACK server not running" };
    }

    jack_on_shutdown (client, jack_shutdown, nullptr);
    jack_set_process_callback (client, _wrap_jack_process_cb, this);

    inputPorts.clear();
    for (auto channel = 0; channel < numberOfInputChannels; ++channel) {
        const auto name = "input_" + std::to_string (channel + 1);
        const auto port = jack_port_register (client, name.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        inputPorts.push_back (port);
    }

    outputPorts.clear();
    for (auto channel = 0; channel < numberOfOutputChannels; ++channel) {
        const auto name = "output_" + std::to_string (channel + 1);
        const auto port = jack_port_register (client, name.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        outputPorts.push_back (port);
    }

    // create buffer arrays of void pointers to prevent memory allocation inside
    // the process loop

    inputBuffers.resize (numberOfInputChannels);
    outputBuffers.resize (numberOfOutputChannels);

    if (jack_activate (client)) {
        throw std::runtime_error { "cannot activate client" };
    }
}


int JackModule::_wrap_jack_process_cb (jack_nframes_t numFrames, void* self) {
    return ((JackModule*) self)->onProcess (numFrames);
}

int JackModule::onProcess (jack_nframes_t numFrames) {
    for (auto channel = 0; channel < numberOfInputChannels; ++channel) {
        inputBuffers[channel] = (SampleType*) jack_port_get_buffer (inputPorts[channel], numFrames);
    }

    for (auto channel = 0; channel < numberOfOutputChannels; ++channel) {
        outputBuffers[channel] = (SampleType*) jack_port_get_buffer (outputPorts[channel], numFrames);
    }

    // push input samples from JACK channel buffers to the input ringbuffer
    // interleave the samples before writing them to the ringbuffer
    for (auto frame = 0; frame < static_cast<int> (numFrames); ++frame) {
        for (auto channel = 0; channel < numberOfInputChannels; ++channel) {
            tempBuffer[frame * numberOfInputChannels + channel] = inputBuffers[channel][frame];
        }
    }

    const auto framesPushed = inputRingBuffer.push ((SampleType*) tempBuffer, numFrames * numberOfInputChannels);
    assert (framesPushed >= numFrames * numberOfInputChannels);

    const auto framesPopped = outputRingBuffer.pop ((SampleType*) tempbuffer, numFrames * numberOfOutputChannels);
    assert (framesPopped >= numFrames * numberOfOutputChannels);

    // pop samples from output ringbuffer into JACK channel buffers
    // de-interleave the samples from the ringbuffer and write them to the
    // appropriate JACK output buffers
    for (auto frame = 0; frame < static_cast<int> (numFrames); ++frame) {
        for (auto channel = 0; channel < numberOfOutputChannels; ++channel) {
            outputBuffers[channel][frame] = tempBuffer[frame * numberOfOutputChannels + channel];
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
    if (n > MAX_OUTPUT_CHANNELS || n < 0) {
        throw std::runtime_error { "invalid number of output channels" };
    }
    numberOfOutputChannels = n;
}


void JackModule::autoConnect (std::string_view inputClient, std::string_view outputClient) {
    if (numberOfInputChannels > 0) {
        auto ports = std::unique_ptr {
            jack_get_ports (client, inputClient.data(), nullptr, JackPortIsOutput),
            [] (jack_port_t* p) { free (p); }
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
        ports = jack_get_ports (client, outputClient.data(), nullptr, JackPortIsInput);
        if (ports == nullptr) {
            std::cout << "Cannot find output ports associated with " << outputClient << ", trying 'system'." << std::endl;
            // try "system"
            ports = jack_get_ports (client, "system", nullptr, JackPortIsInput);
            if (ports == nullptr) {
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

}  // autoconnect() <- end of functie (autoconnect functie) [end]


void JackModule::end() {

}



static void jack_shutdown ([[maybe_unused]] void* arg) {
    exit (1);
}
