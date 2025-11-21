#pragma once

#include "Maths/Maths.hpp"

const u32 OBJECT_COUNT = 20000;

class GameThread
{
public:
	GameThread();
	~GameThread();

	const std::vector<Maths::Vec4> GetSimulationData() const;

private:

};