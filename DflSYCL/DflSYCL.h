//==============================================================
// Copyright © Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================
#pragma once

#include <sycl/sycl.hpp>
#include <vector>
#include <iostream>

using namespace sycl;

typedef std::vector<int> IntVector;

void InitializeVector(IntVector &a);
void VectorAdd(queue &q, const IntVector &a_vector, const IntVector &b_vector, IntVector &sum_parallel, size_t num_repetitions = 1);
