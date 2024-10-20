#pragma once

#define HIGHFIVE_LOG_LEVEL HIGHFIVE_LOG_LEVEL_ERROR

#include <DataReader/HDF5Ports.hpp>
#include <halp/controls.hpp>
#include <halp/meta.hpp>
#include <halp/sample_accurate_controls.hpp>
#include <halp/texture.hpp>

namespace DataReader
{
struct HDF5_TextureReader : public HDF5ObjectBase
{
  halp_meta(name, "HDF5 Texture Reader")
  halp_meta(c_name, "HDF5_TextureReader")
  halp_meta(category, "Data")
  halp_meta(author, "Jean-Michaël Celerier, Société des Arts Technologiques")
  halp_meta(description, "Read a texture from an HDF5 file")
  halp_meta(uuid, "fe2c547b-f53a-41d2-81a4-91a602a45488")

  struct
  {
    HDF5FilePort h5;
    HDF5AccessorPort accessor;
    halp::hslider_f32<"Percentage"> percent;
  } inputs;

  struct
  {
    struct
    {
      halp_meta(name, "Out");
      halp::rgba_texture texture;
    } image;
  } outputs;

  // Some place in RAM to store our pixels
  halp::rgba_texture::uninitialized_bytes bytes;

  HDF5_TextureReader()
  {
    // Allocate some initial data
    bytes = halp::rgba_texture::allocate(480, 270);
    for(unsigned char& c : bytes)
    {
      c = std::rand() % 10;
    }
  }

  // Note that as soon as we use textures,
  // we run at frame rate (e.g. 60hz) instead of audio buffer rate
  // (e.g. maybe 1000hz if you have a decent enough soundcard).
  void operator()();
};
}
