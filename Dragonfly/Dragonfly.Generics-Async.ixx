/*
   Copyright 2023 Christopher-Marios Mamaloukas

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

module;

#include <coroutine>
#include <type_traits>

#include <vulkan/vulkan.h>

export module Dragonfly.Generics:Async;

namespace Dfl {
    namespace Generics {
        export template< typename T >
        concept Complete = !std::is_void_v<T>;

        export template< Complete T > 
        class Job {
        public:
            struct Promise {
                                   Promise() : PromiseHandle(std::coroutine_handle<Promise>().from_promise(*this)) { };

                Job::Awaiter       await_transform(Job::Awaitable awaitable) { return Job::Awaiter(awaitable); };

                Job                get_return_object() noexcept { return Job(this->PromiseHandle); };
                std::suspend_never initial_suspend() noexcept { return std::suspend_never(); };
                std::suspend_never final_suspend() noexcept { this->pJobObject->State = Job::RoutineState::Done; return std::suspend_never(); };
                void               return_value(T expr) { this->pJobObject->State = Job::RoutineState::Done; this->pJobObject->Value = expr; };
                void               unhandled_exception() {};

            private:
                std::coroutine_handle<Promise>  PromiseHandle{ nullptr };
                Job*                            pJobObject{ nullptr };

                friend Job;
            };
            using promise_type = Promise;

            enum class RoutineState {
                InProgress,
                Done
            };

            struct Awaitable {
                VkDevice hGPU{ nullptr };
                VkFence  hFence{ nullptr };

                Awaitable() : hGPU(nullptr), hFence(nullptr) {}
                Awaitable(VkDevice device, VkFence fence) : hGPU(device), hFence(fence) {}
            };

            struct Awaiter {
                     Awaiter(Awaitable awaitable) : Wait(awaitable) {}

                bool await_ready() { return vkGetFenceStatus(this->Wait.hGPU, this->Wait.hFence) == VK_SUCCESS; };
                bool await_suspend(std::coroutine_handle<promise_type> promiseHandle) { return vkGetFenceStatus(this->Wait.hGPU, this->Wait.hFence) == VK_SUCCESS; };
                void await_resume() { };

            private:
                Awaitable Wait{ nullptr };
            };

                         ~Job() { this->Stop(); }

            Job&         Resume() { if(this->State == RoutineState::InProgress) { this->PromiseHandle.resume(); } return *this; }
            Job&         Stop() { if(this->State == RoutineState::InProgress) { this->PromiseHandle.destroy(); } return *this; }
                         operator T () { this->Resume(); return this->Value; }

            RoutineState GetState() { return this->State; }
        private:
            std::coroutine_handle<promise_type> PromiseHandle{ nullptr };
            T                                   Value;
            RoutineState                        State{ RoutineState::InProgress };

            Job(std::coroutine_handle<promise_type> promiseHandle) : PromiseHandle(promiseHandle) { this->PromiseHandle.promise().pJobObject = this; }
        };
    }
}