#pragma once

#include <chrono>
#include <future>

namespace xpano::utils::future {

template <typename TType>
bool IsReady(const std::future<TType>& future) {
  return future.valid() &&
         future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

}  // namespace xpano::utils::future
