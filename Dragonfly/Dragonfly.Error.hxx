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

namespace Dfl {
    enum class API {
        None = 0,
        Win32 = 2,
        Vulkan = 4
    };

    // Dragonfly.Error
    namespace Error {
        class Generic {
        protected:
            const wchar_t* ErrorString{ L"There was an error" };
            const wchar_t* ThrowingFunction{ L"Irrelevant" };
                  Dfl::API Source{ Dfl::API::None };
        public:
                           Generic(
                                const wchar_t* errString, 
                                const wchar_t* throwingFunc,
                                const Dfl::API& source) noexcept 
                                : ErrorString(errString), 
                                  ThrowingFunction(throwingFunc),
                                  Source(source) {};

            const wchar_t* GetError() const noexcept { 
                                return this->ErrorString; };
            const wchar_t* GetThrowingFunction() const noexcept { 
                                return this->ThrowingFunction; }
            const Dfl::API GetSource() const noexcept { 
                                return this->Source; };
        };

        // RUNTIME ERRORS

        class Runtime : public Generic {
        public:
            Runtime(
                const wchar_t* errString, 
                const wchar_t* throwingFunc,
                const Dfl::API& source = Dfl::API::Vulkan) noexcept 
                : Generic(errString, throwingFunc, source) {};
        };
        
        // Error useful when an API failed to fetch data for an object
        class NoData : public Runtime {
        public:
            NoData(
                const wchar_t* errString, 
                const wchar_t* throwingFunc,
                const Dfl::API& source = Dfl::API::Vulkan) noexcept 
                : Runtime(errString, throwingFunc, source) {};
        };
        
        // Error useful when an API failed to create a handle for an object
        class HandleCreation : public Runtime {
        public:
            HandleCreation(
                const wchar_t* errString, 
                const wchar_t* throwingFunc,
                const Dfl::API& source = Dfl::API::Vulkan) noexcept 
                : Runtime(errString, throwingFunc, source) {};
        };

        class System : public Runtime {
        public:
            System(
                const wchar_t* errString, 
                const wchar_t* throwingFunc,
                const Dfl::API& source = Dfl::API::Vulkan) noexcept 
                : Runtime(errString, throwingFunc, source) {}; 
        };

        // Error useful when an action cannot continue because a limit (for example in resources) was hit.
        class Limit : public Runtime {
        public:
            Limit(
                const wchar_t* errString, 
                const wchar_t* throwingFunc,
                const Dfl::API& source = Dfl::API::Vulkan) noexcept 
                : Runtime(errString, throwingFunc, source) {};


        };

        // Error useful when attempting to access illegal memory addresses or invalid pointers
        class OutOfBounds : public Runtime {
        public:
            OutOfBounds(
                const wchar_t* errString, 
                const wchar_t* throwingFunc,
                const Dfl::API& source = Dfl::API::Vulkan) noexcept 
                : Runtime(errString, throwingFunc, source) {};
        };

        //
    }
}