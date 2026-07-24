#pragma once

#include <string>
#include <vector>

namespace dungeon::LevelData {

using Level = std::vector<std::string>;

[[nodiscard]] const std::vector<Level>& allLevels();

} // namespace dungeon::LevelData
