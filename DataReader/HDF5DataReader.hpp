#pragma once

#include <ossia/network/value/value.hpp>

#include <DataReader/HDF5Ports.hpp>
#include <halp/audio.hpp>
#include <halp/controls.hpp>
#include <halp/file_port.hpp>
#include <halp/meta.hpp>

namespace DataReader
{
struct HDF5_Reader1D : public HDF5ObjectBase
{
  halp_meta(name, "HDF5 Reader")
  halp_meta(category, "Control/Data files")
  halp_meta(c_name, "HDF5_Reader1D")
  halp_meta(author, "Jean-Michaël Celerier, Société des Arts Technologiques")
  halp_meta(description, "Read time series from an HDF5 file")
  halp_meta(manual_url, "https://ossia.io/score-docs/processes/hdf5.html#1d-reader")
  halp_meta(uuid, "9b799392-6ec5-4255-b2b9-02c087722f68")
  struct
  {
    HDF5FilePort h5;
    HDF5AccessorPort accessor;

    halp::hslider_f32<"Percentage"> percent;
  } inputs;

  struct
  {
    halp::val_port<"Out", ossia::value> out;
  } outputs;

  using tick = halp::tick_musical;
  void operator()(tick frames);
};
}
