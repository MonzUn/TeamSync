#pragma once
#include <stdint.h>

struct MirageRect
{
	MirageRect() = default;
	MirageRect(int32_t posX, int32_t posY, int32_t width, int32_t height) : PosX(posX), PosY(posY), Width(width), Height(height) {}
	bool IsValid() const { return PosX >= 0 && PosY >= 0 && Width > 0 && Height > 0; }

	int32_t PosX = -1;
	int32_t PosY = -1;
	int32_t Width = -1;
	int32_t Height = -1;
};