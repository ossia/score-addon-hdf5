#include "HDF5Texture.hpp"
namespace DataReader
{
// Note that as soon as we use textures,
// we run at frame rate (e.g. 60hz) instead of audio buffer rate
// (e.g. maybe 1000hz if you have a decent enough soundcard).
void HDF5_TextureReader::operator()()
{
  //   // Do some magic
  //   int k = 0;
  //   for(unsigned char& c : bytes)
  //   {
  //     c += k++ * inputs.bamboozle.value;
  //   }
  //
  //   // Call this when the texture changed
  //   outputs.image.texture.update(bytes.data(), 480, 270);
}
}
