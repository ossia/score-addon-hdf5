#pragma once
#include <Process/Drop/ProcessDropHandler.hpp>

namespace DataReader
{
class CSVDropHandler final : public Process::ProcessDropHandler
{
  SCORE_CONCRETE("496e9235-eff6-4020-8b9b-2822481c477e")

  QSet<QString> fileExtensions() const noexcept override;

  void dropPath(
      std::vector<ProcessDrop>& vec, const score::FilePath& filename,
      const score::DocumentContext& ctx) const noexcept override;
};
}
