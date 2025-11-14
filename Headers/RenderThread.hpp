#pragma once

#include <Windows.h>

#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <bitset>
#include <atomic>

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan.h"

#include "Types.hpp"
#include "Maths/Maths.hpp"

enum LaunchParams : u8
{
	NONE =			0x0,
	ADVANCED =		0x1,
	INVERTED_RB =	0x2,
	BOXDEBUG =		0x4,
	DENOISE =		0x8,
	CLEAR =			0x10,
};

class RenderThread
{
public:
	RenderThread() {};
	~RenderThread() {};

	void Init(HWND hwnd, Maths::IVec2 res);
	void Resize(Maths::IVec2 newRes);
	bool HasFinished() const;
	void Quit();
	f32 GetElapsedTime();
	void MoveMouse(Maths::Vec2 delta);
	void SetKeyState(u8 key, bool state);

private:
	std::thread thread;
	std::chrono::system_clock::duration start = std::chrono::system_clock::duration();
	HWND hwnd = {};
	std::atomic_bool resize;
	std::atomic_bool exit;
	std::atomic_bool queueLock;
	std::mutex mouseLock;
	std::mutex keyLock;
	std::vector<u32> colorBuffer;
	std::bitset<256> keyDown = 0;
	std::bitset<256> keyPress = 0;
	std::bitset<256> keyToggle = 0;
	Maths::IVec2 res;
	Maths::IVec2 storedRes;
	Maths::Vec2 storedDelta;
	Maths::Vec3 position = Maths::Vec3(-5.30251f, 6.38824f, -7.8891f);
	Maths::Vec2 rotation = Maths::Vec2(static_cast<f32>(M_PI_2) - 1.059891f, 0.584459f);
	f32 fov = 3.55f;
	s32 threadID = -1;
	f32 elapsedTime = 0;
	f32 denoiseStrength = 0.2f;

	void ThreadFunc();
	void CopyToScreen(s64 dt);
	void HandleResize();
	void InitThread();
	void LoadAssets();
	void UnloadAssets();

	void CreateVKInstance();
};