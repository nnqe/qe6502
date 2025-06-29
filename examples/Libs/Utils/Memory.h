#pragma once
#include <memory>

namespace qe::Examples
{
template<typename T>
using Ptr = std::shared_ptr<T>;
template<typename T, typename ... Args>
inline Ptr<T> MakePtr(Args&&...args){return std::make_shared<T>(std::forward<Args>(args)...);}
}
