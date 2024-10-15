#pragma once
#include <Process/Drop/ProcessDropHandler.hpp>

namespace DataReader
{
class DropHandler final : public Process::ProcessDropHandler
{
  SCORE_CONCRETE("5e23e437-4081-4596-893d-1759a0d49c28")

  QSet<QString> fileExtensions() const noexcept override;

  void dropPath(
      std::vector<ProcessDrop>& vec, const score::FilePath& filename,
      const score::DocumentContext& ctx) const noexcept override;
};
}
