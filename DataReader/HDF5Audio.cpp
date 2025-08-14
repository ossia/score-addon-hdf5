#include "HDF5Audio.hpp"

#include <highfive/boost.hpp>
#include <highfive/highfive.hpp>
#include <algorithm>
#include <cmath>

namespace DataReader
{

void HDF5_AudioReader::operator()(halp::tick_musical t)
{
  if (!dataset->isValid())
    return;

  if (dataset_dims.size() < 1)
    return;

  // Update channels if needed
  const int requested_channels = inputs.channels.value;
  if (outputs.audio.channels != requested_channels)
  {
    outputs.audio.request_channels(requested_channels);
    return;
  }

  // Setup resampler if sample rate changed or channel count changed
  if ((std::abs(inputs.input_samplerate.value - setup_info.rate) > 1.0 && resamplers.empty()) ||
      (last_channels != requested_channels))
  {
    setup_resampler(requested_channels);
  }
  else if (std::abs(inputs.input_samplerate.value - setup_info.rate) <= 1.0 && !resamplers.empty())
  {
    resamplers.clear();
  }

  // Determine data layout based on dimensions
  int n_frames = dataset_dims[0];
  int n_channels = 1;
  
  // Support different data layouts:
  // 1D: [time] - mono
  // 2D: [time, channels] - multi-channel
  // 2D: [channels, time] - multi-channel (alternative layout)
  if (dataset_dims.size() >= 2)
  {
    // Assume [time, channels] layout by default
    n_channels = std::min((int)dataset_dims[1], requested_channels);
  }

  if (n_frames == 0)
    return;

  // Calculate position in the dataset
  int start_frame = std::clamp(
      (int)(inputs.percent.value * (n_frames - t.frames)), 
      0, 
      n_frames - t.frames);

  int frames_to_read = std::min(t.frames, n_frames - start_frame);

  // Read data from HDF5
  thread_local boost::multi_array<double, 2> hdf5_data;
  thread_local boost::multi_array<double, 1> hdf5_data_1d;

  try
  {
    if (dataset_dims.size() == 1)
    {
      // 1D data - mono
      dataset->select({(unsigned)start_frame}, {(unsigned)frames_to_read})
          .read(hdf5_data_1d);
      
      // Copy to first channel
      if (outputs.audio.channels > 0)
      {
        auto out_channel = outputs.audio.channel(0, t.frames);
        
        if (!resamplers.empty() && std::abs(inputs.input_samplerate.value - setup_info.rate) > 1.0)
        {
          // Resample the data
          double* resampled_ptr;
          int resampled_frames = resamplers[0]->process(hdf5_data_1d.data(), frames_to_read, resampled_ptr);
          
          // Copy resampled data
          int copy_frames = std::min(resampled_frames, t.frames);
          std::copy_n(resampled_ptr, copy_frames, out_channel.data());
          
          // Zero-pad if needed
          if (copy_frames < t.frames)
            std::fill(out_channel.data() + copy_frames, out_channel.data() + t.frames, 0.0);
        }
        else
        {
          // Direct copy
          std::copy_n(hdf5_data_1d.data(), frames_to_read, out_channel.data());
          
          // Zero-pad if needed
          if (frames_to_read < t.frames)
            std::fill(out_channel.data() + frames_to_read, out_channel.data() + t.frames, 0.0);
        }
        
        // Copy to other channels (mono -> multichannel)
        for (int ch = 1; ch < outputs.audio.channels; ch++)
        {
          auto dest_channel = outputs.audio.channel(ch, t.frames);
          std::copy_n(out_channel.data(), t.frames, dest_channel.data());
        }
      }
    }
    else
    {
      // 2D data - potentially multi-channel
      dataset->select({(unsigned)start_frame, 0}, {(unsigned)frames_to_read, (unsigned)n_channels})
          .read(hdf5_data);

      // Process each channel
      for (int ch = 0; ch < std::min(n_channels, outputs.audio.channels); ch++)
      {
        auto out_channel = outputs.audio.channel(ch, t.frames);
        
        if (!resamplers.empty() && ch < resamplers.size() && std::abs(inputs.input_samplerate.value - setup_info.rate) > 1.0)
        {
          // Prepare temporary buffer for this channel
          if (temp_buffer.size() < frames_to_read)
            temp_buffer.resize(frames_to_read);
            
          // Extract channel data
          for (int i = 0; i < frames_to_read; i++)
            temp_buffer[i] = hdf5_data[i][ch];
          
          // Resample
          double* resampled_ptr;
          int resampled_frames = resamplers[ch]->process(temp_buffer.data(), frames_to_read, resampled_ptr);
          
          // Copy resampled data
          int copy_frames = std::min(resampled_frames, t.frames);
          std::copy_n(resampled_ptr, copy_frames, out_channel.data());
          
          // Zero-pad if needed
          if (copy_frames < t.frames)
            std::fill(out_channel.data() + copy_frames, out_channel.data() + t.frames, 0.0);
        }
        else
        {
          // Direct copy
          for (int i = 0; i < frames_to_read; i++)
            out_channel[i] = hdf5_data[i][ch];
          
          // Zero-pad if needed
          if (frames_to_read < t.frames)
            std::fill(out_channel.data() + frames_to_read, out_channel.data() + t.frames, 0.0);
        }
      }
      
      // Zero remaining channels if we have fewer HDF5 channels than requested
      for (int ch = n_channels; ch < outputs.audio.channels; ch++)
      {
        auto out_channel = outputs.audio.channel(ch, t.frames);
        std::fill_n(out_channel.data(), t.frames, 0.0);
      }
    }
  }
  catch (const std::exception& e)
  {
    // On error, output silence
    for (int ch = 0; ch < outputs.audio.channels; ch++)
    {
      auto out_channel = outputs.audio.channel(ch, t.frames);
      std::fill_n(out_channel.data(), t.frames, 0.0);
    }
  }
}

}