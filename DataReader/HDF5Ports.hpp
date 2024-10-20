#pragma once

#define HIGHFIVE_LOG_LEVEL HIGHFIVE_LOG_LEVEL_ERROR
#include <ossia/detail/destination_index.hpp>

#include <halp/controls.hpp>
#include <halp/file_port.hpp>
#include <halp/meta.hpp>
#include <highfive/H5Easy.hpp>
namespace DataReader
{
struct HDF5ObjectBase
{
  // Note: should be std::string_view but HDF5 API wants a std::string...
  void update_dataset(
      H5Easy::File& file, const std::string& path, const ossia::destination_index& idx);

  void update_dataset(auto& self)
  {
    if(auto& file = self.inputs.h5.datafile)
      if(auto& path = self.inputs.accessor.path; !path.empty())
        self.update_dataset(*file, path, self.inputs.accessor.accessors);
  }

  std::optional<H5Easy::DataSet> dataset;
  std::vector<std::size_t> dataset_dims;
};

struct HDF5FilePort : halp::file_port<"HDF5 file", halp::mmap_file_view>
{
  halp_meta(extensions, "*.h5");
  void update(auto& self)
  {
    self.dataset.reset();
    if(update_file())
      self.update_dataset(self);
  }
  bool update_file();

  std::optional<H5Easy::File> datafile;
};

struct HDF5AccessorPort : halp::lineedit<"Path", "/">
{
  void update(auto& self)
  {
    self.dataset.reset();
    if(update_path())
      self.update_dataset(self);
  }
  bool update_path();

  std::string path{"/"};
  ossia::destination_index accessors;
};
}
