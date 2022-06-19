/*
  ==============================================================================

    AngklungFM.h
    Created: 13 Jun 2022 11:11:27pm
    Author:  Joshua

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "AudioBufferQueue.h"
#include "ScopeComponent.h"
#include "ScopeDataCollector.h"
#include "AudioEngine.h"

//==============================================================================
class AngklungFM : public juce::AudioProcessor
{
public:
    //==============================================================================
    AngklungFM()
        : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)), parameters(*this, nullptr, "Parameters", createParams())
    {
        
    }

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        audioEngine.prepare({ sampleRate, (juce::uint32)samplesPerBlock, 2 });
        midiMessageCollector.reset(sampleRate);

        time = 0;
        rate = static_cast<float> (sampleRate);
    }

    void releaseResources() override {}

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        // This is the place where you check if the layout is supported.
        // In this template code we only support mono or stereo.
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
            && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;

        return true;
    }

    void processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override
    {
        juce::ScopedNoDenormals noDenormals;
        auto totalNumInputChannels = getTotalNumInputChannels();
        auto totalNumOutputChannels = getTotalNumOutputChannels();
        

        auto numSamples = buffer.getNumSamples();
        auto& speed = *parameters.getRawParameterValue("SPEED");

        auto noteDuration = static_cast<int> (std::ceil(rate * 0.25f * (0.1f + (1.0f - (speed.load())))));   // [8]

        for ( const auto metadata : midiMessages )
        {
        const auto msg = metadata.getMessage();
        const auto noteNumber = msg.getNoteNumber();
        if (msg.isNoteOn())
        {
        noteTimeOn.insert({ noteNumber, (time + metadata.samplePosition) % noteDuration });
        noteTimeOff.insert({ noteNumber, (time + metadata.samplePosition + 1) % noteDuration });
        noteVelocity.insert({ noteNumber, msg.getVelocity() });
            
        }
        else if (msg.isNoteOff())
        {
        noteTimeOn.erase(noteNumber);
        noteTimeOff.erase(noteNumber);
        noteVelocity.erase(noteNumber);
        }
        }
        
        midiMessages.clear();
        
         
        for ( auto & note : noteTimeOn)
        {
        if ((((time + numSamples) % noteDuration - numSamples)) <= note.second && (time + numSamples > note.second))
        {
        auto offset = juce::jmax(0, juce::jmin(( int )(note.second - time), numSamples - 1));
        midiMessages .addEvent(juce:: MidiMessage ::noteOn(1, note.first, (juce:: uint8 ) noteVelocity[note.first]), offset);
        }
        }
         
        for ( auto & note : noteTimeOff)
        {
        if ((((time + numSamples) % noteDuration - numSamples)) <= note.second && (time + numSamples > note.second))
        {
        auto offset = juce::jmax(0, juce::jmin(( int )(note.second - time), numSamples - 1));
        midiMessages .addEvent(juce:: MidiMessage ::noteOff(1, note.first), offset);
        }
        }

        midiMessageCollector.removeNextBlockOfMessages(midiMessages, buffer.getNumSamples());

        for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
            buffer.clear(i, 0, buffer.getNumSamples());

        audioEngine.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
        scopeDataCollector.process(buffer.getReadPointer(0), (size_t)buffer.getNumSamples());

        for (int i = 0; i < audioEngine.getNumVoices(); i++)
        {
            if (auto voice = dynamic_cast<Voice*>(audioEngine.getVoice(i)))
            {
//                auto& modAttack = *parameters.getRawParameterValue("MODATTACK");
//                auto& modDecay = *parameters.getRawParameterValue("MODDECAY");
//                auto& modSustain = *parameters.getRawParameterValue("MODSUSTAIN");
//                auto& modRelease = *parameters.getRawParameterValue("MODRELEASE");

//                auto& carrAttack = *parameters.getRawParameterValue("CARRATTACK");
//                auto& carrDecay = *parameters.getRawParameterValue("CARRDECAY");
//                auto& carrSustain = *parameters.getRawParameterValue("CARRSUSTAIN");
                auto& carrRelease = *parameters.getRawParameterValue("CARRRELEASE");

//                auto& freqRatio = *parameters.getRawParameterValue("FMFREQRAT");
                auto& depth = *parameters.getRawParameterValue("FMDEPTH");
//                auto& cutoff = *parameters.getRawParameterValue("CUTOFF");

//                voice->update(modAttack.load(), modDecay.load(), modSustain.load(), modRelease.load(),
//                              carrAttack.load(), carrDecay.load(), carrSustain.load(), carrRelease.load(),
//                              freqRatio.load(), depth.load());
                voice->update(carrRelease.load(),
                              depth.load());
            }
        }

        time = (time + numSamples) % noteDuration;
    }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override
    {
//        return new AngklungFMAudioProcessorEditor(*this);
        return new juce::GenericAudioProcessorEditor(*this);
    }

    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    //==============================================================================
    juce::MidiMessageCollector& getMidiMessageCollector() noexcept { return midiMessageCollector; }
    AudioBufferQueue<float>& getAudioBufferQueue() noexcept { return audioBufferQueue; }

private:
//    ==============================================================================
    juce::AudioParameterFloat* speed;
    juce::AudioProcessorValueTreeState parameters;
    AudioEngine audioEngine;
    float rate;
    int time;
    std::map<int, float> noteTimeOn;
    std::map<int, float> noteTimeOff;
    std::map<int, int> noteVelocity;
    juce::MidiMessageCollector midiMessageCollector;
    AudioBufferQueue<float> audioBufferQueue;
    ScopeDataCollector<float> scopeDataCollector{ audioBufferQueue };

    juce::AudioProcessorValueTreeState::ParameterLayout createParams()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        // FM modulator
//        params.push_back(std::make_unique<juce::AudioParameterFloat>("FMFREQRAT", "FM Freq Ratio", juce::NormalisableRange<float> { 0.01f, 10.00f, 0.01f }, 3.10f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("FMDEPTH", "Brightness", juce::NormalisableRange<float> { 0.00f, 1.0f, 0.001f }, 0.154f));

        // Appregio
        params.push_back(std::make_unique<juce::AudioParameterFloat>("SPEED", "Shake frequency", juce::NormalisableRange<float> { 0.00f, 1.00f, 0.01f }, 0.85f));

        // Filter
//        params.push_back(std::make_unique<juce::AudioParameterFloat>("CUTOFF", "Cutoff Freq", juce::NormalisableRange<float> { 20.0f, 2000.0f, 1.0f }, 1000.0f));

        // Modulator ADSR
//        params.push_back(std::make_unique<juce::AudioParameterFloat>("MODATTACK", "Modulator Attack", juce::NormalisableRange<float> { 0.00f, 1.00f, 0.001f }, 0.0f));
//        params.push_back(std::make_unique<juce::AudioParameterFloat>("MODDECAY", "Modulator Decay", juce::NormalisableRange<float> { 0.00f, 1.00f, 0.01f }, 0.01f));
//        params.push_back(std::make_unique<juce::AudioParameterFloat>("MODSUSTAIN", "Modulator Sustain", juce::NormalisableRange<float> { 0.01f, 1.00f, 0.01f }, 1.0f));
//        params.push_back(std::make_unique<juce::AudioParameterFloat>("MODRELEASE", "Modulator Release", juce::NormalisableRange<float> { 0.00f, 3.00f, 0.01f }, 0.25f));
//
//        // Carrier ADSR
//        params.push_back(std::make_unique<juce::AudioParameterFloat>("CARRATTACK", "Carrier Attack", juce::NormalisableRange<float> { 0.00f, 1.00f, 0.001f }, 0.0f));
//        params.push_back(std::make_unique<juce::AudioParameterFloat>("CARRDECAY", "Carrier Decay", juce::NormalisableRange<float> { 0.00f, 1.00f, 0.01f }, 0.01f));
//        params.push_back(std::make_unique<juce::AudioParameterFloat>("CARRSUSTAIN", "Carrier Sustain", juce::NormalisableRange<float> { 0.01f, 1.00f, 0.01f }, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("CARRRELEASE", "Release Time", juce::NormalisableRange<float> { 0.00f, 1.00f, 0.001f }, 0.5f));

        return { params.begin(), params.end() };
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AngklungFM)
};
