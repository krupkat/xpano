// SPDX-FileCopyrightText: 2026 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <SDL3/SDL_dialog.h>

#include "xpano/gui/file_dialog/sdl.h"

namespace {
SDL_DialogFileCallback callback;
void* userdata;
const SDL_DialogFileFilter* filter;
const char* location;
SDL_Window* parent;
bool many;

void Complete(const char* const* paths) {
  // Exercise the same cross-thread handoff used by some SDL backends.
  std::thread worker([paths] { callback(userdata, paths, -1); });
  worker.join();
}
}  // namespace

// These functions are renamed via compile definitions for this test target.
// No native UI is opened; completion is controlled by each test.
extern "C" void SDLCALL SDL_ShowOpenFileDialog(
    SDL_DialogFileCallback cb, void* data, SDL_Window* window,
    const SDL_DialogFileFilter* filters, int, const char*, bool allow_many) {
  callback = cb;
  userdata = data;
  filter = filters;
  parent = window;
  many = allow_many;
}

extern "C" void SDLCALL SDL_ShowOpenFolderDialog(SDL_DialogFileCallback cb,
                                                 void* data, SDL_Window* window,
                                                 const char*, bool allow_many) {
  callback = cb;
  userdata = data;
  parent = window;
  many = allow_many;
}

extern "C" void SDLCALL SDL_ShowSaveFileDialog(
    SDL_DialogFileCallback cb, void* data, SDL_Window* window,
    const SDL_DialogFileFilter* filters, int, const char* default_location) {
  callback = cb;
  userdata = data;
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
  REQUIRE(std::string(filter->pattern).find(';') != std::string::npos);
  REQUIRE(std::string(filter->pattern).find(',') == std::string::npos);
  const char* files[] = {"photo.JPG", "ignore.txt", "caf\xc3\xa9.png", nullptr};
  Complete(files);
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
    const char* files[] = {nullptr};
    Complete(files);
    REQUIRE(future.get().error().type ==
            xpano::gui::file_dialog::ErrorType::kUserCancelled);
  }
  SECTION("Error") {
    callback(userdata, nullptr, -1);
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
    const char* files[] = {"panorama.PNG", nullptr};
    Complete(files);
    REQUIRE(future.get());
  }
  SECTION("Unsupported extension") {
    const char* files[] = {"panorama.txt", nullptr};
    Complete(files);
    REQUIRE(future.get().error().type ==
            xpano::gui::file_dialog::ErrorType::kUnsupportedExtension);
  }
}

TEST_CASE("Folder dialog rejects non-directories") {
  xpano::gui::file_dialog::Sdl backend{nullptr};
  xpano::gui::file_dialog::Base& api = backend;
  auto future = api.Open({.type = xpano::gui::ActionType::kOpenDirectory});
  REQUIRE_FALSE(many);
  const char* files[] = {"nonexistent-file-dialog-test-directory", nullptr};
  Complete(files);
  REQUIRE(future.get().error().type ==
          xpano::gui::file_dialog::ErrorType::kTargetNotDirectory);
}

TEST_CASE(
    "Dialog callback remains safe after the future and backend are destroyed") {
  {
    xpano::gui::file_dialog::Sdl backend{nullptr};
    auto future = backend.Save("panorama.jpg");
  }
  REQUIRE(std::string(location) == "panorama.jpg");
  const char* files[] = {"panorama.jpg", nullptr};
  Complete(files);
}

TEST_CASE("Dialog future remains usable after the backend is destroyed") {
  std::future<
      std::expected<std::filesystem::path, xpano::gui::file_dialog::Error>>
      future;
  {
    xpano::gui::file_dialog::Sdl backend{nullptr};
    future = backend.Save("panorama.jpg");
  }
  const char* files[] = {"panorama.jpg", nullptr};
  Complete(files);
  auto result = future.get();
  REQUIRE(result);
  CHECK(*result == "panorama.jpg");
}

TEST_CASE("Unknown file dialog actions fail without launching a dialog") {
  xpano::gui::file_dialog::Sdl backend{nullptr};
  xpano::gui::file_dialog::Base& api = backend;
  callback = nullptr;
  auto future = api.Open({});
  REQUIRE(future.get().error().type ==
          xpano::gui::file_dialog::ErrorType::kUnknownAction);
  CHECK(callback == nullptr);
}

TEST_CASE("SDL dialogs use the backend window as their parent") {
  // The mocked dialog functions only compare this opaque token; no SDL function
  // dereferences it, so no video subsystem or real window is needed.
  int window_token = 0;
  auto* window = reinterpret_cast<SDL_Window*>(&window_token);
  xpano::gui::file_dialog::Sdl backend{window};
  xpano::gui::file_dialog::Base& api = backend;
  const char* cancelled[] = {nullptr};

  auto open = api.Open({.type = xpano::gui::ActionType::kOpenFiles});
  CHECK(parent == window);
  Complete(cancelled);
  CHECK_FALSE(open.get());

  auto folder = api.Open({.type = xpano::gui::ActionType::kOpenDirectory});
  CHECK(parent == window);
  Complete(cancelled);
  CHECK_FALSE(folder.get());

  auto save = api.Save("panorama.jpg");
  CHECK(parent == window);
  Complete(cancelled);
  CHECK_FALSE(save.get());
}
