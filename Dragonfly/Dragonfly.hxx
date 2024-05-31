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

#pragma once

// Dragonfly.hxx

#include "Dragonfly.h"

#include <stdint.h>

#include <array>
#include <memory>
#include <optional>
#include <vector>
#include <coroutine>
#include <string>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

namespace Dfl{
    DFL_API inline           
                     void     DFL_CALL Initialize() noexcept;
    DFL_API inline
                     void     DFL_CALL Terminate() noexcept;

           consteval uint32_t          MakeVersion(uint32_t major, uint32_t minor, uint32_t patch){ return (major << 16) | (minor << 8) | patch; };
           constexpr uint32_t          EngineVersion{ MakeVersion(0, 1, 0) };

           consteval uint64_t          MakeBinaryPower(uint64_t power) { uint64_t value{ 1 }; while(power > 0) { value *= 2; power--; } return value; }
           constexpr uint64_t          Kilo { MakeBinaryPower(10) };
           constexpr uint64_t          Mega { MakeBinaryPower(20) };
           constexpr uint64_t          Giga { MakeBinaryPower(30) };
           constexpr uint64_t          Peta { MakeBinaryPower(40) };

           constexpr uint32_t          NoOptions{ 0 };
    
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
                  Dfl::API Source{ Dfl::API::None };
        public:
                           Generic(const wchar_t* errString, const Dfl::API& source) noexcept : ErrorString(errString), Source(source) {};

