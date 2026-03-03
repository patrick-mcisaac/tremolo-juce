#pragma once

namespace tremolo {
class Tremolo {
public:
  enum class LfoWaveform : size_t {
    sine = 0,
    triangle = 1,
  };

  Tremolo() {
    for (auto& lfo : lfos) {
      lfo.setFrequency(5.f, true);
    }
  }

  void prepare(double sampleRate, int expectedMaxFramesPerBlock) {
    const juce::dsp::ProcessSpec processSpec{
        .sampleRate = sampleRate,
        .maximumBlockSize =
            static_cast<juce::uint32>(expectedMaxFramesPerBlock),
        .numChannels = 1u,
    };

    for (auto& lfo : lfos) {
      lfo.prepare(processSpec);
    }

    lfoTransitionSmoother.reset(sampleRate, 0.025);
  }

  void setLfoWaveform(LfoWaveform waveform) {
    jassert(waveform == LfoWaveform::sine || waveform == LfoWaveform::triangle);
    lfoToSet = waveform;
  }

  void process(juce::AudioBuffer<float>& buffer) noexcept {
    updateLfoWaveform();
    // for each frame
    for (const auto frameIndex : std::views::iota(0, buffer.getNumSamples())) {
      // TODO: generate the LFO value
      const auto lfoValue = getNextLfoValue();

      constexpr auto modulationDepth = 0.4f;
      const auto modulationValue = modulationDepth * lfoValue + 1.f;
      // TODO: calculate the modulation value

      // for each channel sample in the frame
      for (const auto channelIndex :
           std::views::iota(0, buffer.getNumChannels())) {
        // get the input sample
        const auto inputSample = buffer.getSample(channelIndex, frameIndex);

        // TODO: modulate the sample
        const auto outputSample = inputSample * modulationValue;

        // set the output sample
        buffer.setSample(channelIndex, frameIndex, outputSample);
      }
    }
  }

  void reset() noexcept {
    for (auto& lfo : lfos) {
      lfo.reset();
    }
  }

private:
  // You should put class members and private functions here

  std::array<juce::dsp::Oscillator<float>, 2u> lfos{
      juce::dsp::Oscillator<float>{[](auto phase) { return std::sin(phase); }},
      juce::dsp::Oscillator<float>{triangle}};

  LfoWaveform currentLfo = LfoWaveform::sine;
  LfoWaveform lfoToSet = currentLfo;

  juce::SmoothedValue<Float, juce::ValueSmoothingTypes::Linear>
      lfoTransitionSmoother{0.f};

  static float triangle(float phase) {
    // ft stands for phase over time
    const auto ft = phase / juce::MathConstants<float>::twoPi;
    return 4.f * std::abs(ft - std::floor(ft + 0.5f)) - 1.f;
  }

  void updateLfoWaveform() {
    if (currentLfo != lfoToSet) {
      // Update smoother
      lfoTransitionSmoother.setCurrentAndTargetValue(getNextLfoValue());

      currentLfo = lfoToSet;

      // initiate smoothing
      lfoTransitionSmoother.setTargetValue(getNextLfoValue());
    }
  }

  float getNextLfoValue() {
    if (lfoTransitionSmoother.isSmoothing()) {
      return lfoTransitionSmoother.getNextValue();
    }
    return lfos[juce::toUnderlyingType(currentLfo)].processSample(0.f);
  }
};
}  // namespace tremolo
