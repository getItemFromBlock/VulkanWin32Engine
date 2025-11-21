#include "GameThread.hpp"

using namespace Maths;

HWND GameThread::hWnd = NULL;

const u8 MOVEMENT_KEYS[6] =
{
	'A', 'E', 'W', 'D', 'Q', 'S'
};

std::string GetFormattedTime()
{
	time_t timeObj;
	time(&timeObj);
	tm pTime = {};
	gmtime_s(&pTime, &timeObj);
	char buffer[256];
	sprintf_s(buffer, 255, "%d-%d-%d_%d-%d-%d", pTime.tm_year+1900, pTime.tm_mon+1, pTime.tm_mday, pTime.tm_hour, pTime.tm_min, pTime.tm_sec);
	return std::string(buffer);
}

void GameThread::Init(HWND hwnd, Maths::IVec2 resIn)
{
	hWnd = hwnd;
	res = resIn;
	thread = std::thread(&GameThread::ThreadFunc, this);
}

void GameThread::MoveMouse(Vec2 delta)
{
	mouseLock.lock();
	storedDelta -= delta;
	mouseLock.unlock();
}

void GameThread::Resize(s32 x, s32 y)
{
	u64 packed = (u32)(x) | ((u64)(y) << 32);
	storedRes = packed;
}

void GameThread::SetKeyState(u8 key, bool state)
{
	keyLock.lock();
	keyDown.set(key, state);
	if (state)
		keyToggle.flip(key);
	keyLock.unlock();
}

void GameThread::SendErrorPopup(const std::string &err)
{
	LogMessage(err);
#ifdef NDEBUG
	MessageBoxA(appData.hWnd, err.c_str(), "Error!", MB_YESNO);
#else
	if (MessageBoxA(hWnd, (err + "\nBreak?").c_str(), "Error!", MB_YESNO) == IDYES)
		DebugBreak();
#endif
}

void GameThread::SendErrorPopup(const std::wstring &err)
{
	LogMessage(err);
#ifdef NDEBUG
	MessageBoxW(appData.hWnd, err.c_str(), L"Error!", MB_YESNO);
#else
	if (MessageBoxW(hWnd, (err + L"\nBreak?").c_str(), L"Error!", MB_YESNO) == IDYES)
		DebugBreak();
#endif
}

void GameThread::LogMessage(const std::string &msg)
{
	OutputDebugStringA(msg.c_str());
}

void GameThread::LogMessage(const std::wstring &msg)
{
	OutputDebugStringW(msg.c_str());
}

void GameThread::HandleResize()
{
	res.x = (s32)(storedRes & 0xffffffff);
	res.y = (s32)(storedRes >> 32);
}

void GameThread::Quit()
{
	exit = true;
	if (thread.joinable())
		thread.join();
}

void GameThread::InitThread()
{
	SetThreadDescription(GetCurrentThread(), L"Game Thread");
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	start = now.time_since_epoch();
}

void GameThread::ThreadFunc()
{
	InitThread();

	u32 counter = 0;
	u32 tm0 = 0;
	while (!exit)
	{
		std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
		auto duration = now.time_since_epoch() - start;
		auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
		f64 iTime = micros / 1000000.0;
		f32 deltaTime = static_cast<f32>(iTime - appTime);
		appTime = iTime;
		u32 tm1 = (u32)(iTime);
		if (tm0 != tm1)
		{
			tm0 = tm1;
			LogMessage("TPS: " + std::to_string(counter) + "\n");
			counter = 0;
		}
		counter++;

		mouseLock.lock();
		Vec2 delta = storedDelta;
		storedDelta = Vec2();
		mouseLock.unlock();
		delta *= 0.005f;
		rotation.x = Util::Clamp(rotation.x - delta.y, static_cast<f32>(-M_PI_2), static_cast<f32>(M_PI_2));
		rotation.y = Util::Mod(rotation.y + delta.x, static_cast<f32>(2 * M_PI));
		Maths::Vec3 dir;
		keyLock.lock();
		for (u8 i = 0; i < 6; ++i)
		{
			f32 key = keyDown.test(MOVEMENT_KEYS[i]);
			dir[i % 3] += (i > 2) ? -key : key;
		}
		f32 fovDir = static_cast<f32>(keyDown.test(VK_UP)) - static_cast<f32>(keyDown.test(VK_DOWN));
		keyPress.reset();
		keyLock.unlock();
		fov = Util::Clamp(fov + fovDir * deltaTime * fov, 0.5f, 100.0f);
		Quat q = Quat::FromEuler(Vec3(rotation.x, rotation.y, 0.0f));
		if (dir.Dot())
		{
			dir = dir.Normalize() * deltaTime * 10;
			position += q * dir;
		}
		
		HandleResize();
		Update();
	}
}