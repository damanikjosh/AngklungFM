/*
  ==============================================================================

    Voice.h
    Created: 13 Jun 2022 11:09:37pm
    Author:  Joshua

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CustomOscillator.h"
#include "CustomADSR.h"
#include "FMSynth.h"

//==============================================================================
class Voice : public juce::MPESynthesiserVoice
{
public:
    //Voice()
    //{
    //    //auto& masterGain = processorChain.get<masterGainIndex>();
    //    //masterGain.setGainLinear(0.7f);

    //    //auto& filter = processorChain.get<filterIndex>();
    //    //filter.setCutoffFrequencyHz(1000.0f);
    //    //filter.setResonance(0.7f);
    //}

    //==============================================================================
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        tempBlock = juce::dsp::AudioBlock<float>(heapBlock, spec.numChannels, spec.maximumBlockSize);
        mod1Chain.prepare(spec);
        carr1Chain.prepare(spec);

        mod2Chain.prepare(spec);
        carr2Chain.prepare(spec);
        //auto a = spec.sampleRate;

        modAdsr.setSampleRate(spec.sampleRate);
        carrAdsr.setSampleRate(spec.sampleRate);
    }

    //==============================================================================
    void noteStarted() override
    {
        auto velocity = getCurrentlyPlayingNote().noteOnVelocity.asUnsignedFloat();
        auto freqHz = (float)getCurrentlyPlayingNote().getFrequencyInHertz();

        lastNoteFreqHz = freqHz;
        mod1Chain.get<oscIndex>().reset();
        carr1Chain.get<oscIndex>().reset();
        carr1Chain.get<oscIndex>().setFrequency(freqHz);
        carr1Chain.get<gainIndex>().setGainLinear(0.1*velocity);
        
        mod2Chain.get<oscIndex>().reset();
        carr2Chain.get<oscIndex>().reset();
        carr2Chain.get<oscIndex>().setFrequency(2*freqHz);
        carr2Chain.get<gainIndex>().setGainLinear(0.1*velocity);

        //modulator.setFrequency(fmFreqRatio * freqHz, true);
        //processorChain.get<osc1Index>().setFrequency(freqHz + fmMod, true);
        //processorChain.get<osc1Index>().setLevel(velocity);

        

        //processorChain.get<osc2Index>().setFrequency(freqHz * 2.0f, true);
        //processorChain.get<osc2Index>().setLevel(velocity);

        carrAdsr.noteOn();
        modAdsr.noteOn();

        //processorChain.get<osc3Index>().setFrequency(freqHz * 0.99f, true);
        //processorChain.get<osc3Index>().setLevel(velocity);
    }

    //==============================================================================
    void notePitchbendChanged() override
    {
        auto freqHz = (float)getCurrentlyPlayingNote().getFrequencyInHertz();
        
        mod1Chain.get<oscIndex>().reset();
        mod2Chain.get<oscIndex>().reset();
        
        carr1Chain.get<oscIndex>().setFrequency(freqHz);
        carr2Chain.get<oscIndex>().setFrequency(2*freqHz);
        //processorChain.get<osc2Index>().setFrequency(freqHz * 2.0f);
        //processorChain.get<osc3Index>().setFrequency(freqHz * 0.99f);
    }

    //==============================================================================
    void noteStopped(bool) override
    {
        modAdsr.noteOff();
        carrAdsr.noteOff();
        //clearCurrentNote();
    }

    //==============================================================================
    void notePressureChanged() override {}
    void noteTimbreChanged()   override {}
    void noteKeyStateChanged() override {}

    //==============================================================================
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {

        synth1Buffer.setSize(outputBuffer.getNumChannels(), numSamples, false, false, true);
        synth1Buffer.clear();

        juce::dsp::AudioBlock<float> output1{ synth1Buffer };

        mod1Chain.process(juce::dsp::ProcessContextReplacing<float>(output1));
        modAdsr.applyEnvelopeToBuffer(synth1Buffer, 0, synth1Buffer.getNumSamples());

        output1.copyFrom(synth1Buffer);
        carr1Chain.process(juce::dsp::ProcessContextReplacing<float>(output1));
        carrAdsr.applyEnvelopeToBuffer(synth1Buffer, 0, synth1Buffer.getNumSamples());


        synth2Buffer.setSize(outputBuffer.getNumChannels(), numSamples, false, false, true);
        synth2Buffer.clear();

        juce::dsp::AudioBlock<float> output2{ synth2Buffer };

        mod2Chain.process(juce::dsp::ProcessContextReplacing<float>(output2));
        modAdsr.applyEnvelopeToBuffer(synth2Buffer, 0, synth2Buffer.getNumSamples());

        output2.copyFrom(synth2Buffer);
        carr2Chain.process(juce::dsp::ProcessContextReplacing<float>(output2));
        carrAdsr.applyEnvelopeToBuffer(synth2Buffer, 0, synth2Buffer.getNumSamples());

        for (int channel = 0; channel < outputBuffer.getNumChannels(); channel++)
        {
            outputBuffer.addFrom(channel, startSample, synth1Buffer, channel, 0, numSamples);
            outputBuffer.addFrom(channel, startSample, synth2Buffer, channel, 0, numSamples);

            if (!modAdsr.isActive() && !carrAdsr.isActive())
            {
                clearCurrentNote();
                
            }
        }
    }

    void update(const float carrRelease,
                const float depth)
    {
        modAdsrParams.attack = 0.0f;
        modAdsrParams.decay = 0.0f;
        modAdsrParams.sustain = 1.0f;
        modAdsrParams.release = carrRelease;

        modAdsr.setParameters(modAdsrParams);

        carrAdsrParams.attack = 0.0f;
        carrAdsrParams.decay = 0.0f;
        carrAdsrParams.sustain = 1.0f;
        carrAdsrParams.release = carrRelease;

        carrAdsr.setParameters(carrAdsrParams);
        
        const float freqRatio = 3.1f;

        mod1Chain.get<oscIndex>().setFrequency(freqRatio * carr1Chain.get<oscIndex>().getFrequency());
        mod1Chain.get<gainIndex>().setGainLinear(depth);

        mod2Chain.get<oscIndex>().setFrequency(freqRatio * carr2Chain.get<oscIndex>().getFrequency());
        mod2Chain.get<gainIndex>().setGainLinear(depth);
        //processorChain.get<filterIndex>().setCutoffFrequencyHz(cutoff);
        //fmDepth = depth;
        //processorChain.get<osc1Index>().setFrequency(lastNoteFreqHz + fmMod);
    }

private:
    //==============================================================================
    juce::HeapBlock<char> heapBlock;
    juce::dsp::AudioBlock<float> tempBlock;

    juce::AudioBuffer<float> synth1Buffer;
    juce::AudioBuffer<float> synth2Buffer;

    enum
    {
        oscIndex,
        gainIndex
    };

    juce::dsp::ProcessorChain<FMSynth<float>, juce::dsp::Gain<float>> mod1Chain; // [1]
    juce::dsp::ProcessorChain<FMSynth<float>, juce::dsp::Gain<float>> carr1Chain;

    juce::dsp::ProcessorChain<FMSynth<float>, juce::dsp::Gain<float>> mod2Chain; // [1]
    juce::dsp::ProcessorChain<FMSynth<float>, juce::dsp::Gain<float>> carr2Chain;

    static constexpr size_t lfoUpdateRate = 100;
    size_t lfoUpdateCounter = lfoUpdateRate;
   

    float lastNoteFreqHz;

    juce::ADSR modAdsr;
    juce::ADSR carrAdsr;

    juce::ADSR::Parameters modAdsrParams;
    juce::ADSR::Parameters carrAdsrParams;
};
