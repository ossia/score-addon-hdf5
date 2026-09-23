#include "DropCSV.hpp"

#include <Curve/Commands/UpdateCurve.hpp>
#include <Curve/Segment/PointArray/PointArraySegment.hpp>

#include <Scenario/Commands/Metadata/ChangeElementLabel.hpp>
#include <Scenario/Commands/Metadata/ChangeElementName.hpp>

#include <Automation/AutomationModel.hpp>
#include <Automation/Commands/SetAutomationMax.hpp>

#include <ossia/detail/parse_strict.hpp>
#include <ossia/math/safe_math.hpp>

#include <limits>

#include <AvndProcesses/DeviceRecorder.hpp>

namespace DataReader
{

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

      // Non-finite values stay out of the range, and out of the curve.
      if(!ossia::safe_isfinite(res))
        res = std::numeric_limits<float>::quiet_NaN();
      else
      {
        mins[track] = std::min(mins[track], res);
        maxs[track] = std::max(maxs[track], res);
      }
      tracks[track].push_back(res);
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

  // Large columns become a single sampled segment.
  for(int i = 0; i < tracks.size(); i++)
  {
    for_each(titles[i], Curve::curveFromSamples(tracks[i]), mins[i], maxs[i]);
  }
}

QSet<QString> CSVDropHandler::fileExtensions() const noexcept
{
  return {"csv"};
}
}
