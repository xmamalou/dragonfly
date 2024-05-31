#include "pch.h"
#include "CppUnitTest.h"

#include "Specialization.hpp"

#include <iostream>

import Dragonfly;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Tests
{
	TEST_CLASS(WindowTesting)
	{
	public:
		TEST_METHOD(WindowCreationTest)
		{
			Dfl::Observer::WindowInfo info;

			Dfl::Observer::Window window(info);
			Assert::AreEqual(Dfl::Observer::WindowError::Success, window.OpenWindow());
		}

		TEST_METHOD(WindowCreationNullResolutionTest)
		{
			Dfl::Observer::WindowInfo info =
			{
				.Resolution{ std::make_tuple(0, 0) }
			};

			Dfl::Observer::Window window(info);
			Assert::AreEqual(Dfl::Observer::WindowError::Success, window.OpenWindow());

			
		}

		TEST_METHOD(WindowCreationUTFTitleTest)
		{
			Dfl::Observer::WindowInfo info =
		    {
                .WindowTitle{ "τεστ" }
            };

            Dfl::Observer::Window window(info);
            Assert::AreEqual(Dfl::Observer::WindowError::Success, window.OpenWindow());

			while (window.CloseStatus() == false)
			{
                std::cout << "";
            }
		}
	};

	TEST_CLASS(SessionTesting)
	{
	public:
		TEST_METHOD(SessionCreationTest)
		{
			Dfl::Hardware::SessionInfo sesInfo;

			Dfl::Hardware::Session session(sesInfo);
			Assert::AreEqual(Dfl::Hardware::SessionError::Success, session.InitVulkan());
		}

		TEST_METHOD(SessionCreationWithDebugTest)
		{
			Dfl::Hardware::SessionInfo sesInfo =
			{
				.DoDebug{ true }
			};

			Dfl::Hardware::Session session(sesInfo);
			Assert::AreEqual(Dfl::Hardware::SessionError::Success, session.InitVulkan());
		}

		TEST_METHOD(SessionCreationNoRenderingTest)
		{
			Dfl::Hardware::SessionInfo sesInfo =
			{
				.EnableOnscreenRendering{ false }
			};

			Dfl::Hardware::Session session(sesInfo);
			Assert::AreEqual(Dfl::Hardware::SessionError::Success, session.InitVulkan());
		}

		TEST_METHOD(DeviceInitIndexOutOfBoundsTest)
		{
			Dfl::Hardware::SessionInfo sesInfo;
			Dfl::Hardware::Session session(sesInfo);
			session.InitVulkan();

			Dfl::Hardware::GPUInfo gpuInfo = {
				.DeviceIndex{ 1000 }
			};

			Assert::AreEqual(Dfl::Hardware::SessionError::VkDeviceIndexOutOfRangeError, session.InitDevice(gpuInfo));
		}
	};
}
