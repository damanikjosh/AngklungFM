/*
  ==============================================================================

    CustomOscillator.h
    Created: 13 Jun 2022 11:08:43pm
    Author:  Joshua

  ==============================================================================
*/

#pragma once
#include "FMSynth.h"

//==============================================================================
template <typename Type>
class CustomOscillator
{
public:

    //==============================================================================
    void setFrequency(Type newValue, bool force = false)
    {
        carrChain.template get<oscIndex>().setFrequency(newValue, force);
    }

    //==============================================================================
    void setLevel(Type newValue)
    {
        carrChain.template get<gainIndex>().setGainLinear(newValue);
    }

    void setParameter(Type ratio, Type depth)
    {
        modChain.template get<oscIndex>().setFrequency(ratio * carrChain.template get<oscIndex>().getFrequency());
        modChain.template get<gainIndex>().setGainLinear(depth);
    }

    //==============================================================================
    void reset() noexcept
    {
        modChain.reset(); // [4]
        carrChain.reset();
    }

    //==============================================================================
    template <typename ProcessContext>
    void process(const ProcessContext& context) noexcept
    {
        modChain.process(context);
        carrChain.process(context);
    }

    //==============================================================================
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        modChain.prepare(spec); // [3]
        carrChain.prepare(spec);
    }

private:
    //==============================================================================
    enum
    {
        oscIndex,
        gainIndex
    };

    juce::dsp::ProcessorChain<FMSynth<Type>, juce::dsp::Gain<Type>> modChain; // [1]
    juce::dsp::ProcessorChain<FMSynth<Type>, juce::dsp::Gain<Type>> carrChain;
};