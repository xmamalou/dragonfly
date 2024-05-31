// This is a dummy project, used for testing

#include <iostream>
#include <thread>

#include <atomic>

#include "../Dragonfly/Dragonfly.hxx"

class Rendering {
    const Dfl::Hardware::Device& Device;

    std::atomic<bool>            Close{ false };
public:
    Rendering(const Dfl::Hardware::Device& device);

          void operator() ();
    const bool ShouldClose() const noexcept { return this->Close; };
};

Rendering::Rendering(const Dfl::Hardware::Device& device) : Device(device) {}

void Rendering::operator() () 
{
    try {
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

        while ( ( this->Close  = window.GetCloseStatus() ) == false) {
            renderer.Cycle();
        }

        std::cout << "Done with the window\n";
    }
    catch (Dfl::Error::Generic& err) {
        std::wcout << err.GetError() << "\n";
        this->Close = true;
    }
}

int main() {
    try {
        const Dfl::Hardware::Session::Info sesInfo{
            .AppName{ "My super app" },
            .AppVersion{ Dfl::MakeVersion(0, 0, 1) },
            .DoDebug{ true }
        };
        Dfl::Hardware::Session session(sesInfo);

        std::cout << "You have " << session.GetDeviceCount() << " Vulkan capable device(s) in your system.\n";
        std::cout << "Processor information: Cores - " << session.GetCharacteristics().CPU.Count << "\n\t\t       Speed - " << session.GetCPU().Speed << " MHz\n";
        std::cout << "Memory information: " << session.GetCharacteristics().Memory << " MB\n";

        const Dfl::Hardware::Device::Info gpuInfo{
            .Session{ session },
            .DeviceIndex{ 0 },
            .RenderOptions{ Dfl::Hardware::Device::RenderOptions::Raytracing },
        };
        Dfl::Hardware::Device device(gpuInfo);

        std::cout << "\nYour device's name is " << device.GetCharacteristics().Name << "\n";
        std::cout << "\nThe session can also tell your device's name: " << session.GetDeviceName(0) << "\n";

        const Dfl::Memory::Block::Info memoryInfo{
            .Device{ device },
            .Size{ Dfl::MakeBinaryPower(20) }
        };
        Dfl::Memory::Block memory(memoryInfo);

        const Dfl::Memory::Buffer::Info bufferInfo{
            .MemoryBlock{ memory },
            .Size{ Dfl::MakeBinaryPower(10) },
            .Options{ Dfl::NoOptions }
        };
        Dfl::Memory::Buffer buffer(bufferInfo);

        Rendering render(device);

        std::thread renderThread(std::ref(render));

        auto test1{ "This is a test. Dragonfly should be able to write this and read it afterwards.." };
        auto task{ buffer.Write((void*)test1, 80, 0, 0) };
        task.Resume();

        while (render.ShouldClose() == false){
            
            printf("");
        }

        renderThread.join();

        auto test2{ new char[80] };
        auto task2{ buffer.Read((void*) test2, 80, 0) };
        task2.Resume();

        std::cout << test2 << "\n";

        delete[] test2;

        return 0;
    }
    catch (Dfl::Error::Generic& err) {
        std::wcout << err.GetError() << "\n";
        return 1;
    }
};