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
TType WaitWithCancellation(std::future<TType> future,
                           const std::atomic_int& cancel) {
  std::future_status status = future.wait_for(kCancellationTimeout);
  while (status != std::future_status::ready) {
    if (cancel > 0) {
      throw Cancelled();
    }
    status = future.wait_for(kCancellationTimeout);
  }
  return future.get();
}

}  // namespace xpano::utils::future
