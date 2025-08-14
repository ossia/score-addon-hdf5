#pragma once

#include <DataReader/HDF5Ports.hpp>
#include <DataReader/IncludeH5.hpp>

#include <halp/audio.hpp>
#include <halp/controls.hpp>
#include <halp/meta.hpp>

#include <CDSPResampler.h>
#include <memory>

namespace DataReader
{
class HDF5_AudioReader : public HDF5ObjectBase
{
public:
  halp_meta(name, "HDF5 Audio Reader")
  halp_meta(c_name, "hdf5_audio")
  halp_meta(category, "Data/HDF5")
  halp_meta(author, "ossia score")
  halp_meta(description, "Read HDF5 data as audio with resampling")
  halp_meta(uuid, "a8b4c5d6-e7f8-9012-3456-789abcdef012")

  struct inputs_t
  {
    HDF5FilePort h5;
    HDF5AccessorPort accessor;
    halp::spinbox_f32<"Position", halp::range{0., 1., 0.}> percent;
    halp::spinbox_f32<"Input Sample Rate", halp::range{1., 192000., 44100.}> input_samplerate;
    halp::spinbox_i32<"Channels", halp::range{1, 64, 1}> channels;
  } inputs;

  struct outputs_t
  {
    halp::variable_audio_bus<"Audio", double> audio;
  } outputs;

  using tick = halp::tick_musical;
  using setup = halp::setup;

  void prepare(halp::setup s)
  {
    setup_info = s;
    outputs.audio.request_channels(inputs.channels.value);
    setup_resampler(inputs.channels.value);
  }

  void setup_resampler(int channels = 1)
  {
    if (std::abs(inputs.input_samplerate.value - setup_info.rate) > 1.0)
    {
      resamplers.clear();
      for (int i = 0; i < channels; i++)
      {
        resamplers.emplace_back(std::make_unique<r8b::CDSPResampler>(
            inputs.input_samplerate.value,
            setup_info.rate,
            setup_info.frames,
            2.0,
            136.45,
            r8b::fprMinPhase));
      }
    }
    else
    {
      resamplers.clear();
    }
    last_channels = channels;
  }

  void operator()(halp::tick_musical t);

private:
  halp::setup setup_info;
  std::vector<std::unique_ptr<r8b::CDSPResampler>> resamplers;
  std::vector<double> temp_buffer;
  int last_channels = 0;
};
}