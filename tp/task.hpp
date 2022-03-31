#pragma once

#include "../unique_function/function.hpp"

namespace exe::tp {

// Intrusive tasks?
using Task = wheels::UniqueFunction<void()>;

}  // namespace exe::tp
