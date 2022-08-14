#pragma once

#include <optional>
#include <string>

namespace xpano::utils::resource {

std::optional<std::string> Find(const std::string& executable_path,
                                const std::string& rel_path);

}
