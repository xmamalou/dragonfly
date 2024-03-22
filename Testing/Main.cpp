// This is a dummy project, used for testing

#include <iostream>
#include <thread>

#include <atomic>

import Dragonfly;

class Rendering {
    Dfl::Hardware::Device* const pDevice{ nullptr };

    std::atomic<bool>            Close{ false };
public:
    Rendering(Dfl::Hardware::Device* const pDevice);

          void operator() ();
    const bool ShouldClose() const noexcept { return this->Close; };
};

Rendering::Rendering(Dfl::Hardware::Device* const pDevice) : pDevice(pDevice) {}

void Rendering::operator() () {
    const Dfl::Observer::WindowInfo winInfo{
       .Resolution{ Dfl::Observer::DefaultResolution },
       .DoFullscreen{ false },
       .DoVsync{ true }, 
       .Rate{ Dfl::Observer::DefaultRate },
       .WindowTitle{ L"Χαίρε Κόσμε!" },
    };
    Dfl::Observer::Window window(winInfo);
    if (window.GetErrorCode() < Dfl::Observer::WindowError::Success) {
        throw 1;
    }
    
    const Dfl::Graphics::RendererInfo renderInfo{
        .pAssocDevice{ this->pDevice },
        .pAssocWindow{ &window },
    };
    Dfl::Graphics::Renderer renderer(renderInfo);
    if (renderer.GetErrorCode() < Dfl::Graphics::RendererError::Success) {
        throw 1;
    }

    while ( ( this->Close = window.GetCloseStatus() ) == false) {
        renderer.Cycle();
    }

    std::cout << "Done with the window\n";
}

int main() {
    const Dfl::Hardware::SessionInfo sesInfo{
        .AppName{ "My super app" },
        .AppVersion{ Dfl::MakeVersion(0, 0, 1) },
        .DoDebug{ true }
    };
    Dfl::Hardware::Session session(sesInfo);
    if (session.GetErrorCode() < Dfl::Hardware::SessionError::Success)
        return 1;

    std::cout << "You have " << session.GetDeviceCount() << " Vulkan capable device(s) in your system.\n";
    std::cout << "Processor information: Cores - " << session.GetCPU().Count << "\n\t\t       Speed - " << session.GetCPU().Speed << " MHz\n";
    std::cout << "Memory information: " << session.GetMemory() << " MB\n";

    const Dfl::Hardware::DeviceInfo gpuInfo{
        .pSession{ &session },
        .DeviceIndex{ 0 },
        .RenderOptions{ Dfl::Hardware::RenderOptions::Raytracing },
    };
    Dfl::Hardware::Device device(gpuInfo);
    if (device.GetErrorCode() < Dfl::Hardware::DeviceError::Success)
        return 1;

    std::cout << "\nYour device's name is " << device.GetCharacteristics().Name << "\n";
    std::cout << "\nThe session can also tell your device's name: " << session.GetDeviceName(0) << "\n";

    const Dfl::Memory::BlockInfo memoryInfo{
        .pDevice{ &device },
        .Size{ Dfl::MakeBinaryPower(20) }
    };
    Dfl::Memory::Block memory(memoryInfo);
    if (memory.GetErrorCode() < Dfl::Memory::BlockError::Success)
        return 1;

    const Dfl::Memory::BufferInfo bufferInfo{
        .pMemoryBlock{ &memory },
        .Size{ Dfl::MakeBinaryPower(10) },
        .Options{ Dfl::NoOptions }
    };
    Dfl::Memory::Buffer buffer(bufferInfo);
    if (buffer.GetErrorCode() < Dfl::Memory::BufferError::Success)
        return 1;

    const uint32_t* pData = new uint32_t[ Dfl::MakeBinaryPower(10)/sizeof(uint32_t) ];

    Rendering render(&device);

    std::thread renderThread(std::ref(render));

    while (render.ShouldClose() == false){
        auto task = buffer.Write(pData, Dfl::MakeBinaryPower(10), 0);
        printf("");
    }

    renderThread.join();

    delete[] pData;

    return 0;
};