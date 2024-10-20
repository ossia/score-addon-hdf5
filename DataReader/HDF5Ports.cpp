#include "HDF5Ports.hpp"

#include <State/Address.hpp>

#include <ossia/network/common/destination_qualifiers.hpp>
namespace DataReader
{

void HDF5ObjectBase::update_dataset(
    H5Easy::File& file, const std::string& path, const ossia::destination_index& idx)
{
  try
  {
    if(auto val = file.getDataSet(path); val.isValid())
    {
      dataset_dims = val.getDimensions();
      if(dataset_dims.size() == 0)
        return;
      if(dataset_dims[0] == 0)
        return;

      // FIXME: accessor incorrect -> exception thrown
      // if(m_accessors.size() != dims.size())
      // {
      //
      // }
      dataset = std::move(val);
    }
  }
  catch(...)
  {
    dataset.reset();
    return;
  }
}

bool HDF5FilePort::update_file()
{
  datafile.reset();
  if(file.bytes.size() == 0)
    return false;

  try
  {
    datafile.emplace(std::string(file.filename), H5Easy::File::ReadOnly);
  }
  catch(...)
  {
    datafile.reset();
    return false;
  }

  return true;
}

bool HDF5AccessorPort::update_path()
{
  path.clear();
  if(this->value.empty())
    return false;

  // FIXME make independent of score
  auto str = QString::fromStdString(this->value);
  if(str.startsWith('/'))
    str.prepend(":");
  else
    str.prepend(":/");

  auto res = State::parseAddressAccessor(str);
  if(!res)
    return false;

  path = "/" + res->address.path.join("/").toStdString();
  accessors = res->qualifiers.get().accessors;

  return true;
}

}
