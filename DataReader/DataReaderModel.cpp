#include "DataReaderModel.hpp"

#include <State/Address.hpp>

#include <ossia/network/common/destination_qualifiers.hpp>

#include <highfive/H5Easy.hpp>
#include <highfive/boost.hpp>
#include <highfive/highfive.hpp>
namespace DataReader
{

void HDF5_Reader1D::update_file()
{
  m_dataset.reset();
  m_file.reset();
  if(inputs.h5.file.bytes.size() == 0)
    return;

  try
  {
    m_file.emplace(std::string(inputs.h5.file.filename), H5Easy::File::ReadOnly);
  }
  catch(...)
  {
    m_file.reset();
    return;
  }

  update_dataset();
}

void HDF5_Reader1D::update_path()
{
  m_dataset.reset();
  m_path.clear();
  if(this->inputs.accessor.value.empty())
    return;

  // FIXME make independent of score
  auto str = QString::fromStdString(this->inputs.accessor.value);
  if(str.startsWith('/'))
    str.prepend(":");
  else
    str.prepend(":/");

  auto res = State::parseAddressAccessor(str);
  if(!res)
    return;

  m_path = "/" + res->address.path.join("/").toStdString();
  m_accessors = res->qualifiers.get().accessors;

  update_dataset();
}

void HDF5_Reader1D::update_dataset()
{
  m_dataset.reset();
  if(!m_file || m_path.empty())
    return;

  try
  {
    if(auto val = m_file->getDataSet(m_path); val.isValid())
    {
      m_dataset_dims = val.getDimensions();
      if(m_dataset_dims.size() == 0)
        return;
      if(m_dataset_dims[0] == 0)
        return;

      // FIXME: accessor incorrect -> exception thrown
      // if(m_accessors.size() != dims.size())
      // {
      //
      // }
      m_dataset = std::move(val);
    }
  }
  catch(...)
  {
    m_dataset.reset();
    return;
  }
}

void HDF5_Reader1D::operator()(halp::tick_musical t)
{
  if(!m_dataset->isValid())
    return;

  // Time is the first index.
  // We work proportionally:
  auto& d = *m_dataset;
  std::size_t n_rows = m_dataset_dims[0];
  auto idx = std::size_t(this->inputs.percent * n_rows) % n_rows;

  thread_local boost::multi_array<double, 1> arr1;
  thread_local boost::multi_array<double, 2> arr2;
  thread_local boost::multi_array<double, 3> arr3;
  thread_local boost::multi_array<double, 4> arr4;
  thread_local boost::multi_array<double, 5> arr5;

  const auto ndims = m_dataset_dims.size();
  thread_local std::vector<std::size_t> offsets;
  offsets.clear();
  thread_local std::vector<std::size_t> sizes;
  sizes.clear();

  switch(ndims)
  {
    case 2: {
      offsets = {idx, 0}; // read one row at idx, all columns considered
      switch(m_dataset_dims[1])
      {
        case 1: {
          sizes = {1, 1};
          float a;
          d.select(offsets, sizes).read(a);
          outputs.out.value = a;
          break;
        }
        case 2: {
          sizes = {1, 2};
          std::array<std::array<float, 2>, 1> a;
          d.select(offsets, sizes).read(a);
          outputs.out.value = a[0];
          break;
        }
        case 3: {
          sizes = {1, 3};
          std::array<std::array<float, 3>, 1> a;
          d.select(offsets, sizes).read(a);
          outputs.out.value = a[0];
          break;
        }
        case 4: {
          sizes = {1, 4};
          std::array<std::array<float, 4>, 1> a;
          d.select(offsets, sizes).read(a);
          outputs.out.value = a[0];
          break;
        }
        default: {
          sizes = {1, m_dataset_dims[1]};
          std::array<std::vector<float>, 1> a;
          d.select(offsets, sizes).read(a);
          // FIXME suboptimal
          outputs.out.value = std::vector<ossia::value>(a[0].begin(), a[0].end());

          break;
        }
      }
      break;
    }
    case 3:
      // d.select({idx, 0}, {1, 0}).read(arr1);
      break;
    case 4:
      // d.select({idx, 0}, {1, 0}).read(arr3);
      break;
    case 5:
      //d.select({idx, 0}, {1, 0}).read(arr4);
      break;
  }

  {
  }
}
}
