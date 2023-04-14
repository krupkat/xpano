#pragma once

#define ALPACA_EXCLUDE_SUPPORT_STD_ARRAY
#define ALPACA_EXCLUDE_SUPPORT_STD_CHRONO
#define ALPACA_EXCLUDE_SUPPORT_STD_DEQUE
#define ALPACA_EXCLUDE_SUPPORT_STD_LIST
#define ALPACA_EXCLUDE_SUPPORT_STD_MAP
#define ALPACA_EXCLUDE_SUPPORT_STD_OPTIONAL
#define ALPACA_EXCLUDE_SUPPORT_STD_SET
#define ALPACA_EXCLUDE_SUPPORT_STD_STRING
#define ALPACA_EXCLUDE_SUPPORT_STD_PAIR
#define ALPACA_EXCLUDE_SUPPORT_STD_UNIQUE_PTR
#define ALPACA_EXCLUDE_SUPPORT_STD_UNORDERED_MAP
#define ALPACA_EXCLUDE_SUPPORT_STD_UNORDERED_SET
#define ALPACA_EXCLUDE_SUPPORT_STD_VARIANT
#define ALPACA_EXCLUDE_SUPPORT_STD_VECTOR

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <vector>

#include <alpaca/alpaca.h>
#include <spdlog/spdlog.h>

namespace xpano::utils::serialize {

template <typename TType>
[[nodiscard]] std::error_code SerializeWithVersion(
    const std::filesystem::path& path, const TType& value) {
  std::ofstream ostream(path, std::ios::binary);
  if (!ostream) {
    spdlog::warn("Failed to open {}", path.string());
    return std::make_error_code(std::errc::io_error);
  }

  std::vector<std::uint8_t> buffer;
  auto bytes_written =
      alpaca::serialize<alpaca::options::with_version>(value, buffer);

  if (!ostream.write(reinterpret_cast<char*>(buffer.data()), bytes_written)) {
    spdlog::warn("Failed to write to {}", path.string());
    return std::make_error_code(std::errc::io_error);
  }
  return {};
}

enum class DeserializeStatus {
  kNoSuchFile,
  kBreakingChange,
  kUnknownError,
  kSuccess,
};

template <typename TType>
struct DeserializeResult {
  DeserializeStatus status;
  TType value;
};

template <typename TType>
DeserializeResult<TType> DeserializeWithVersion(
    const std::filesystem::path& path) {
  std::ifstream istream(path, std::ios::binary);
  if (!istream) {
    spdlog::warn("Failed to open {}", path.string());
    return {DeserializeStatus::kNoSuchFile};
  }

  auto size = std::filesystem::file_size(path);
  std::vector<std::uint8_t> buffer(size);
  if (!istream.read(reinterpret_cast<char*>(buffer.data()),
                    std::ssize(buffer))) {
    spdlog::warn("Failed to read from {}", path.string());
    return {DeserializeStatus::kUnknownError};
  }

  std::error_code error_code;
  auto recovered = alpaca::deserialize<alpaca::options::with_version, TType>(
      buffer, error_code);

  if (!error_code) {
    return {DeserializeStatus::kSuccess, recovered};
  }

  if (error_code == std::errc::invalid_argument) {
    spdlog::warn("Version mismatch in {}", path.string());
    return {DeserializeStatus::kBreakingChange};
  }

  return {DeserializeStatus::kUnknownError};
}

}  // namespace xpano::utils::serialize
