#pragma once

#include <Windows.h>

#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <bitset>
#include <atomic>

#include "Maths/Maths.hpp"

const u32 OBJECT_COUNT = 65536;
const u32 WORLD_SIZE = 500;
const u32 CHUNK_COUNT_SIDE = 10;
const u32 CHUNK_COUNT = CHUNK_COUNT_SIDE * CHUNK_COUNT_SIDE * CHUNK_COUNT_SIDE;
const u32 MAX_OBJECTS_PER_CHUNK = OBJECT_COUNT * 4 / CHUNK_COUNT + 1;
const u32 SORT_THREAD_COUNT = 64;
const u32 SORT_OBJECT_COUNT = (OBJECT_COUNT + SORT_THREAD_COUNT - 1) / SORT_THREAD_COUNT;
const u32 SORT_THREAD_OBJECT_PER_CHUNK = MAX_OBJECTS_PER_CHUNK / 4 + 1;

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

enum WindowMessage : u32
{
	NONE = 0,
	FULLSCREEN = 1,
	LOCK_MOUSE = 2,
	EXIT_WINDOW = 3
};

class GameThread
{
public:
	GameThread() = default;
	~GameThread() = default;

	void Init(HWND hwnd, u32 customMsg, Maths::IVec2 res, bool isUnitTest);
	void Resize(s32 x, s32 y);
	bool HasFinished() const;
	void Quit();
	void MoveMouse(Maths::Vec2 delta);
	void SetKeyState(u8 key, u8 scanCode, bool state);
	void SendWindowMessage(WindowMessage msg, u64 payload = 0);
	std::vector<Maths::Vec4> GetInitialSimulationData();
	const Maths::Mat4 &GetViewProjectionMatrix() const;

	static void SendErrorPopup(const std::wstring &err);
	static void SendErrorPopup(const std::string &err);
	static void LogMessage(const std::wstring &msg);
	static void LogMessage(const std::string &msg);
	static bool HasCrashed();

private:
	static HWND hWnd;
	static std::atomic_bool crashed;
	static bool isUnitTest;
	std::thread thread;
	std::chrono::system_clock::duration start = std::chrono::system_clock::duration();
	std::atomic_bool exit;
	std::mutex mouseLock;
	std::mutex keyLock;
	std::bitset<256> keyDown = 0;
	std::bitset<256> keyPress = 0;
	std::bitset<256> keyToggle = 0;
	std::bitset<256> keyCodesDown = 0;
	std::bitset<256> keyCodesPress = 0;
	std::bitset<256> keyCodesToggle = 0;
	Maths::IVec2 res;
	Maths::IVec2 cellCount;
	std::atomic<u64> storedRes;
	Maths::Vec2 storedDelta;
	Maths::Vec3 position = Maths::Vec3(0,3,0);
	Maths::Vec2 rotation = Maths::Vec2(0, (float)(M_PI*5/4));
	Maths::Quat rotationQuat;
	u32 customMessage = 0;
	f32 fov = 70.0f;
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
	void UpdateBuffers(const Maths::Mat4 &mat);
	float NextFloat01();
	Maths::Vec3 NextUnitVector();
	s32 GetCell(Maths::IVec2 pos, Maths::IVec2 &dt);
	void ThreadPoolFunc();
	bool ThreadPoolUpdate();
	void ProcessCellUpdate(u32 x, u32 y, float deltaTime);
	void ProcessPostUpdate(u32 start, u32 end, float deltaTime);
};
