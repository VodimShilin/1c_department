#pragma once
#include <functional>
//#include "function2.hpp"

// https://github.com/Naios/function2

namespace wheels {
    template <typename Signature>
    using UniqueFunction = std::function<Signature>;
//    using UniqueFunction = fu2::unique_function<Signature>;
}