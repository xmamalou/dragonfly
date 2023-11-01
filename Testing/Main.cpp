// This is a dummy project, used for testing

#include <iostream>
#include <tuple>

import Dragonfly;

int main()
{
    Dfl::Observer::WindowInfo info =
    {
        .Resolution{ std::make_tuple(1920, 1080) },
        .DoFullscreen{ false },
        .WindowTitle{ "Gorgonzola" },
    };
    Dfl::Observer::Window window(info);
    if (window.OpenWindow() < Dfl::Observer::WindowError::Success)
        return 1;

    Dfl::Hardware::SessionInfo sesInfo = {
        .AppName{ "My super app" },
        .AppVersion{ Dfl::MakeVersion(0, 0, 1) },
        .Criteria{ static_cast<int>(Dfl::Hardware::SessionCriteria::None) }
    };

    Dfl::Hardware::Session session(sesInfo);
    if (session.InitVulkan() < Dfl::Hardware::SessionError::Success)
        return 1;

    if (session.LoadDevices() < Dfl::Hardware::SessionError::Success)
        return 1;
    
    std::cout << "You have " << session.DeviceCount() << " devices in your system." << std::endl;
    std::cout << "Your first device's name is: " << session.DeviceName(0) << std::endl;

    Dfl::Hardware::GPUInfo gpuInfo = {
        .DeviceIndex = 0,
        .pDstWindows{ {&window} },
    };

    auto error = session.InitDevice(gpuInfo);
    if (error < Dfl::Hardware::SessionError::Success)
        return -static_cast<int>(error);

    while (window.CloseStatus() != true)
    {
    }

    return 0;
}