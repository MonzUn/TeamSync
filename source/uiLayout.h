#pragma once
#include "player.h"
#include "team.h"
#include <stdint.h>

const int32_t ImagePositions[MAX_PLAYERS][2]
{
	{ 10,10 },		// Upper left
	{ 965, 10 },	// Upper right
	{ 10,505 },		// Lower left
	{ 965,505 }		// Lower right
};

const int32_t ImagePosAndDimensions[PlayerImage::Count][4]
{
	{ 0,0,300, Player::DEFAULT_HEIGHT },					// Inventory,
	{ 300, 0, 175, 55 },									// Name,
	{ 380, 65, 85, 80 },									// Head,
	{ 382, 245, 85, 80 },									// Backpack,
	{ 380, 155, 85, 80 },									// Body,
	{ 360, 65, 10, 265 },									// BackpackStat,
	{ 475,0,475, Player::DEFAULT_HEIGHT },					// Weapon,
	{ 0,0,Player::DEFAULT_WIDTH, Player::DEFAULT_HEIGHT }	// Fullscreen
};

const int32_t CutPositions1440P[PlayerImage::Count - 1][4] // 2560 * 1440
{
	{ 487, 185, 786, 1255 },	// Inventory
	{ 1075, 42, 1494, 91 },		// Name
	{ 878, 232, 958, 311 },		// Head
	{ 878, 584, 958, 663 },		// Backpack
	{ 878, 494, 958, 574 },		// Body
	{ 840, 495, 850, 751 },		// BackpackStat
	{ 1748, 185, 2383, 1280 },	// Weapon
};

const int32_t CutPositions1080P[PlayerImage::Count - 1][4] // 1920 * 1080
{
	{ 365, 152, 589, 956 },		// Inventory
	{ 842, 44, 1090, 81 },		// Name
	{ 659, 187, 718, 246 },		// Head
	{ 659, 451, 718, 510 },		// Backpack
	{ 659, 384, 718, 443 },		// Body
	{ 630, 384, 637, 575 },		// BackpackStat
	{ 1311, 151, 1788, 973 },	// Weapon
};