#pragma once

#define HIGHFIVE_LOG_LEVEL HIGHFIVE_LOG_LEVEL_ERROR
#include <ossia/detail/destination_index.hpp>

#include <halp/audio.hpp>
#include <halp/controls.hpp>
#include <halp/file_port.hpp>
#include <halp/meta.hpp>
#include <highfive/H5Easy.hpp>

namespace DataReader
{
class HDF5_Reader1D
{
public:
  halp_meta(name, "HDF5 Reader")
  halp_meta(category, "Data")
  halp_meta(c_name, "HDF5_Reader1D")
  halp_meta(uuid, "9b799392-6ec5-4255-b2b9-02c087722f68")
  struct
  {
    struct : halp::file_port<"HDF5 file", halp::mmap_file_view>
    {
      halp_meta(extensions, "*.h5");
      void update(HDF5_Reader1D& self) { self.update_file(); }
    } h5;

    struct : halp::lineedit<"Path", "/">
    {
      void update(HDF5_Reader1D& self) { self.update_path(); }
    } accessor;

    halp::hslider_f32<"Percentage"> percent;
  } inputs;

  struct
  {
    halp::val_port<"Out", ossia::value> out;
  } outputs;

  using tick = halp::tick_musical;
  void operator()(tick frames);
  void update_path();
  void update_file();
  void update_dataset();

  std::string m_path{"/"};
  ossia::destination_index m_accessors;

  std::optional<H5Easy::File> m_file;
  std::optional<H5Easy::DataSet> m_dataset;
  std::vector<std::size_t> m_dataset_dims;
};

}
