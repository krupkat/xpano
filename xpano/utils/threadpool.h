// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <BS_thread_pool.hpp>

namespace xpano::utils::mt {

template <typename TResultType>
using MultiFuture = BS::multi_future<TResultType>;

using Threadpool = BS::thread_pool;

}  // namespace xpano::utils::mt
