#include "score_addon_datareader.hpp"

#include <score/plugins/FactorySetup.hpp>

#include <DataReader/CommandFactory.hpp>
#include <DataReader/DropCSV.hpp>
#include <DataReader/DropHDF5.hpp>

#include <score_addon_datareader_commands_files.hpp>

#include <vector>

score_addon_datareader::score_addon_datareader() { }

score_addon_datareader::~score_addon_datareader() { }

std::vector<score::InterfaceBase*> score_addon_datareader::factories(
    const score::ApplicationContext& ctx, const score::InterfaceKey& key) const
{
  return instantiate_factories<
      score::ApplicationContext, FW<Process::ProcessDropHandler, DataReader::DropHandler,
                                    DataReader::CSVDropHandler>>(ctx, key);
}

std::pair<const CommandGroupKey, CommandGeneratorMap>
score_addon_datareader::make_commands()
{
  using namespace DataReader;
  std::pair<const CommandGroupKey, CommandGeneratorMap> cmds{
      CommandFactoryName(), CommandGeneratorMap{}};

  ossia::for_each_type<
#include <score_addon_datareader_commands.hpp>
      >(score::commands::FactoryInserter{cmds.second});

  return cmds;
}

#include <score/plugins/PluginInstances.hpp>
SCORE_EXPORT_PLUGIN(score_addon_datareader)
