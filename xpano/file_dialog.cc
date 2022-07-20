#include "file_dialog.h"

#include <array>
#include <string>
#include <vector>

#include <nfd.h>
#include <nfd.hpp>
#include <SDL.h>

namespace xpano::file_dialog {

std::vector<std::string> CallNfd() {
  NFD::UniquePathSet out_paths;

  std::array<nfdfilteritem_t, 1> filter_item;
  filter_item[0] = {"Images", "jpg,jpeg,png"};

  std::vector<std::string> results;

  // show the dialog
  nfdresult_t result =
      NFD::OpenDialogMultiple(out_paths, filter_item.data(), 1);
  if (result == NFD_OKAY) {
    SDL_Log("Success!");

    nfdpathsetsize_t num_paths;
    NFD::PathSet::Count(out_paths, num_paths);

    for (nfdpathsetsize_t i = 0; i < num_paths; ++i) {
      NFD::UniquePathSetPath path;
      NFD::PathSet::GetPath(out_paths, i, path);
      results.emplace_back(path.get());
    }
  } else if (result == NFD_CANCEL) {
    SDL_Log("User pressed cancel.");
  } else {
    SDL_Log("Error: %s", NFD::GetError());
  }

  return results;
}

}  // namespace xpano::file_dialog
