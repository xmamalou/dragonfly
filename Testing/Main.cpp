// This is a dummy project, used for testing

#include <iostream>
#include <thread>

#include <atomic>

#include "../Dragonfly/Dragonfly.hxx"

class Rendering {
    Dfl::Hardware::Device& Device;

    std::atomic<bool>            Close{ false };
public:
    Rendering(Dfl::Hardware::Device& device);

          void operator() ();
    const bool ShouldClose() const noexcept { return this->Close; };
};

Rendering::Rendering(Dfl::Hardware::Device& device) : Device(device) {}

void Rendering::operator() () 
{
    using DflWinRect = Dfl::UI::Window::Rectangle;

    try {
        const Dfl::UI::Window::Info winInfo{
           .Resolution{ Dfl::UI::Window::DefaultResolution },
           .DoFullscreen{ false },
           .WindowTitle{ L"Χαίρε Κόσμε!" },
        };
        Dfl::UI::Window window(winInfo);
    
        const Dfl::Graphics::Renderer::Info renderInfo{
            .AssocDevice{ this->Device },
            .AssocWindow{ window },
        };
        Dfl::Graphics::Renderer renderer(renderInfo);

        std::cout << "Resolution of the window is: (" << window.GetRectangle<DflWinRect::Resolution>()[0] << ", " << window.GetRectangle<DflWinRect::Resolution>()[1] << ").\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        window.SetRectangle<DflWinRect::Resolution>({1000, 1000})
              .SetRectangle<DflWinRect::Position>({500, 500})
              .SetTitle(L"Νέος τίτλος");

        std::cout << "Resolution of the window is: (" << window.GetRectangle<DflWinRect::Resolution>()[0] << ", " << window.GetRectangle<DflWinRect::Resolution>()[1] << ").\n";


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
        std::cout << "Processor information: Cores - " << session.GetCharacteristics().CPU.Count << "\n\t\t       Speed - " << session.GetCharacteristics().CPU.Speed << " MHz\n";
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

        const Dfl::Memory::GenericBuffer::Info bufferInfo{
            .MemoryBlock{ memory },
            .Size{ Dfl::MakeBinaryPower(10) },
            .Options{ Dfl::NoOptions }
        };
        Dfl::Memory::GenericBuffer buffer(bufferInfo);

        Rendering render(device);

        std::thread renderThread(std::ref(render));

        struct TestStruct {
            uint32_t Num1{ 10 };
            uint32_t Num2{ 10 };
            uint32_t Num3{ 10 };
            uint32_t Num4{ 10 };
            uint32_t Num5{ 10 };
            uint32_t Num6{ 10 };
        } test1{ 0, 1, 2, 3, 4, 5 };
        auto task{ buffer.Write(test1, 0, 8) };
        task.Resume();

        while (render.ShouldClose() == false){
            
            printf("");
        }

        renderThread.join();

        TestStruct test2{};
        auto task2{ buffer.Read(test2, 0, 0) };
        task2.Resume();

        std::cout << test2.Num2 << "\n";

        return 0;
    }
    catch (Dfl::Error::Generic& err) {
        std::wcout << err.GetError() << "\n";
        return 1;
    }
};