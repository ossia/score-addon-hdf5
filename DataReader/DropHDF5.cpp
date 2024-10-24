#include "DropHDF5.hpp"

#include <Curve/Commands/UpdateCurve.hpp>
#include <Curve/Segment/Linear/LinearSegment.hpp>

#include <Automation/AutomationModel.hpp>

#include <DataReader/IncludeH5.hpp>
#include <highfive/boost.hpp>
#include <highfive/highfive.hpp>

namespace DataReader
{

static Curve::SegmentData
make_segment(int& current_id, double& cur_x, double& cur_y, double x, double y)
{
  Curve::SegmentData dat;
  dat.id = Id<Curve::SegmentModel>{current_id};
  dat.start.rx() = cur_x;
  dat.start.ry() = cur_y;
  dat.end.rx() = x;
  dat.end.ry() = y;
  cur_x = x;
  cur_y = y;
  dat.previous = Id<Curve::SegmentModel>{current_id - 1};
  dat.following = Id<Curve::SegmentModel>{current_id + 1};
  dat.type = Metadata<ConcreteKey_k, Curve::LinearSegment>::get();
  dat.specificSegmentData = QVariant::fromValue(Curve::LinearSegmentData{});
  return dat;
}

template <typename T, std::size_t N>
static auto range_to_automation(
    const boost::detail::multi_array::multi_array_view<T, N>& points, auto func)
    = delete;

template <typename T>
static auto range_to_automation(
    const boost::detail::multi_array::multi_array_view<T, 1>& points, auto func)
{
  std::vector<Curve::SegmentData> segt;

  if(points.empty())
    return segt;
  int current_id = 0;

  double cur_x = 0.;
  double cur_y = points[0];

  for(int i = 1, N = std::ssize(points); i < N; i++)
  {
    auto x = double(i) / N;
    auto y = points[i];

    segt.push_back(make_segment(current_id, cur_x, cur_y, x, y));
    current_id++;
  }
  segt.front().previous = std::nullopt;
  segt.back().following = std::nullopt;

  return segt;
}

template <typename T>
static auto range_to_automation(
    const boost::detail::multi_array::multi_array_view<T, 2>& points, auto func)
{
  std::vector<Curve::SegmentData> segt;
  if(points.empty())
    return segt;
  if(points[0].size() < 2)
    return segt;
  int current_id = 0;

  double cur_x = points[0][0];
  double cur_y = points[0][1];

  for(int i = 1, N = std::ssize(points); i < N; i++)
  {
    auto x = points[i][0];
    auto y = points[i][1];

    segt.push_back(make_segment(current_id, cur_x, cur_y, x, y));
    current_id++;
  }
  segt.front().previous = std::nullopt;
  segt.back().following = std::nullopt;

  return segt;
}

template <typename T, std::size_t N>
static constexpr auto shape(const boost::multi_array<T, N>& array) noexcept
{
  static_assert(N <= 128);
  boost::container::static_vector<int, N> res;
  for(auto it = array.shape(); it != array.shape() + N; it++)
    res.push_back(*it);
  return res;
}

template <typename T>
static void process_dataset(
    std::string_view path, boost::multi_array<T, 1>&& vec, auto func) noexcept
{
  // for(std::size_t i = 0; i < vec.size(); i++)
  //   qDebug() << vec[i];
}

template <typename T>
static void
process_dataset(std::string_view path, boost::multi_array<T, 2>&& A, auto func) noexcept
{
  const auto matrix_shape = shape(A);
  using range_t = typename std::remove_cvref_t<decltype(A)>::index_range;
  // Read by column
  for(int column = 0; column < matrix_shape[1]; column++)
  {
    int column_size = matrix_shape[0];

    auto col_view = A[boost::indices[range_t(0, column_size)][column]];
    if(auto res = range_to_automation(col_view, func); !res.empty())
    {
      func(path, std::move(res));
      return;
    }
    // for(auto element : col_view)
    // {
    //   qDebug() << element;
    // }
  }

  // Or read by row
  for(int row = 0; row < matrix_shape[0]; row++)
  {
    int row_size = matrix_shape[1];
    auto row_view = A[boost::indices[row][range_t(0, row_size)]];
    // for(auto element : row_view)
    // {
    //   qDebug() << element;
    // }
  }
}

template <typename T>
static void
process_dataset(std::string_view path, boost::multi_array<T, 3>&& A, auto func) noexcept
{
  const auto s = shape(A);
  // 3D texture ? RGB ?
  // for(std::size_t i = 0; i < A.size(); i++)
  //   qDebug() << A[i][0][0];
}

template <typename T>
static void
process_dataset(std::string_view path, boost::multi_array<T, 4>&& A, auto func) noexcept
{
  // for(std::size_t i = 0; i < A.size(); i++)
  //   qDebug() << A[i][0][0][0];
}

template <std::size_t NDims>
static void dispatch_dataset_type(HighFive::DataSet& dataset, auto func) noexcept
{
  auto val_type = dataset.getDataType().getClass(); // float, etc
  auto val_sz = dataset.getDataType().getSize();    // in bytes
  switch(val_type)
  {
    case HighFive::DataTypeClass::Integer:
      switch(val_sz)
      {
        case 1:
          process_dataset(
              dataset.getPath(), dataset.read<boost::multi_array<int8_t, NDims>>(),
              func);
          break;
        case 2:
          process_dataset(
              dataset.getPath(), dataset.read<boost::multi_array<int16_t, NDims>>(),
              func);
          break;
        case 4:
          process_dataset(
              dataset.getPath(), dataset.read<boost::multi_array<int32_t, NDims>>(),
              func);
          break;
        case 8:
          process_dataset(
              dataset.getPath(), dataset.read<boost::multi_array<int64_t, NDims>>(),
              func);
          break;
        default:
          break;
      }
      break;
    case HighFive::DataTypeClass::Float:
      switch(val_sz)
      {
        case 4:
          process_dataset(
              dataset.getPath(), dataset.read<boost::multi_array<float, NDims>>(), func);
          break;
        case 8:
          process_dataset(
              dataset.getPath(), dataset.read<boost::multi_array<double, NDims>>(),
              func);
          break;
        default:
          break;
      }
      break;
    default:
      return;
  }
}

static void dispatch_dataset_dim(HighFive::DataSet&& dataset, auto func) noexcept
{
  switch(dataset.getDimensions().size())
  {
    case 0:
      break;
    case 1:
      // dimension 0 is row
      dispatch_dataset_type<1>(dataset, func);
      break;
    case 2:
      // dimension 0 is row
      // dimension 1 is column
      dispatch_dataset_type<2>(dataset, func);
      break;
    case 3:
      dispatch_dataset_type<3>(dataset, func);
      break;
    case 4:
      dispatch_dataset_type<4>(dataset, func);
      break;
  }
}

void walk_hdf5(auto&& root, auto func) noexcept
{
  for(const auto& obj : root.listObjectNames())
  {
    switch(root.getObjectType(obj))
    {
      case H5Easy::ObjectType::Group: {
        walk_hdf5(root.getGroup(obj), func);
        break;
      }
      case H5Easy::ObjectType::Dataset: {
        dispatch_dataset_dim(root.getDataSet(obj), func);
        break;
      }
      default:
        break;
    }
  }
}

void DropHandler::dropPath(
    std::vector<ProcessDrop>& vec, const score::FilePath& filename,
    const score::DocumentContext& ctx) const noexcept
{
  H5Easy::File file(filename.absolute.toStdString(), H5Easy::File::ReadOnly);
  walk_hdf5(file, [&vec](std::string_view path, auto&& generated_data) {
    Process::ProcessDropHandler::ProcessDrop p;
    p.creation.prettyName = QString::fromUtf8(path);
    // p.creation.customData = filename.relative;
    // p.duration = 765000000;

    p.creation.key = Metadata<ConcreteKey_k, Automation::ProcessModel>::get();
    p.setup = [data = std::move(generated_data)](
                  Process::ProcessModel& proc, score::Dispatcher& disp) mutable {
      auto& p = safe_cast<Automation::ProcessModel&>(proc);
      disp.submit(new Curve::UpdateCurve(p.curve(), std::move(data)));
    };
    vec.push_back(p);
  });
}

QSet<QString> DropHandler::fileExtensions() const noexcept
{
  return {"h5", "hdf5"};
}
}