            const wchar_t* GetError() const noexcept { return this->ErrorString; };
            const Dfl::API GetSource() const noexcept { return this->Source; };
        };

        // RUNTIME ERRORS

        class Runtime : public Generic {
        public:
            Runtime(const wchar_t* errString, const Dfl::API& source = Dfl::API::Vulkan) noexcept : Generic(errString, source) {};
        };
        
        // Couldn't get any data from an API
        class NoData : public Runtime {
        public:
            NoData(const wchar_t* errString, const Dfl::API& source = Dfl::API::Vulkan) noexcept : Runtime(errString, source) {};
        };

        class HandleCreation : public Runtime {
        public:
            HandleCreation(const wchar_t* errString, const Dfl::API& source = Dfl::API::Vulkan) noexcept : Runtime(errString, source) {};
        };

        class System : public Runtime {
        public:
            System(const wchar_t* errString, const Dfl::API& source = Dfl::API::Vulkan) noexcept : Runtime(errString, source) {};
        };

        class Limit : public Runtime {
        public:
            Limit(const wchar_t* errString, const Dfl::API& source = Dfl::API::Vulkan) noexcept : Runtime(errString, source) {};
        };

        //
    }

    // Dragonfly.Generics
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

            Tree&    operator [] (const uint32_t node) { if (node < NodeNumber) { return this->Nodes[node] != nullptr ? *this->Nodes[node] : *this; } else { return *this; } }
            T&       operator = (const uint32_t value) { this->NodeValue = value; return this->NodeValue; }

            uint32_t GetDepth() const { return this->CurrentDepth; }
            T        GetNodeValue() const { return this->NodeValue; }
            bool     HasBranch(uint32_t position) const { return position < NodeNumber ? (position < this->CurrentEmptyNode ? true : false) : false; }

            void     MakeBranch(const T& value) { if (this->CurrentEmptyNode < NodeNumber) { this->Nodes[this->CurrentEmptyNode] = new Tree(value, this->CurrentDepth + 1); this->CurrentEmptyNode++; } }
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

    namespace Memory {
        class Block;
        class Buffer;
    }

    namespace Graphics {
        class Renderer;
    }

    // Dragonfly.Hardware
    namespace Hardware {
        // Dragonfly.Hardware.Session
        class Session {
        public:
            struct Info {
                const std::string_view AppName{ "Dragonfly App" };
                const uint32_t         AppVersion{ 0 };

                const bool             DoDebug{ false };
            };

            struct Handles {
                const VkInstance                    hInstance{ nullptr };
                const VkDebugUtilsMessengerEXT      hDbMessenger{ nullptr };
                const std::vector<VkPhysicalDevice> hDevices{};

                operator VkInstance() const { return this->hInstance; }
            };

            struct Characteristics {
                const struct Processor {
                    const uint32_t Count{ 0 }; // amount of processors in the machine
                    const uint64_t Speed{ 0 }; // speed in MHz
                }               CPU{ 0, 0 };
                const uint64_t  Memory{ 0 };
            };

        protected:
            const std::unique_ptr<const Info>            pInfo{ };
            const std::unique_ptr<const Characteristics> pCharacteristics{ };

            const Handles                                Instance{ };
        public:
            DFL_API DFL_CALL Session(const Info& info);
            DFL_API DFL_CALL ~Session();

            //

            const VkInstance       GetInstance() const noexcept { 
                                        return this->Instance; }
            const uint64_t         GetDeviceCount() const noexcept { 
                                        return this->Instance.hDevices.size(); }
            DFL_API 
            inline 
            const std::string     
                          DFL_CALL GetDeviceName(const uint32_t& index) const noexcept;
            const VkPhysicalDevice GetDeviceHandle(const uint32_t& index) const noexcept { 
                                        return index >= this->Instance.hDevices.size() 
                                                    ? nullptr 
                                                    : this->Instance.hDevices[index]; }
            const Characteristics& GetCharacteristics() {
                                        return *this->pCharacteristics; }
        };

        template< typename T > 
        concept SessionLike = requires (T a, uint32_t index)
        {
            typename T::Info;
            typename T::Handles;
            typename T::Characteristics;

            { a.GetInstance() } noexcept 
                -> Dfl::Generics::Equal<VkInstance>;
            { a.GetDeviceCount() } noexcept
                -> Dfl::Generics::Equal<uint64_t>;
            { a.GetDeviceHandle(index) } noexcept 
                -> Dfl::Generics::Equal<VkPhysicalDevice>;
        };

        // Dragonfly.Hardware.Device
        class Device {
        public:
            enum class RenderOptions : unsigned int {
                Raytracing = 1,
            };

            struct Info {
                const Session&        Session; // the session the device belongs to
                const uint32_t        DeviceIndex{ 0 }; // the index of the device in the session's device list

                const uint32_t        RenderersNumber{ 1 };
                const DflGen::BitFlag RenderOptions{ 0 };
                const uint32_t        SimulationsNumber{ 1 };
            };

            enum class MemoryType : unsigned int {
                Local,
                Shared
            };

            struct Queue {
                enum class Type : unsigned int {
                    Graphics = 1,
                    Compute = 2,
                    Transfer = 4,
                };

                struct Family {
                    uint32_t             Index{ 0 };
                    uint32_t             QueueCount{ 0 };
                    DflGen::BitFlag      QueueType{ 0 };
                };

                const VkQueue  hQueue{ nullptr };
                const uint32_t FamilyIndex{ 0 };
                const uint32_t Index{ 0 };

                operator VkQueue() const { return this->hQueue; }
            };

            struct Fence {
                const VkFence  hFence{ nullptr };
                const uint32_t QueueFamilyIndex{ 0 };
                const uint32_t QueueIndex{ 0 };

                operator VkFence () { return this->hFence; }
            };

            struct Characteristics {
                const std::string                             Name{ "Placeholder GPU Name" };

                template < MemoryType type >
                struct Memory {
                    struct Properties {
                        uint32_t     TypeIndex{ 0 };

                        bool         IsHostVisible{ false };
                        bool         IsHostCoherent{ false };
                        bool         IsHostCached{ false };
                    };

                    VkDeviceSize            Size{ 0 };
                    uint32_t                HeapIndex{ 0 };

                    std::vector<Properties> MemProperties{ };

                    operator VkDeviceSize() const { return this->Size; }
                };
                const std::vector<Memory<MemoryType::Local>>  LocalHeaps{ };
                const std::vector<Memory<MemoryType::Shared>> SharedHeaps{ };

                const std::array<uint32_t, 2>                 MaxViewport{ {0, 0} };
                const std::array<uint32_t, 2>                 MaxSampleCount{ {0, 0} }; // {colour, depth}

                const std::array<uint32_t, 3>                 MaxGroups{ {0, 0, 0} }; // max amount of groups the device supports
                const uint64_t                                MaxAllocations{ 0 };

                const uint64_t                                MaxDrawIndirectCount{ 0 };
            };

            struct Tracker {
                uint64_t                    Allocations{ 0 };
                uint64_t                    IndirectDraws{ 0 };

                std::vector<uint32_t>       LeastClaimedQueue{ };
                std::vector<uint32_t>       AreFamiliesUsed{ };

                std::vector<uint64_t>       UsedLocalMemoryHeaps{ }; // size is the amount of heaps
                std::vector<uint64_t>       UsedSharedMemoryHeaps{ }; // size is the amount of heaps

                std::vector<Fence>          Fences{ };

                VkDeviceMemory              hStageMemory{ nullptr };
                VkDeviceSize                StageMemorySize{ 0 };
                VkBuffer                    hStageBuffer{ nullptr };

                void*                       pStageMemoryMap{ nullptr };
            };

            struct Handles {
                const VkDevice                    hDevice{ nullptr };
                const VkPhysicalDevice            hPhysicalDevice{ nullptr };
                const std::vector<Queue::Family>  Families{ };
                const bool                        WithTimelineSems{ false };
                const bool                        WithRTX{ false };

                operator VkDevice() const { return this->hDevice; }
                operator VkPhysicalDevice() const { return this->hPhysicalDevice; }
            };

        protected:
            const std::unique_ptr<const Info>             pInfo{ };
            const std::unique_ptr<const Characteristics>  pCharacteristics{ };

            const Handles                                 GPU{ };
            const std::unique_ptr<      Tracker>          pTracker{ };

            DFL_API 
            const VkFence 
            DFL_CALL GetFence(
                        const uint32_t queueFamilyIndex,
                        const uint32_t queueIndex) const;

            DFL_API
                  Tracker&
            DFL_CALL GetTracker() const noexcept { 
                        return *this->pTracker; }
                          
        public:
            DFL_API DFL_CALL Device(const Info& info);
            DFL_API DFL_CALL ~Device();

            //

            const Session&                     GetSession() const noexcept { 
                                                    return this->pInfo->Session; }
            const Characteristics&             GetCharacteristics() const noexcept { 
                                                    return *this->pCharacteristics; }
            const VkDevice                     GetDevice() const noexcept {
                                                    return this->GPU.hDevice; }
            const VkPhysicalDevice             GetPhysicalDevice() const noexcept {
                                                    return this->GPU.hPhysicalDevice; }
            const decltype(Handles::Families)& GetQueueFamilies() const noexcept {
                                                    return this->GPU.Families; }
            
            friend class
                Dfl::Graphics::Renderer;
            friend class
                Dfl::Memory::Block;
            friend class
                Dfl::Memory::Buffer;
        };
    }
    namespace DflHW = Dfl::Hardware;

    // Dragonfly.Memory
    namespace Memory {
        class Buffer;

        // Dragonfly.Memory.Block
        class Block {
        public:
            struct Info {
                const DflHW::Device& Device;
                const uint64_t       Size{ 0 }; // in B
            };

            struct Handles {
                const VkDeviceMemory       hMemory{ nullptr };
                const DflHW::Device::Queue TransferQueue{ };
                const VkCommandPool        hCmdPool{ nullptr };

                operator VkDeviceMemory () const { return this->hMemory; }
            };

            // NOTE: The following is only used as a suggestion by the allocator
            // Actual size of stage memory may vary. There may not even be a stage
            // memory if there's not enough memory to spare
            static constexpr uint64_t StageMemorySize{ 65536 }; // in B

        protected:
            const std::unique_ptr<const Info>    pInfo{ nullptr };
            const Handles                        Memory{};
                  DflGen::BinaryTree<uint64_t>   MemoryLayout;

            DFL_API 
            auto        
            DFL_CALL Alloc(const VkBuffer& buffer) -> std::optional< std::array<uint64_t, 2> >;
            DFL_API 
            void        
            DFL_CALL Free(
                        const std::array<uint64_t, 2>& memoryID,
                        const uint64_t                 size);
            
        public:
            DFL_API DFL_CALL Block(const Info& info);
            DFL_API DFL_CALL ~Block();

            const DflHW::Device&        GetDevice() const noexcept { 
                                            return this->pInfo->Device; }
            const DflHW::Device::Queue& GetQueue() const noexcept {
                                            return this->Memory.TransferQueue; }
            const VkCommandPool         GetCmdPool() const noexcept {
                                            return this->Memory.hCmdPool; }

            friend class
                    Buffer;
        };

        // Dragonfly.Memory.Buffer
        class Buffer {
        public:
            struct Info {
                      Block&                              MemoryBlock;
                const uint64_t                            Size{ 0 };

                const DflGen::BitFlag                     Options{ 0 };
            };

            struct Handles {
                const VkBuffer        hBuffer{ nullptr };

                // CmdBuffers

                const VkEvent         hCPUTransferDone{ nullptr };
                const VkCommandBuffer hTransferCmdBuff{ nullptr };

                operator const VkBuffer() { return this->hBuffer; }
            };

            enum class Error {
                Success = 0,
                WriteError = -1,
                UnreadableError = -2,
                ReadError = -3,
                EventSetError = -4,
                RecordError = -5    
            };

        protected:
            const std::unique_ptr<const Info> pInfo{ nullptr };

            const Handles                     Buffers{ };

            const std::array<uint64_t, 2>     MemoryLayoutID{ 0, 0 };
                  VkFence                     QueueAvailableFence{ nullptr };

        public:
            DFL_API                          DFL_CALL Buffer(const Info& info);
            DFL_API                          DFL_CALL ~Buffer();

            DFL_API const DflGen::Job<Error> DFL_CALL Write(
                                                        const void*    pData,
                                                        const uint64_t sourceSize,
                                                        const uint64_t sourceOffset,
                                                        const uint64_t dstOffset) const noexcept;
            DFL_API const DflGen::Job<Error> DFL_CALL Read(
                                                              void*     pDestination,
                                                        const uint64_t& dstSize,
                                                        const uint64_t& sourceOffset) const noexcept;
        };
    }
    namespace DflMem = Dfl::Memory;

    namespace UI { class Window; }

    // Dragonfly.Graphics
    namespace Graphics {
        // Dragonfly.Graphics.Renderer
        class Renderer {
        public:
            struct Info {
                const DflHW::Device&   AssocDevice;
                const Dfl::UI::Window& AssocWindow;
            };

            enum class [[ nodiscard ]] Error {
                Success = 0,
                // error
                NullHandleError = -0x1001,
                VkWindowNotAssociatedError = -0x4601,
                VkSurfaceCreationError = -0x4602,
                VkNoSurfaceFormatsError = -0x4603,
                VkNoSurfacePresentModesError = -0x4604,
                VkComPoolInitError = -0x4702,
                VkSwapchainInitError = -0x4801,
                VkSwapchainSurfaceLostError = -0x4802,
                VkSwapchainWindowUnavailableError = -0x4803,
                VkBufferFenceCreationError = -0x4A02,
                // warnings
                ThreadNotReadyWarning = 0x1001,
                VkAlreadyInitDeviceWarning = 0x4201,
                VkNoRenderingRequestedWarning = 0x4202,
                VkUnknownQueueTypeWarning = 0x4701,
            };

            struct Handles {
                const VkSurfaceKHR                    hSurface{ nullptr };
                const DflHW::Device::Queue            AssignedQueue{ };

                      VkSwapchainKHR                  hSwapchain{ nullptr };
                      std::vector<VkImage>            hSwapchainImages{ };
                      VkCommandPool                   hCmdPool{ nullptr };
                // ^ why is this here, even though command pools are per family? The reason is that command pools need to be
                // used only by the thread that created them. Hence, it is not safe to allocate one command pool per family, but rather
                // per thread.

                Handles( const Info& info );
                operator VkSwapchainKHR() { return this->hSwapchain; }
            };

            struct Characteristics {
                const std::array< uint32_t, 2>        TargetRes{ 0, 0 };

                const VkSurfaceCapabilitiesKHR        Capabilities{ 0 };
                const std::vector<VkSurfaceFormatKHR> Formats;
                const std::vector<VkPresentModeKHR>   PresentModes;
            };

            enum class State {
                Initialize,
                Loop,
                Fail
            };

        protected:
            const std::unique_ptr<const Info>            pInfo;
            const Handles                                Swapchain;
            const std::unique_ptr<const Characteristics> pCharacteristics;
            const VkFence                                QueueFence{ nullptr };

                  State                                  CurrentState{ State::Initialize };
        public:
            DFL_API             DFL_CALL Renderer(const Info& info);
            DFL_API             DFL_CALL ~Renderer();

            DFL_API       void  DFL_CALL Cycle();
        };
    }
    namespace DflGr = Dfl::Graphics;

    // Dragonfly.UI
    namespace UI {
        // Dragonfly.UI.Window
        class Window
        {
        public:
            struct Info {
                std::array<uint32_t, 2>         Resolution{ DefaultResolution };
                std::array<uint32_t, 2>         View{ DefaultResolution }; // currently equivalent to resolution
                bool                            DoFullscreen{ false };

                bool                            DoVsync{ true };
                uint32_t                        Rate{ 60 };

                std::array<int, 2>              Position{ {0, 0} }; // Relative to the screen space
                std::basic_string_view<wchar_t> WindowTitle{ L"Dragonfly App" };

                bool                            HasTitleBar{ true };
                bool                            Extends{ false }; // whether the draw area covers the titlebar as well

                const HWND                      hWindow{ nullptr }; // instead of creating a new window, set this to render to children windows
            };
            struct Handles {
                const HWND   hWin32Window{ nullptr };

                operator HWND() { return this->hWin32Window; }
            };

            DFL_API static constexpr uint32_t   DefaultWidth{ 1920 };
            DFL_API static constexpr uint32_t   DefaultHeight{ 1080 };
            DFL_API static constexpr std::array DefaultResolution{ DefaultWidth, DefaultHeight };

            DFL_API static constexpr uint32_t   DefaultRate{ 60 };

        protected:
            const  std::unique_ptr<Info> pInfo{ nullptr };
            const  Handles               Win32{ };

        public:
            DFL_API DFL_CALL Window(const Info& info);
            DFL_API DFL_CALL ~Window();

            const HWND GetHandle() const noexcept { 
                          return this->Win32.hWin32Window; }
            DFL_API 
            inline 
            const bool        
            DFL_CALL   GetCloseStatus() const noexcept;

            DFL_API 
            const Window&        
            DFL_CALL   SetTitle(const wchar_t* newTitle) const noexcept;

            friend class
                Dfl::Graphics::Renderer;
        };

        // Dragonfly.UI.Dialog
        class Dialog : public Window {
        public:
            enum class Type {
                Info,
                Warning,
                Error,
                Question,
                Custom
            };

            struct Info {
                DflGen::WindowsString Title{ L"Dragonfly Message Box" };
                DflGen::WindowsString Message{ L"Hello, World!" };
                Type                  DialogType{ Type::Info };
            };

            enum class [[ nodiscard ]] Result : uint32_t {
                NotReady = 0,
                OK = 1,
                Cancel = 2,
            };

            DFL_API static constexpr uint32_t Width{ 400 };
            DFL_API static constexpr uint32_t Height{ 200 };

        public:
            DFL_API DFL_CALL Dialog(const Info& info);
            DFL_API DFL_CALL ~Dialog();

            DFL_API 
            inline
            const Result  
            DFL_CALL GetResult() const noexcept;
        };
    }
    namespace DflUI = Dfl::UI;

    using Session = Hardware::Session;
    using SessionInfo = Session::Info;

    using Device = Hardware::Device;
    using DevInfo = Device::Info;
    using DevOptions = Device::RenderOptions;
}