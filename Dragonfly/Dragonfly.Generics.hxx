/*
   Copyright 2024 Christopher-Marios Mamaloukas

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

#pragma once

#include <string>
#include <coroutine>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "Dragonfly.Error.hxx"

namespace Dfl {
    namespace Generics {
        
        // METAFUNCTIONS 

        template< typename T, typename U >
        constexpr bool SameType = false;

        template< typename T >
        constexpr bool SameType< T, T > = true;

        template< typename T, typename U >
        concept Equal = SameType<T, U>;

        // TYPE ALIASES

        using String = std::basic_string<char8_t>;
        using WindowsString = std::basic_string<wchar_t>;

        //

        class BitFlag;

        // CONCEPTS

        template< typename I >
        concept Bitable = std::is_enum_v< I > || std::is_integral_v< I > || SameType< BitFlag, I >;

        template< typename T >
        concept Complete = !std::is_void_v<T>;

        template< typename T >
        concept VulkanStorage = SameType< T, VkBuffer > || SameType< T, VkImage >;

        //

        class BitFlag {
            unsigned int flag{ 0 };
        public:
            BitFlag() {}
            template< Bitable N >
            BitFlag(N num) { this->flag = static_cast<unsigned int>(num); }

            // assignment

            template< Bitable N >
            BitFlag& operator= (const N num) { this->flag = static_cast<unsigned int>(num); return *this; }
            template< Bitable N >
            BitFlag& operator|= (const N num) { this->flag |= static_cast<unsigned int>(num); return *this; }
            template< Bitable N >
            BitFlag& operator&= (const N num) { this->flag &= static_cast<unsigned int>(num); return *this; }

            // bitwise

            template< Bitable N >
            BitFlag       operator| (const N num) const { return BitFlag(this->flag | static_cast<unsigned int>(num)); }
            template< Bitable N >
            BitFlag       operator& (const N num) const { return BitFlag(this->flag & static_cast<unsigned int>(num)); }

            // conversions

            operator unsigned int() { return this->flag; }

            const unsigned int GetValue() const { return this->flag; }
        };

        template< typename T, uint32_t NodeNumber >
        class Tree {
                  T        NodeValue;

                  Tree*    Nodes[NodeNumber];
                  uint32_t CurrentEmptyNode{ 0 };
            const uint32_t CurrentDepth{ 0 };

            Tree(T value, uint32_t depth) : NodeValue(value), CurrentDepth(depth) { }
        public:
            Tree(T value) : NodeValue(value) { }
            ~Tree() { for (uint32_t i = 0; i < this->CurrentEmptyNode; i++) { delete this->Nodes[i]; } }

            operator T () { return this->NodeValue; }

            Tree&    operator [] (const uint32_t node) const { 
                        if (node < NodeNumber) {
                          return this->Nodes[node] != nullptr 
                                 ? *this->Nodes[node] 
                                 : throw Dfl::Error::OutOfBounds(
                                            L"This branch isn't created yet",
                                            L"Tree::operator[]",
                                            Dfl::API::None); }
                        else { throw Dfl::Error::OutOfBounds(
                                        L"Reached bounds of tree", 
                                        L"Tree::operator[]",
                                        Dfl::API::None); } }
            T&       operator = (const T& value) { 
                        this->NodeValue = value; 
                        return this->NodeValue; }

            uint32_t GetDepth() const noexcept { 
                        return this->CurrentDepth; }
            T        GetNodeValue() const noexcept { 
                        return this->NodeValue; }
            bool     HasBranch(uint32_t position) const { 
                        return position < NodeNumber 
                               ? (position < this->CurrentEmptyNode ? true : false) 
                               : false; }

            Tree&    MakeBranch(const T& value) { 
                        if (this->CurrentEmptyNode < NodeNumber) { 
                            this->Nodes[this->CurrentEmptyNode] = new Tree(value, this->CurrentDepth + 1); 
                            this->CurrentEmptyNode++; } 
                        return *this; }
        };

        template< typename T >
        using LinkedList = Tree<T, 1>;

        template< typename T >
        using BinaryTree = Tree<T, 2>;

        // COROUTINE

        template< Complete T >
        class Job {
        public:
            struct Awaiter;
            struct Awaitable;

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
                Job* pJobObject{ nullptr };

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

            Job& Resume() { if (this->State == RoutineState::InProgress) { this->PromiseHandle.resume(); } return *this; }
            Job& Stop() { if (this->State == RoutineState::InProgress) { this->PromiseHandle.destroy(); } return *this; }
            operator T () { this->Resume(); return this->Value; }

            RoutineState GetState() { return this->State; }
        private:
            std::coroutine_handle<promise_type> PromiseHandle{ nullptr };
            T                                   Value;
            RoutineState                        State{ RoutineState::InProgress };

            Job(std::coroutine_handle<promise_type> promiseHandle) : PromiseHandle(promiseHandle) { this->PromiseHandle.promise().pJobObject = this; }
        };

        //
    }
    namespace DflGen = Dfl::Generics;
}