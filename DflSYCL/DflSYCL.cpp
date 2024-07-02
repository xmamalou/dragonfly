//==============================================================
// Copyright © Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================
#include "DflSYCL.h"

#include <string>
#if defined(FPGA_HARDWARE) || defined(FPGA_EMULATOR) || defined(FPGA_SIMULATOR)
#include <sycl/ext/intel/fpga_extensions.hpp>
#endif

//************************************
// Vector add in SYCL on device: returns sum in 4th parameter "sum_parallel".
//************************************
void VectorAdd(queue &q, const IntVector &a_vector, const IntVector &b_vector,
               IntVector &sum_parallel, size_t num_repetitions) {
    // Create the range object for the vectors managed by the buffer.
    range<1> num_items{ a_vector.size() };

    // Create buffers that hold the data shared between the host and the devices.
    // The buffer destructor is responsible to copy the data back to host when it
    // goes out of scope.
    buffer a_buf(a_vector);
    buffer b_buf(b_vector);
    buffer sum_buf(sum_parallel.data(), num_items);

    for (size_t i = 0; i < num_repetitions; i++) {

        // Submit a command group to the queue by a lambda function that contains the
        // data access permission and device computation (kernel).
        q.submit([&](handler &h) {
            // Create an accessor for each buffer with access permission: read, write or
            // read/write. The accessor is a mean to access the memory in the buffer.
            accessor a(a_buf, h, read_only);
            accessor b(b_buf, h, read_only);

            // The sum_accessor is used to store (with write permission) the sum data.
            accessor sum(sum_buf, h, write_only, no_init);

            // Use parallel_for to run vector addition in parallel on device. This
            // executes the kernel.
            //    1st parameter is the number of work items.
            //    2nd parameter is the kernel, a lambda that specifies what to do per
            //    work item. The parameter of the lambda is the work item id.
            // SYCL supports unnamed lambda kernel by default.
            h.parallel_for(num_items, [=](auto i) { sum[i] = a[i] + b[i]; });
            });
    };
    // Wait until compute tasks on GPU done
    q.wait();
}

//************************************
// Initialize the vector from 0 to vector_size - 1
//************************************
void InitializeVector(IntVector &a) {
    for (size_t i = 0; i < a.size(); i++) a.at(i) = i;
}
