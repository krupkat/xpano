#pragma once

#include <atomic>
#include <chrono>
#include <future>

#include "xpano/constants.h"

namespace xpano::utils::future {

template <typename TType>
bool IsReady(const std::future<TType>& future) {
  return future.valid() &&
         future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

struct Cancelled {};

template <typename TType>
TType GetWithCancellation(std::future<TType> future,
                          const std::atomic_int& cancel) {
  for (auto status = std::future_status::timeout;
       status != std::future_status::ready;
       status = future.wait_for(kCancellationTimeout)) {
    if (cancel > 0) {
      throw Cancelled();
    }
  }
  return future.get();
}

}  // namespace xpano::utils::future
