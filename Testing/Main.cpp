// This is a dummy project, used for testing

#include <iostream>

import Dragonfly;

int main()
{
    Dfl::Observer::WindowInfo info =
    {
        .Resolution{ { 1920, 1080 } },
        .DoFullscreen{ false },
        .Rate{ 60 },
        .WindowTitle{ "Mama Mia, Papa pia, I got-a the diarrhoeaaaaaa!!!" },
    };
    Dfl::Observer::Window window(info);

    if (window.InitWindow() < Dfl::Observer::WindowError::Success)
        return 1;

    Dfl::Hardware::SessionInfo sesInfo = {
        .AppName{ "My super app" },
        .AppVersion{ Dfl::MakeVersion(0, 0, 1) },
        .DoDebug{ true }
    };

    Dfl::Hardware::Session session(sesInfo);
    if (session.InitSession() < Dfl::Hardware::SessionError::Success)
        return 1;

    std::cout << "You have " << session.DeviceCount() << " devices in your system.\n";
    
    auto hDevice = session.Device(0);

    Dfl::Hardware::GPUInfo gpuInfo = {
        .EnableOnscreenRendering{ true },
        .EnableRaytracing{ false },
        .VkInstance{ hDevice.Instance },
        .VkPhysicalGPU{ hDevice.GPU },
        .pDstWindows{&window},
    };
    Dfl::Hardware::Device device(gpuInfo);

    std::cout << "\nYour device's name is " << device.DeviceName() << "\n";

    if (device.InitDevice() < Dfl::Hardware::DeviceError::Success)
        return 1;
    
    if (Dfl::Hardware::CreateRenderer(device, window) < Dfl::Hardware::DeviceError::Success)
        return 1;

    while (window.CloseStatus() == false){
        std::cout << "";
    }

    return 0;
}