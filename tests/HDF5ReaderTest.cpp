// The HDF5 readers driven as the executor drives them: the update() of the file
// and path ports opens a dataset, then every tick reads from it. Ticks run on
// the execution thread, so neither a reader with nothing to read nor a read
// that HighFive refuses may stop it.

#include <DataReader/HDF5DataReader.hpp>
#include <DataReader/HDF5Texture.hpp>

#include <highfive/highfive.hpp>

#include <QTemporaryDir>

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

namespace
{
//! A file with a dataset of no rows, one of rows, and datasets of text, which
//! no read into numbers converts
struct Fixture
{
  QTemporaryDir dir;
  std::string path = dir.filePath("readers.h5").toStdString();

  Fixture()
  {
    REQUIRE(dir.isValid());
    HighFive::File f{path, HighFive::File::Truncate};
    f.createDataSet<float>("/empty", HighFive::DataSpace({0, 2}));
    f.createDataSet("/rows", std::vector<std::vector<float>>{{1, 2}, {3, 4}, {5, 6}});
    f.createDataSet<std::string>("/text", HighFive::DataSpace({3, 1}));
    f.createDataSet<std::string>("/frames", HighFive::DataSpace({2, 2, 2, 3}));
  }

  //! What a reader's ports do when the file and the path are set
  template <typename Reader>
  void open(Reader& r, std::string dataset) const
  {
    r.inputs.h5.file.filename = path;
    r.inputs.h5.file.bytes = "hdf5";
    r.inputs.h5.update(r);
    r.inputs.accessor.value = std::move(dataset);
    r.inputs.accessor.update(r);
  }
};

halp::tick_musical tick()
{
  halp::tick_musical t;
  t.frames = 64;
  return t;
}
}

TEST_CASE("an HDF5 reader with no dataset ticks", "[addons][hdf5]")
{
  auto reader = std::make_unique<DataReader::HDF5_Reader1D>();
  REQUIRE_NOTHROW((*reader)(tick()));
  CHECK(reader->outputs.out.value.get_type() == ossia::val_type::NONE);

  auto texture = std::make_unique<DataReader::HDF5_TextureReader>();
  REQUIRE_NOTHROW((*texture)());
}

TEST_CASE("an empty HDF5 dataset is not opened", "[addons][hdf5]")
{
  const Fixture fx;
  auto reader = std::make_unique<DataReader::HDF5_Reader1D>();
  fx.open(*reader, "/empty");
  CHECK_FALSE(reader->dataset);
  REQUIRE_NOTHROW((*reader)(tick()));
  CHECK(reader->outputs.out.value.get_type() == ossia::val_type::NONE);

  auto texture = std::make_unique<DataReader::HDF5_TextureReader>();
  fx.open(*texture, "/empty");
  CHECK_FALSE(texture->dataset);
  REQUIRE_NOTHROW((*texture)());
}

TEST_CASE("the HDF5 reader outputs the row at its percentage", "[addons][hdf5]")
{
  const Fixture fx;
  auto reader = std::make_unique<DataReader::HDF5_Reader1D>();
  fx.open(*reader, "/rows");
  REQUIRE(reader->dataset);
  reader->inputs.percent.value = 0.5f;
  (*reader)(tick());
  const auto& out = reader->outputs.out.value;
  REQUIRE(out.get_type() == ossia::val_type::VEC2F);
  CHECK(out.get<ossia::vec2f>() == ossia::vec2f{3.f, 4.f});
}

TEST_CASE("a read HighFive throws on leaves the reader running", "[addons][hdf5]")
{
  const Fixture fx;
  {
    // What the readers are given: HighFive throws on reading it as numbers
    HighFive::File f{fx.path, HighFive::File::ReadOnly};
    float v{};
    CHECK_THROWS(f.getDataSet("/text").select({0, 0}, {1, 1}).read(v));
    std::vector<std::vector<std::vector<std::vector<double>>>> frame;
    CHECK_THROWS(f.getDataSet("/frames").select({0, 0, 0, 0}, {1, 2, 2, 3}).read(frame));
  }

  auto reader = std::make_unique<DataReader::HDF5_Reader1D>();
  fx.open(*reader, "/text");
  REQUIRE(reader->dataset);
  REQUIRE_NOTHROW((*reader)(tick()));
  CHECK(reader->outputs.out.value.get_type() == ossia::val_type::NONE);

  // And it reads again once given a dataset it can read
  fx.open(*reader, "/rows");
  (*reader)(tick());
  CHECK(reader->outputs.out.value.get_type() == ossia::val_type::VEC2F);

  auto texture = std::make_unique<DataReader::HDF5_TextureReader>();
  fx.open(*texture, "/frames");
  REQUIRE(texture->dataset);
  REQUIRE_NOTHROW((*texture)());
}
