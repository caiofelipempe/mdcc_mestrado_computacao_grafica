#pragma once

#include <vector>

namespace sort {

template <typename T, typename Compare>
void selectSort(std::vector<T>& arr, Compare comp);

}

#include "selectsort.inl"