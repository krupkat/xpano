// SPDX-FileCopyrightText: 2026 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <chrono>
#include <filesystem>
#include <future>
#include <string>
#include <thread>
#include <variant>

#include <catch2/catch_test_macros.hpp>
#include <SDL3/SDL_dialog.h>

#include "xpano/gui/action.h"
#include "xpano/gui/file_dialog/base.h"
#include "xpano/gui/file_dialog/error.h"
#include "xpano/gui/file_dialog/sdl.h"

namespace {
SDL_DialogFileCallback dialog_callback;
void* dialog_userdata;
const SDL_DialogFileFilter* filter;
const char* location;
SDL_Window* parent;
bool many;

void Complete(const char* const* paths) {
  // Exercise the same cross-thread handoff used by some SDL backends.
  std::thread worker([paths] { dialog_callback(dialog_userdata, paths, -1); });
  worker.join();
}
}  // namespace

// These functions are renamed via compile definitions for this test target.
// No native UI is opened; completion is controlled by each test.
extern "C" void SDLCALL SDL_ShowOpenFileDialog(
    SDL_DialogFileCallback callback, void* userdata, SDL_Window* window,
    const SDL_DialogFileFilter* filters, int /*nfilters*/,
    const char* /*default_location*/, bool allow_many) {
  dialog_callback = callback;
  dialog_userdata = userdata;
  filter = filters;
  parent = window;
  many = allow_many;
}

extern "C" void SDLCALL SDL_ShowOpenFolderDialog(
    SDL_DialogFileCallback callback, void* userdata, SDL_Window* window,
    const char* /*default_location*/, bool allow_many) {
  dialog_callback = callback;
  dialog_userdata = userdata;
  parent = window;
  many = allow_many;
}

extern "C" void SDLCALL
SDL_ShowSaveFileDialog(SDL_DialogFileCallback callback, void* userdata,
                       SDL_Window* window, const SDL_DialogFileFilter* filters,
                       int /*nfilters*/, const char* default_location) {
  dialog_callback = callback;
  dialog_userdata = userdata;
  filter = filters;
  parent = window;
  location = default_location;
}

TEST_CASE("File dialogs return asynchronously and filter selected paths") {
  xpano::gui::file_dialog::Sdl backend{nullptr};
  xpano::gui::file_dialog::Base& api = backend;
  auto future = api.Open({.type = xpano::gui::ActionType::kOpenFiles});
  REQUIRE(future.wait_for(std::chrono::seconds(0)) ==
          std::future_status::timeout);
  REQUIRE(many);
  REQUIRE(std::string(filter->pattern).contains(';'));
  REQUIRE_FALSE(std::string(filter->pattern).contains(','));
  const std::array<const char*, 4> files{"photo.JPG", "ignore.txt",
                                         "caf\xc3\xa9.png", nullptr};
  Complete(files.data());
  auto result = future.get();
  REQUIRE(result);
  REQUIRE(result->size() == 2);
  REQUIRE(result->at(1).u8string() == u8"caf\u00e9.png");
}

TEST_CASE("File dialog cancellation and errors are distinct") {
  xpano::gui::file_dialog::Sdl backend{nullptr};
  xpano::gui::file_dialog::Base& api = backend;
  auto future = api.Open({.type = xpano::gui::ActionType::kOpenFiles});
  SECTION("Cancellation") {
    const std::array<const char*, 1> files{nullptr};
    Complete(files.data());
    REQUIRE(future.get().error().type ==
            xpano::gui::file_dialog::ErrorType::kUserCancelled);
  }
  SECTION("Error") {
    dialog_callback(dialog_userdata, nullptr, -1);
    REQUIRE(future.get().error().type ==
            xpano::gui::file_dialog::ErrorType::kUnknownError);
  }
}

TEST_CASE("Save dialog keeps its default name and validates extensions") {
  xpano::gui::file_dialog::Sdl backend{nullptr};
  xpano::gui::file_dialog::Base& api = backend;
  auto future = api.Save(std::string("panorama.jpg"));
  REQUIRE(std::string(location) == "panorama.jpg");
  SECTION("Supported extension") {
    const std::array<const char*, 2> files{"panorama.PNG", nullptr};
    Complete(files.data());
    REQUIRE(future.get());
  }
  SECTION("Unsupported extension") {
    const std::array<const char*, 2> files{"panorama.txt", nullptr};
    Complete(files.data());
    REQUIRE(future.get().error().type ==
            xpano::gui::file_dialog::ErrorType::kUnsupportedExtension);
  }
}

