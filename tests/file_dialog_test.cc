// SPDX-FileCopyrightText: 2026 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/gui/file_dialog.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <SDL3/SDL_dialog.h>

namespace {
SDL_DialogFileCallback callback;
void* userdata;
const SDL_DialogFileFilter* filter;
const char* location;
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
    SDL_DialogFileCallback cb, void* data, SDL_Window*,
    const SDL_DialogFileFilter* filters, int, const char*, bool allow_many) {
  callback = cb;
  userdata = data;
  filter = filters;
  many = allow_many;
}

extern "C" void SDLCALL SDL_ShowOpenFolderDialog(SDL_DialogFileCallback cb,
                                                 void* data, SDL_Window*,
                                                 const char*, bool allow_many) {
  callback = cb;
  userdata = data;
  many = allow_many;
}

extern "C" void SDLCALL SDL_ShowSaveFileDialog(
    SDL_DialogFileCallback cb, void* data, SDL_Window*,
    const SDL_DialogFileFilter* filters, int, const char* default_location) {
  callback = cb;
  userdata = data;
  filter = filters;
  location = default_location;
}

TEST_CASE("File dialogs return asynchronously and filter selected paths") {
  auto future = xpano::gui::file_dialog::Open(
      {.type = xpano::gui::ActionType::kOpenFiles}, nullptr);
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
  auto future = xpano::gui::file_dialog::Open(
      {.type = xpano::gui::ActionType::kOpenFiles}, nullptr);
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
  auto future =
      xpano::gui::file_dialog::Save(std::string("panorama.jpg"), nullptr);
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
  auto future = xpano::gui::file_dialog::Open(
      {.type = xpano::gui::ActionType::kOpenDirectory}, nullptr);
  REQUIRE_FALSE(many);
  const char* files[] = {"nonexistent-file-dialog-test-directory", nullptr};
  Complete(files);
  REQUIRE(future.get().error().type ==
          xpano::gui::file_dialog::ErrorType::kTargetNotDirectory);
}

TEST_CASE("Dialog callback remains safe after the future is discarded") {
  {
    auto future = xpano::gui::file_dialog::Save("panorama.jpg", nullptr);
  }
  REQUIRE(std::string(location) == "panorama.jpg");
  const char* files[] = {"panorama.jpg", nullptr};
  Complete(files);
}

TEST_CASE("Unknown file dialog actions fail without launching a dialog") {
  auto future = xpano::gui::file_dialog::Open({}, nullptr);
  REQUIRE(future.get().error().type ==
          xpano::gui::file_dialog::ErrorType::kUnknownAction);
}
