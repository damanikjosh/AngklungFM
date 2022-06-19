/*
  ==============================================================================

    ADSR.h
    Created: 14 Jun 2022 12:23:28am
    Author:  Joshua

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
template <typename Type>
class CustomADSR : public juce::ADSR
{
public:
    //==============================================================================
    CustomADSR()
    {
        
    }


    //==============================================================================
    template <typename ProcessContext>
    void process(const ProcessContext& context) noexcept
    {
        
    }

    //==============================================================================
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        setSampleRate(spec.sampleRate);

        adsrParams.attack = 1.0f;
        adsrParams.decay = 0.1f;
        adsrParams.sustain = 1.0f;
        adsrParams.release = 1.0f;

        setParameters(adsrParams);
    }

private:
    //==============================================================================
    juce::ADSR::Parameters adsrParams;

};