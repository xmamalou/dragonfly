// This is a dummy project, used for testing

#include <iostream>

import Dragonfly;

int main(){
    Dfl::Observer::WindowInfo winInfo = {
        .Resolution{ { 1920, 1080 } },
        .DoFullscreen{ false },
        .Rate{ 60 },
        .WindowTitle{ u8"Χαίρε Κόσμε!" },
    };
    Dfl::Observer::Window window(winInfo);
    if (window.GetErrorCode() < Dfl::Observer::WindowError::Success)
        return 1;

    Dfl::Hardware::SessionInfo sesInfo = {
        .AppName{ "My super app" },
        .AppVersion{ Dfl::MakeVersion(0, 0, 1) },
        .DoDebug{ true }
    };
    Dfl::Hardware::Session session(sesInfo);
    if (session.GetErrorCode() < Dfl::Hardware::SessionError::Success)
        return 1;

    std::cout << "You have " << session.GetDeviceCount() << " devices in your system.\n";

    Dfl::Hardware::DeviceInfo gpuInfo = {
        .pSession{ &session },
        .DeviceIndex{ 0 },
        .RenderOptions{ Dfl::Hardware::RenderOptions::Raytracing },
    };
    Dfl::Hardware::Device device(gpuInfo);
    if (device.GetErrorCode() < Dfl::Hardware::DeviceError::Success)
        return 1;

    std::cout << "\nYour device's name is " << device.GetCharacteristics().Name << "\n";

    Dfl::Graphics::RendererInfo renderInfo = {
        .pAssocDevice{ &device },
        .pAssocWindow{ &window },
    };
    Dfl::Graphics::Renderer renderer(renderInfo);
    if (renderer.GetErrorCode() < Dfl::Graphics::RendererError::Success)
        return 1;

    while (window.GetCloseStatus() == false){
        std::cout << "";
    }

    return 0;
};