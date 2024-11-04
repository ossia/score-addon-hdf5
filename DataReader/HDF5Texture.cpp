#include "HDF5Texture.hpp"

#include <highfive/boost.hpp>
#include <highfive/highfive.hpp>
namespace DataReader
{
void HDF5_TextureReader::operator()()
{
  if(!dataset->isValid())
    return;

  if(dataset_dims.size() != 4)
    return;
  if(dataset_dims[3] != 3)
    return;

  int n_frames = dataset_dims[0];
  int height = dataset_dims[1];
  int width = dataset_dims[2];

  if(width != outputs.image.texture.width || height != outputs.image.texture.height)
    bytes = outputs.image.texture.allocate(width, height);

  int frame = std::clamp(this->inputs.percent * (n_frames - 1.), 0., n_frames - 1.);
  thread_local boost::multi_array<double, 4> result;
  dataset->select({(unsigned)frame, 0, 0, 0}, {1, (unsigned)height, (unsigned)width, 3})
      .read(result);

  int sp = 0;
  const auto& img = result[0];
  for(int y = 0; y < height; y++)
  {
    const auto& row = img[y];
    for(int x = 0; x < width; x++)
    {
      const auto& col = row[x];
      for(int c = 0; c < 3; c++)
        bytes[sp++] = 255 * col[c];
      bytes[sp++] = 255;
    }
  }
  outputs.image.texture.update(bytes.data(), width, height);
}
}