TEST_CASE("Folder dialog rejects non-directories") {
  xpano::gui::file_dialog::Sdl backend{nullptr};
  xpano::gui::file_dialog::Base& api = backend;
  auto future = api.Open({.type = xpano::gui::ActionType::kOpenDirectory});
  REQUIRE_FALSE(many);
  const std::array<const char*, 2> files{
      "nonexistent-file-dialog-test-directory", nullptr};
  Complete(files.data());
  REQUIRE(future.get().error().type ==
          xpano::gui::file_dialog::ErrorType::kTargetNotDirectory);
}

TEST_CASE(
    "Dialog callback remains safe after the future and backend are destroyed") {
  {
    xpano::gui::file_dialog::Sdl backend{nullptr};
    auto future = backend.Save("panorama.jpg");
    REQUIRE(future.valid());
  }
  REQUIRE(std::string(location) == "panorama.jpg");
  const std::array<const char*, 2> files{"panorama.jpg", nullptr};
  Complete(files.data());
}

TEST_CASE("Dialog future remains usable after the backend is destroyed") {
  std::future<xpano::gui::file_dialog::SaveResult> future;
  {
    xpano::gui::file_dialog::Sdl backend{nullptr};
    future = backend.Save("panorama.jpg");
  }
  const std::array<const char*, 2> files{"panorama.jpg", nullptr};
  Complete(files.data());
  auto result = future.get();
  REQUIRE(result);
  CHECK(*result == "panorama.jpg");
}

TEST_CASE("Unknown file dialog actions fail without launching a dialog") {
  xpano::gui::file_dialog::Sdl backend{nullptr};
  xpano::gui::file_dialog::Base& api = backend;
  dialog_callback = nullptr;
  auto future = api.Open({});
  REQUIRE(future.get().error().type ==
          xpano::gui::file_dialog::ErrorType::kUnknownAction);
  CHECK(dialog_callback == nullptr);
}

TEST_CASE("SDL dialogs use the backend window as their parent") {
  // The mocked dialog functions only compare this opaque token; no SDL function
  // dereferences it, so no video subsystem or real window is needed.
  int window_token = 0;
  auto* window = reinterpret_cast<SDL_Window*>(&window_token);
  xpano::gui::file_dialog::Sdl backend{window};
  xpano::gui::file_dialog::Base& api = backend;
  const std::array<const char*, 1> cancelled{nullptr};

  auto open = api.Open({.type = xpano::gui::ActionType::kOpenFiles});
  CHECK(parent == window);
  Complete(cancelled.data());
  CHECK_FALSE(open.get());

  auto folder = api.Open({.type = xpano::gui::ActionType::kOpenDirectory});
  CHECK(parent == window);
  Complete(cancelled.data());
  CHECK_FALSE(folder.get());

  auto save = api.Save("panorama.jpg");
  CHECK(parent == window);
  Complete(cancelled.data());
  CHECK_FALSE(save.get());
}

// Catch2 assertion macro expansion inflates the reported complexity.
// NOLINTBEGIN(readability-function-cognitive-complexity)
TEST_CASE("File dialog future variant tracks open and save lifetimes") {
  using xpano::gui::file_dialog::Future;
  using xpano::gui::file_dialog::OpenResult;
  using xpano::gui::file_dialog::SaveResult;
  xpano::gui::file_dialog::Sdl backend{nullptr};
  Future future;
  auto pending = [&future] {
    return std::visit([](const auto& value) { return value.valid(); }, future);
  };

  CHECK_FALSE(pending());
  future = backend.Open({.type = xpano::gui::ActionType::kOpenFiles});
  CHECK(std::holds_alternative<std::future<OpenResult>>(future));
  CHECK(pending());
  CHECK(std::get<std::future<OpenResult>>(future).wait_for(
            std::chrono::seconds{0}) == std::future_status::timeout);
  const std::array<const char*, 2> files{"photo.jpg", nullptr};
  Complete(files.data());
  auto opened = std::get<std::future<OpenResult>>(future).get();
  REQUIRE(opened);
  CHECK(opened->size() == 1);
  CHECK_FALSE(pending());

  future = backend.Save("panorama.jpg");
  CHECK(std::holds_alternative<std::future<SaveResult>>(future));
  CHECK(pending());
  const std::array<const char*, 1> cancelled{nullptr};
  Complete(cancelled.data());
  auto saved = std::get<std::future<SaveResult>>(future).get();
  REQUIRE_FALSE(saved);
  CHECK(saved.error().type ==
        xpano::gui::file_dialog::ErrorType::kUserCancelled);
  CHECK_FALSE(pending());
}
// NOLINTEND(readability-function-cognitive-complexity)
