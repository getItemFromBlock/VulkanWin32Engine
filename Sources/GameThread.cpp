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

float GameThread::NextFloat01()
{
	return rand() / static_cast<float>(RAND_MAX);
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
	MessageBoxA(hWnd, err.c_str(), "Error!", MB_YESNO);
#else
	if (MessageBoxA(hWnd, (err + "\nBreak?").c_str(), "Error!", MB_YESNO) == IDYES)
		DebugBreak();
#endif
}

void GameThread::SendErrorPopup(const std::wstring &err)
{
	LogMessage(err);
#ifdef NDEBUG
	MessageBoxW(hWnd, err.c_str(), L"Error!", MB_YESNO);
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
	cellCount.x = (res.x + CELL_SIZE - 1) / CELL_SIZE;
	cellCount.y = (res.y + CELL_SIZE - 1) / CELL_SIZE;
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
	srand(std::chrono::duration_cast<std::chrono::milliseconds>(start).count());

	positions.resize(OBJECT_COUNT);
	velocities.resize(OBJECT_COUNT);
	accels.resize(OBJECT_COUNT);
	rotations.resize(OBJECT_COUNT);

	bufferA.resize(OBJECT_COUNT);
	bufferB.resize(OBJECT_COUNT);

	for (u32 i = 0; i < OBJECT_COUNT; i++)
	{
		positions[i] = Vec2(NextFloat01() * res.x, NextFloat01() * res.y);
		velocities[i] = (Vec2(NextFloat01(), NextFloat01()) * 2 - 1) * BOID_MAX_SPEED * 0.2f;
	}

	const u32 threadCount = Util::MaxU(std::thread::hardware_concurrency() - 4, std::thread::hardware_concurrency() / 2);
	threadPool.resize(threadCount);
	// Need one extra slot for the "main threa" (Game Thread)
	threadCells.resize(threadCount+1);

	for (u32 i = 0; i < threadCount; i++)
	{
		threadPool[i] = std::thread(&GameThread::ThreadPoolFunc, this);
	}
}

s32 GameThread::GetCell(Maths::IVec2 pos, Maths::IVec2 &dt)
{
	if (pos.x < 0)
	{
		pos.x += cellCount.x;
		dt.x = -res.x;
	}
	else if (pos.x >= cellCount.x)
	{
		pos.x -= cellCount.x;
		dt.x = res.x;
	}
	if (pos.y < 0)
	{
		pos.y += cellCount.y;
		dt.y = -res.y;
	}
	else if (pos.y >= cellCount.y)
	{
		pos.y -= cellCount.y;
		dt.y = res.y;
	}
	return pos.x + pos.y * cellCount.x;
}

void GameThread::PreUpdate()
{
	for (u32 i = 0; i < cells.size(); i++)
		cells[i].clear();

	const u32 totalCells = cellCount.x * cellCount.y;
	if (cells.size() < totalCells)
	{
		cells.resize(totalCells);
		for (u32 i = 0; i < threadCells.size(); i++)
			threadCells[i].resize(totalCells);
	}

	for (u32 i = 0; i < OBJECT_COUNT; i++)
	{
		IVec2 cell = positions[i] / CELL_SIZE;
		cell.x = Util::MinI(cell.x, cellCount.x - 1);
		cell.y = Util::MinI(cell.y, cellCount.y - 1);
		cells[cell.x + cell.y * cellCount.x].push_back(i);
	}
}

#include <unordered_set>

void GameThread::Update(float deltaTime)
{
	taskLock.lock();
	for (u32 cx = 0; cx < cellCount.x; cx++)
	{
		for (u32 cy = 0; cy < cellCount.y; cy++)
		{
			PoolTask task;
			task.taskID = 0;
			task.deltaTime = deltaTime;
			task.cellX = cx;
			task.cellY = cy;
			tasks.push_back(task);
		}
	}
	taskCounter = tasks.size();
	taskLock.unlock();

	while (taskCounter != 0)
		ThreadPoolUpdate();
}

void GameThread::PostUpdate(float deltaTime)
{
	taskLock.lock();
	for (u32 x = 0; x < OBJECT_COUNT; x+= BOID_CHUNK)
	{
		PoolTask task;
		task.taskID = 1;
		task.deltaTime = deltaTime;
		task.cellX = x;
		task.cellY = Util::MinU(x + BOID_CHUNK, OBJECT_COUNT);
		tasks.push_back(task);
	}
	taskCounter = tasks.size();
	taskLock.unlock();

	while (taskCounter != 0)
		ThreadPoolUpdate();
}

void GameThread::UpdateBuffers()
{
	auto &buf = currentBuf ? bufferA : bufferB;

	for (u32 i = 0; i < OBJECT_COUNT; i++)
	{
		Vec2 v = velocities[i];
		rotations[i] = -atan2f(v.y, v.x) - M_PI_2;

		buf[i] = Vec4(positions[i].x, positions[i].y, rotations[i], 0.0f);
	}
	currentBuf = !currentBuf;
}

