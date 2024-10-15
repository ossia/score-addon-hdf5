#pragma once
#include <score/command/Command.hpp>
#include <score/model/path/Path.hpp>

namespace DataReader
{
inline const CommandGroupKey& CommandFactoryName()
{
  static const CommandGroupKey key{"DataReader"};
  return key;
}

}
