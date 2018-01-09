#pragma once
#include "player.h"
#include "team.h"
#include <stdint.h>

const int32_t ApplicationWindowWidth	= 1920;
const int32_t ApplicationWindowHeight	= 1000;

const int32_t ImagePositions[MAX_PLAYERS][2]
{
	{ 10,10 },		// Upper left
	{ 965, 10 },	// Upper right
	{ 10,505 },		// Lower left
	{ 965,505 }		// Lower right
};

const int32_t ImagePosAndDimensions[PlayerImageSlot::Count][4]
{
	{ 0,0,300, Player::DEFAULT_HEIGHT },					// Inventory,
	{ 300, 0, 175, 55 },									// Name,
	{ 380, 65, 85, 80 },									// Head,
	{ 382, 155, 85, 80 },									// Backpack,
	{ 380, 245, 85, 80 },									// Body,
	{ 360, 65, 10, 265 },									// BackpackStat,
	{ 475,0,475, Player::DEFAULT_HEIGHT },					// Weapon,
	{ 0,0,Player::DEFAULT_WIDTH, Player::DEFAULT_HEIGHT }	// Fullscreen
};

// TODODB: Change cut positions to use width and height instead of upperLeft and lowerRight coordinates
const int32_t CutPositions1440P[PlayerImageSlot::Count - 1][4] // 2560 * 1440
{
	{ 487, 185, 786, 1255 },	// Inventory
	{ 1075, 42, 1494, 91 },		// Name
	{ 878, 232, 958, 311 },		// Head
	{ 878, 494, 958, 574 },		// Backpack
	{ 878, 584, 958, 663 },		// Body
	{ 840, 495, 850, 751 },		// BackpackStat
	{ 1748, 185, 2383, 1280 },	// Weapon
};

const int32_t CutPositions1080P[PlayerImageSlot::Count - 1][4] // 1920 * 1080
{
	{ 365, 140, 589, 942 },		// Inventory
	{ 842, 30, 1090, 67 },		// Name
	{ 659, 175, 718, 234 },		// Head
	{ 659, 372, 718, 431 },		// Backpack
	{ 659, 439, 718, 498 },		// Body
	{ 630, 372, 637, 563 },		// BackpackStat
	{ 1311, 140, 1788, 960 },	// Weapon
};