const std::vector<Maths::Vec4> GameThread::GetSimulationData() const
{
	return currentBuf ? bufferB : bufferA;
}

void GameThread::ProcessCellUpdate(u32 cx, u32 cy, float deltaTime)
{
	const auto &vec1 = cells[cx + cy * cellCount.x];
	for (u32 index1 = 0; index1 < vec1.size(); index1++)
	{
		u32 boid1 = vec1[index1];

		Vec2 globalPos;
		Vec2 globalRot;
		Vec2 avoidDir;
		u32 count = 0;
		u32 avoidCount = 0;

		for (s32 i = -1; i <= 1; i++)
		{
			for (s32 j = -1; j <= 1; j++)
			{
				IVec2 dt;
				s32 cellId = GetCell(IVec2(cx + i, cy + j), dt);

				const auto &vec2 = cells[cellId];
				for (u32 index2 = 0; index2 < vec2.size(); index2++)
				{
					u32 boid2 = vec2[index2];
					if (boid1 == boid2)
						continue;

					Vec2 delta = positions[boid2] - positions[boid1] + dt;
					float distSqr = delta.Dot();
					if (distSqr > BOID_DIST_MAX * BOID_DIST_MAX)
						continue;

					globalPos += delta;
					globalRot += velocities[boid2];
					count++;

					if (distSqr < BOID_DIST_MIN * BOID_DIST_MIN && distSqr > 0)
					{
						float dist = sqrtf(distSqr);
						avoidCount++;
						avoidDir -= delta / (dist * dist * dist) * BOID_DIST_MIN * BOID_DIST_MIN * BOID_DIST_MIN;
					}
				}
			}
		}

		if (count != 0)
		{
			accels[boid1] = (globalPos / count) * 700 + (globalRot / count) * 2500;
			if (avoidCount != 0)
				accels[boid1] += (avoidDir / avoidCount) * 9000;
			accels[boid1] *= deltaTime;
		}
		else
			accels[boid1] = velocities[boid1].Normalize() * deltaTime;

		if (mousePressed)
		{
			Vec2 d = positions[boid1] - cursorPos;
			if (d.Dot() < BOID_CURSOR_DIST * BOID_CURSOR_DIST)
			{
				float len = d.Length();
				accels[boid1] += d / (len * len) * deltaTime * 60000000;
			}
		}
	}
}

void GameThread::ProcessPostUpdate(u32 start, u32 end, float deltaTime)
{
	for (u32 i = start; i < end; i++)
	{
		Vec2 newVel = velocities[i] + accels[i] * deltaTime;
		float len = newVel.Length();
		if (len > BOID_MAX_SPEED)
		{
			newVel = newVel.Normalize() * BOID_MAX_SPEED;
		}
		velocities[i] = newVel;

		Vec2 newPos = positions[i] + velocities[i] * deltaTime;
		if (newPos.x < 0)
			newPos.x += res.x;
		else if (newPos.x >= res.x)
			newPos.x -= res.x;
		if (newPos.y < 0)
			newPos.y += res.y;
		else if (newPos.y >= res.y)
			newPos.y -= res.y;

		positions[i] = newPos;
	}
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
		HandleResize();
		if (res.x <= 0 || res.y <= 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			continue;
		}

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
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(hWnd, &p);
		bool click = GetKeyState(VK_LBUTTON) < 0;
		cursorPos.x = p.x;
		cursorPos.y = p.y;
		mousePressed = click;
		
		// Hard cap movement to 30 fps so that deltatime does not gets too big
		if (deltaTime > 0.033f)
			deltaTime = 0.033f;
		if (appTime > 1)
		{
			PreUpdate();
			Update(deltaTime);
			PostUpdate(deltaTime);
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		
		UpdateBuffers();
	}

	poolExit = true;
	for (u32 i = 0; i < threadPool.size(); i++)
	{
		threadPool[i].join();
	}
}

bool GameThread::ThreadPoolUpdate()
{
	taskLock.lock();
	PoolTask task;
	task.taskID = -1;
	if (!tasks.empty())
	{
		task = tasks.back();
		tasks.pop_back();
	}
	taskLock.unlock();

	if (task.taskID == (u32)(-1))
	{
		return false;
	}
	else
	{
		switch (task.taskID)
		{
		case 0:
			ProcessCellUpdate(task.cellX, task.cellY, task.deltaTime);
			break;
		case 1:
			ProcessPostUpdate(task.cellX, task.cellY, task.deltaTime);
			break;
		default:
			break;
		}
		taskCounter--;
	}
	return true;
}

void GameThread::ThreadPoolFunc()
{
	while (!poolExit)
	{
		if (!ThreadPoolUpdate())
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}