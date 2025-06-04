#include "DropCSV.hpp"

#include <Curve/Commands/UpdateCurve.hpp>
#include <Curve/Segment/Linear/LinearSegment.hpp>

#include <Scenario/Commands/Metadata/ChangeElementLabel.hpp>
#include <Scenario/Commands/Metadata/ChangeElementName.hpp>

#include <Automation/AutomationModel.hpp>
#include <Automation/Commands/SetAutomationMax.hpp>

#include <ossia/detail/parse_strict.hpp>

#include <AvndProcesses/DeviceRecorder.hpp>

namespace DataReader
{

static Curve::SegmentData
make_segment_csv(int& current_id, double& cur_x, double& cur_y, double x, double y)
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

static auto range_to_automation(const std::span<float>& points)
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

    segt.push_back(make_segment_csv(current_id, cur_x, cur_y, x, y));
    current_id++;
  }
  segt.front().previous = std::nullopt;
  segt.back().following = std::nullopt;

  return segt;
}

void CSVDropHandler::dropPath(
    std::vector<ProcessDrop>& vec, const score::FilePath& filename,
    const score::DocumentContext& ctx) const noexcept
{
  QFile f{filename.absolute};
  auto res = f.open(QIODevice::ReadOnly);
  SCORE_ASSERT(res);
  auto data = (const char*)f.map(0, f.size());

  csv2::Reader<> r;
  r.parse_view({data, data + f.size()});

  int columns = r.cols();
  auto header = r.header();
  SCORE_ASSERT(columns >= 0);
  std::vector<std::vector<float>> tracks(columns);
  for(auto& t : tracks)
    t.reserve(r.rows());

  std::vector<float> mins, maxs;
  mins.resize(tracks.size());
  std::fill(mins.begin(), mins.end(), 1e99);
  maxs.resize(tracks.size());
  std::fill(maxs.begin(), maxs.end(), -1e99);
  std::vector<std::string> titles;
  for(const auto& addr : header)
  {
    std::string a;
    addr.read_raw_value(a);
    titles.push_back(a);
  }

  int k = 0;

  std::string v;
  int rr = 0;
  for(const csv2::Reader<>::Row& row : r)
  {
    int track = 0;
    for(const auto& cell : row)
    {
      if(track >= tracks.size())
        break;
      v.clear();
      cell.read_value(v);

      float res = 0.f;
      if(auto r = ossia::parse_strict<float>(v))
        res = *r;

      tracks[track].push_back(res);
      mins[track] = std::min(mins[track], res);
      maxs[track] = std::max(maxs[track], res);
      track++;
    }
    rr++;
  }

  for(int i = 0; i < tracks.size(); i++)
  {
    auto& t = tracks[i];
    float min = mins[i];
    float max = maxs[i];

    if(max - min <= 0.00000000000001f)
      continue;
    for(float& f : tracks[i])
      f = (f - min) / (max - min);
  }

  auto for_each
      = [&vec](std::string_view path, auto&& generated_data, float min, float max) {
    Process::ProcessDropHandler::ProcessDrop p;
    p.creation.prettyName = QString::fromUtf8(path);

    p.creation.key = Metadata<ConcreteKey_k, Automation::ProcessModel>::get();
    p.setup = [min, max, data = std::move(generated_data)](
                  Process::ProcessModel& proc, score::Dispatcher& disp) mutable {
      auto& p = safe_cast<Automation::ProcessModel&>(proc);

      disp.submit(new Automation::SetMin(p, min));
      disp.submit(new Automation::SetMax(p, max));
      disp.submit(new Curve::UpdateCurve(p.curve(), std::move(data)));
    };
    vec.push_back(p);
  };

  for(int i = 0; i < tracks.size(); i++)
  {
    for_each(titles[i], range_to_automation(tracks[i]), mins[i], maxs[i]);
  }
}

QSet<QString> CSVDropHandler::fileExtensions() const noexcept
{
  return {"csv"};
}
}
