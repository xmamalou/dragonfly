// This is a dummy project, used for testing

#include <iostream>
#include <thread>

#include <atomic>

#include <Windows.h>
#include <dwmapi.h>

//#include "winrt/base.h"

#include <Dragonfly.hxx>

class Rendering {
    const Dfl::Hardware::Device&  Device;

    std::atomic<bool>             Close{ false };
public:
    Rendering(const Dfl::Hardware::Device& device);

          void operator() ();
    const bool ShouldClose() const noexcept { return this->Close; };
};

Rendering::Rendering(const Dfl::Hardware::Device& device) : Device(device) {}

void Rendering::operator() () {
    try 
    { 
        const Dfl::UI::Window::Info winInfo{
           .Resolution{ Dfl::UI::Window::DefaultResolution },
           .DoFullscreen{ false },
           .DoVsync{ true }, 
           .Rate{ Dfl::UI::Window::DefaultRate },
           .WindowTitle{ L"Χαίρε Κόσμε!" },
        };
        Dfl::UI::Window window(winInfo);
    
        const Dfl::Graphics::Renderer::Info renderInfo{
            .AssocDevice{ this->Device },
            .AssocWindow{ window },
        };
        Dfl::Graphics::Renderer renderer(renderInfo);

        while ( ( this->Close = window.GetCloseStatus() ) == false) {
            renderer.Cycle();
        }

        std::cout << "Done with the window\n";
    }
    catch (Dfl::Error::Generic& err) {
        std::wcout << err.GetError() << std::endl;
        this->Close = true;
    }
}

int main() {
    Dfl::Initialize();

    try {
        const Dfl::SessionInfo sesInfo{
            .AppName{ "My super app" },
            .AppVersion{ Dfl::MakeVersion(0, 0, 1) },
            .DoDebug{ true }
        };
        Dfl::Hardware::Session session(sesInfo);

        std::cout << "You have " << session.GetDeviceCount() << " Vulkan capable device(s) in your system.\n";

        const Dfl::DevInfo gpuInfo{
            .Session{ session },
            .DeviceIndex{ 0 },
            .RenderOptions{ Dfl::DevOptions::Raytracing },
        };
        Dfl::Device device(gpuInfo);

        std::cout << "\nYour device's name is " << device.GetCharacteristics().Name << "\n";
        std::cout << "\nThe session can also tell your device's name: " << session.GetDeviceName(0) << "\n";

        Rendering render(device);
        std::thread renderThread(std::ref(render));

        const Dfl::Memory::Block::Info blockInfo{
            .Device{ device },
            .Size{ Dfl::MakeBinaryPower(10) }
        };
        Dfl::Memory::Block block(blockInfo);

        const Dfl::Memory::Buffer::Info buffInfo{
            .MemoryBlock{ block },
            .Size{ Dfl::MakeBinaryPower(8) }
        };
        Dfl::Memory::Buffer buffer(buffInfo);

        const char* test = "This is a test. The 'Write' function should transfer it to the GPU. Later, the 'Read' function should be able to read it back. Simple, yes?";

        auto writeOP{ buffer.Write(test, 140, 0, 0) };
        writeOP.Resume();

        while (render.ShouldClose() == false){
            printf("");
        }

        const char* test2 = new char[140];
        auto readOP{ buffer.Read((void*)test2, 140, 0) };
        readOP.Resume();

        std::cout << test2;

        delete[] test2;

        renderThread.join();

        Dfl::Terminate();

    }
    catch (Dfl::Error::Generic& err) {
        std::wcout << err.GetError() << std::endl;
    }

    return 0;
};