/*
  ==============================================================================

    FMSynth.h
    Created: 14 Jun 2022 5:34:37pm
    Author:  Joshua

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

template <typename Type>
class FMSynth
{
public:

    //==============================================================================
    void setFrequency(Type newValue, bool force = false)
    {
        oscFreq = newValue;
    }

    Type getFrequency()
    {
        return oscFreq;
    }

    //=============================================================================

    //==============================================================================
    void reset() noexcept
    {
        oscPhase = 0.00;
    }

    //==============================================================================
    template <typename ProcessContext>
    void process(const ProcessContext& context) noexcept
    {
        auto&& outBlock = context.getOutputBlock();
        auto&& inBlock = context.getInputBlock();

        auto len = outBlock.getNumSamples();
        auto numChannels = outBlock.getNumChannels();
        auto inputChannels = inBlock.getNumChannels();
        auto baseIncrement = MathConstants<Type>::twoPi / sampleRate;

        double nextoscPhase = oscPhase;

        for (size_t ch = 0; ch < jmin(numChannels, inputChannels); ch++)
        {
            auto* dst = outBlock.getChannelPointer(ch);
            auto* src = inBlock.getChannelPointer(ch);
            
            double curroscPhase = oscPhase;

            for (size_t i = 0; i < len; i++)
            {
                curroscPhase += baseIncrement * oscFreq + src[i];
                dst[i] = std::sin(curroscPhase);
            }

            nextoscPhase = curroscPhase;
        }

        oscPhase = nextoscPhase;

    }

    //==============================================================================
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
    }
    
private:
    Type oscPhase = 0.00;
    Type oscFreq = 200.0;
    Type sampleRate;
};
