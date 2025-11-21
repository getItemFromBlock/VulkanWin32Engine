#pragma once

#include <Windows.h>

#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <bitset>
#include <atomic>

#include "Maths/Maths.hpp"

const u32 OBJECT_COUNT = 20000;

class GameThread
{
public:
	GameThread() = default;
	~GameThread() = default;

	void Init(HWND hwnd, Maths::IVec2 res);
	void Resize(s32 x, s32 y);
	bool HasFinished() const;
	void Quit();
	void MoveMouse(Maths::Vec2 delta);
	void SetKeyState(u8 key, bool state);
	const std::vector<Maths::Vec4> GetSimulationData() const;

	static void SendErrorPopup(const std::wstring &err);
	static void SendErrorPopup(const std::string &err);
	static void LogMessage(const std::wstring &msg);
	static void LogMessage(const std::string &msg);

private:
	static HWND hWnd;
	std::thread thread;
	std::chrono::system_clock::duration start = std::chrono::system_clock::duration();
	std::atomic_bool exit;
	std::mutex mouseLock;
	std::mutex keyLock;
	std::bitset<256> keyDown = 0;
	std::bitset<256> keyPress = 0;
	std::bitset<256> keyToggle = 0;
	Maths::IVec2 res;
	std::atomic<u64> storedRes;
	Maths::Vec2 storedDelta;
	Maths::Vec3 position = Maths::Vec3(-5.30251f, 6.38824f, -7.8891f);
	Maths::Vec2 rotation = Maths::Vec2(static_cast<f32>(M_PI_2) - 1.059891f, 0.584459f);
	f32 fov = 3.55f;
	f64 appTime = 0;

	void ThreadFunc();
	void HandleResize();
	void InitThread();
	void Update();
};