#pragma once

#include <Windows.h>

#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <bitset>
#include <atomic>

#include "Maths/Maths.hpp"

const u32 OBJECT_COUNT = 30000;
const u32 CELL_SIZE = 64;
const u32 BOID_CHUNK = 512;
const float BOID_DIST_MAX = 64.0f;
const float BOID_DIST_MIN = 12.0f;
const float BOID_MAX_SPEED = 250.0f;
const float BOID_CURSOR_DIST = 256.0f;

struct PoolTask
{
	u32 taskID;
	u32 cellX;
	u32 cellY;
	float deltaTime;
};

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
	const std::vector<Maths::Vec4> &GetSimulationData() const;
	const Maths::Mat4 &GetViewProjectionMatrix() const;

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
	Maths::IVec2 cellCount;
	std::atomic<u64> storedRes;
	Maths::Vec2 storedDelta;
	Maths::Vec3 position = Maths::Vec3(-5.30251f, 6.38824f, -7.8891f);
	Maths::Vec2 rotation = Maths::Vec2(static_cast<f32>(M_PI_2) - 1.059891f, 0.584459f);
	f32 fov = 3.55f;
	f64 appTime = 0;
	Maths::Vec2 cursorPos;
	std::atomic_bool mousePressed = false;

	std::vector<Maths::Vec2> positions;
	std::vector<Maths::Vec2> velocities;
	std::vector<Maths::Vec2> accels;
	std::vector<float> rotations;

	std::vector<std::vector<u32>> cells;

	Maths::Mat4 vpA;
	Maths::Mat4 vpB;
	std::vector<Maths::Vec4> bufferA;
	std::vector<Maths::Vec4> bufferB;
	std::atomic_bool currentBuf = false;

	std::vector<std::thread> threadPool;
	std::vector<PoolTask> tasks;
	std::atomic_uint32_t taskCounter;
	std::atomic_bool poolExit;
	std::mutex taskLock;

	void ThreadFunc();
	void HandleResize();
	void InitThread();
	void PreUpdate();
	void Update(float deltaTime);
	void PostUpdate(float deltaTime);
	void UpdateBuffers();
	float NextFloat01();
	s32 GetCell(Maths::IVec2 pos, Maths::IVec2 &dt);
	void ThreadPoolFunc();
	bool ThreadPoolUpdate();
	void ProcessCellUpdate(u32 x, u32 y, float deltaTime);
	void ProcessPostUpdate(u32 start, u32 end, float deltaTime);
};
