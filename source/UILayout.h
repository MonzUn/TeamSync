#pragma once
#include "Player.h"
#include "GlobalsBlackboard.h"
#include <stdint.h>

namespace UILayout
{
	// ---------- Generic ----------
	constexpr int32_t APPLICATION_WINDOW_WIDTH	= 1920;
	constexpr int32_t APPLICATION_WINDOW_HEIGHT	= 1000;
	constexpr int32_t DEFAULT_UI_BORDER_PADDING = 10;

	// ---------- MainMenu ----------
	constexpr int32_t MAIN_MENU_NUM_BUTTONS			= 2;
	constexpr int32_t MAIN_MENU_BUTTON_SPACING		= 10; // TODODB: Rename so it's understood that this is for the big buttons in the vertical list
	constexpr int32_t MAIN_MENU_IP_TO_PORT_SPACING	= 10;
	constexpr int32_t MAIN_MENU_HORIZONTAL_BUTTON_SPACING = 10;
	constexpr int32_t MAIN_MENU_INPUT_TEXT_BOX_TO_DESCRIPTION_SPACING = 3;
	constexpr int32_t MAIN_MENU_INPUT_TEXT_BOX_HEIGHT = 25;
	constexpr int32_t MAIN_MENU_BUTTON_LIST_TO_FEEDBACK_TEXT_SPACING = 80;

	// Sizes
	constexpr int32_t MAIN_MENU_BIG_BUTTON_WIDTH	= 200;
	constexpr int32_t MAIN_MENU_BIG_BUTTON_HEIGHT	= 100;
	constexpr int32_t MAIN_MENU_SMALL_BUTTON_WIDTH	= 170;
	constexpr int32_t MAIN_MENU_SMALL_BUTTON_HEIGHT = 80;

	constexpr int32_t IP_TEXT_BOX_WIDTH		= 150;
	constexpr int32_t PORT_TEXT_BOX_WIDTH	= 100;
	constexpr int32_t MAIN_MENU_PLAYER_NAME_TEXT_BOX_WIDTH	= 150;

	constexpr int32_t APP_TITLE_TEXT_BOX_WIDTH	= static_cast<int32_t>(APPLICATION_WINDOW_WIDTH * 0.75f);
	constexpr int32_t APP_TITLE_TEXT_BOX_HEIGHT = 100;
	constexpr int32_t APP_VERSION_NUMBER_TEXT_BOX_WIDTH		= 150;
	constexpr int32_t APP_VERSION_NUMBER_TEXT_BOX_HEIGHT	= 50;
	constexpr int32_t MAIN_MENU_FEEDBACK_TEXT_WIDTH			= 300;
	constexpr int32_t MAIN_MENU_FEEDBACK_TEXT_HEIGHT		= 50;

	constexpr int32_t TOTAL_MENU_HEIGHT = (MAIN_MENU_BIG_BUTTON_HEIGHT * MAIN_MENU_NUM_BUTTONS) + (MAIN_MENU_BUTTON_SPACING * (MAIN_MENU_NUM_BUTTONS - 1));

	// Positions
	constexpr int32_t HOST_BUTTON_POS_X			= APPLICATION_WINDOW_WIDTH / 2 - MAIN_MENU_BIG_BUTTON_WIDTH / 2; // TODODB: Rename to first, second, etc, menu button
	constexpr int32_t HOST_BUTTON_POS_Y			= APPLICATION_WINDOW_HEIGHT / 2 - TOTAL_MENU_HEIGHT / 2;
	constexpr int32_t CONNECT_BUTTON_POS_X		= HOST_BUTTON_POS_X;
	constexpr int32_t CONNECT_BUTTON_POS_Y		= HOST_BUTTON_POS_Y + MAIN_MENU_BIG_BUTTON_HEIGHT + MAIN_MENU_BUTTON_SPACING;
	constexpr int32_t QUIT_BUTTON_POS_X			= APPLICATION_WINDOW_WIDTH - MAIN_MENU_SMALL_BUTTON_WIDTH - DEFAULT_UI_BORDER_PADDING;
	constexpr int32_t QUIT_BUTTON_POS_Y			= APPLICATION_WINDOW_HEIGHT - MAIN_MENU_SMALL_BUTTON_HEIGHT - DEFAULT_UI_BORDER_PADDING;
	constexpr int32_t ABOUT_BUTTON_POS_X		= DEFAULT_UI_BORDER_PADDING;
	constexpr int32_t ABOUT_BUTTON_POS_Y		= APPLICATION_WINDOW_HEIGHT - MAIN_MENU_SMALL_BUTTON_HEIGHT - DEFAULT_UI_BORDER_PADDING;
	constexpr int32_t DEV_BLOG_BUTTON_POS_X		= ABOUT_BUTTON_POS_X + MAIN_MENU_SMALL_BUTTON_WIDTH + MAIN_MENU_HORIZONTAL_BUTTON_SPACING;
	constexpr int32_t DEV_BLOG_BUTTON_POS_Y		= ABOUT_BUTTON_POS_Y;
	constexpr int32_t CONTROLS_BUTTON_POS_X		= DEV_BLOG_BUTTON_POS_X + MAIN_MENU_SMALL_BUTTON_WIDTH + MAIN_MENU_HORIZONTAL_BUTTON_SPACING;
	constexpr int32_t CONTROLS_BUTTON_POS_Y		= ABOUT_BUTTON_POS_Y;
	constexpr int32_t REPORT_BUG_BUTTON_POS_X	= CONTROLS_BUTTON_POS_X + MAIN_MENU_SMALL_BUTTON_WIDTH + MAIN_MENU_HORIZONTAL_BUTTON_SPACING;
	constexpr int32_t REPORT_BUG_BUTTON_POS_Y	= ABOUT_BUTTON_POS_Y; // TODODB: Create and use lower button row posY for all these buttons

	constexpr int32_t IP_TEXT_BOX_POS_X		= HOST_BUTTON_POS_X + MAIN_MENU_BIG_BUTTON_WIDTH + MAIN_MENU_BUTTON_SPACING;
	constexpr int32_t IP_TEXT_BOX_POS_Y		= CONNECT_BUTTON_POS_Y + (MAIN_MENU_BIG_BUTTON_HEIGHT / 2) - (MAIN_MENU_INPUT_TEXT_BOX_HEIGHT / 2);
	constexpr int32_t PORT_TEXT_BOX_POS_X	= IP_TEXT_BOX_POS_X + IP_TEXT_BOX_WIDTH + MAIN_MENU_IP_TO_PORT_SPACING;
	constexpr int32_t PORT_TEXT_BOX_POS_Y	= IP_TEXT_BOX_POS_Y;
	constexpr int32_t PLAYER_NAME_TEXT_BOX_POS_X = DEFAULT_UI_BORDER_PADDING;
	constexpr int32_t PLAYER_NAME_TEXT_BOX_POS_Y = DEFAULT_UI_BORDER_PADDING + MAIN_MENU_INPUT_TEXT_BOX_HEIGHT;

	constexpr int32_t APP_TITLE_TEXT_BOX_POS_X = APPLICATION_WINDOW_WIDTH / 2 - APP_TITLE_TEXT_BOX_WIDTH / 2;
	constexpr int32_t APP_TITLE_TEXT_BOX_POS_Y = APPLICATION_WINDOW_HEIGHT / 6;
	constexpr int32_t APP_VERSION_NUMBER_TEXT_BOX_POS_X = APPLICATION_WINDOW_WIDTH / 2 - APP_VERSION_NUMBER_TEXT_BOX_WIDTH / 2;
	constexpr int32_t APP_VERSION_NUMBER_TEXT_BOX_POS_Y = APP_TITLE_TEXT_BOX_POS_Y + APP_TITLE_TEXT_BOX_HEIGHT + 50;
	constexpr int32_t MAIN_MENU_FEEDBACK_TEXT_POS_X = APPLICATION_WINDOW_WIDTH / 2 - MAIN_MENU_FEEDBACK_TEXT_WIDTH / 2;
	constexpr int32_t MAIN_MENU_FEEDBACK_TEXT_POS_Y = HOST_BUTTON_POS_Y + MAIN_MENU_BIG_BUTTON_HEIGHT * MAIN_MENU_NUM_BUTTONS + MAIN_MENU_BUTTON_SPACING * MAIN_MENU_NUM_BUTTONS - 1;

	// ---------- About ----------
	// Sizes
	constexpr int32_t ABOUT_BACK_BUTTON_WIDTH	= 100; // TODODB: Prefix all sizes and positions with the screen in which they are used
	constexpr int32_t ABOUT_BACK_BUTTON_HEIGHT	= 50;
	constexpr int32_t ABOUT_TEXT_BOX_WIDTH	= APPLICATION_WINDOW_WIDTH	- DEFAULT_UI_BORDER_PADDING * 2;
	constexpr int32_t ABOUT_TEXT_BOX_HEIGHT = APPLICATION_WINDOW_HEIGHT - DEFAULT_UI_BORDER_PADDING * 2;

	// Positions
	constexpr int32_t ABOUT_BACK_BUTTON_POS_X = DEFAULT_UI_BORDER_PADDING;
	constexpr int32_t ABOUT_BACK_BUTTON_POS_Y = APPLICATION_WINDOW_HEIGHT - ABOUT_BACK_BUTTON_HEIGHT - DEFAULT_UI_BORDER_PADDING;
	constexpr int32_t ABOUT_TEXT_BOX_POS_X = DEFAULT_UI_BORDER_PADDING;
	constexpr int32_t ABOUT_TEXT_BOX_POS_Y = DEFAULT_UI_BORDER_PADDING;

	// ---------- Multiplayer ----------
	// PlayerPositions
	constexpr int32_t PLAYER_WIDTH								= 945;
	constexpr int32_t PLAYER_HEIGHT								= 485;
	constexpr int32_t PLAYER_EXTERNAL_BORDER_PADDING_X			= 10;
	constexpr int32_t PLAYER_EXTERNAL_BORDER_PADDING_Y			= 10;
	constexpr int32_t PLAYER_INTERNAL_BORDER_PADDING_X			= 10;
	constexpr int32_t PLAYER_INTERNAL_BORDER_PADDING_Y			= 10;

	constexpr int32_t PLAYER_0_POS_X							= PLAYER_EXTERNAL_BORDER_PADDING_X;
	constexpr int32_t PLAYER_0_POS_Y							= PLAYER_EXTERNAL_BORDER_PADDING_Y;
	constexpr int32_t PLAYER_1_POS_X							= PLAYER_0_POS_X + PLAYER_WIDTH + PLAYER_INTERNAL_BORDER_PADDING_X;
	constexpr int32_t PLAYER_1_POS_Y							= PLAYER_0_POS_Y;
	constexpr int32_t PLAYER_2_POS_X							= PLAYER_0_POS_X;
	constexpr int32_t PLAYER_2_POS_Y							= PLAYER_0_POS_Y + PLAYER_HEIGHT + PLAYER_INTERNAL_BORDER_PADDING_Y;
	constexpr int32_t PLAYER_3_POS_X							= PLAYER_1_POS_X;
	constexpr int32_t PLAYER_3_POS_Y							= PLAYER_2_POS_Y;

	constexpr int32_t PLAYER_FRAME_BORDER_SIZE					= 5;
	constexpr int32_t PLAYER_FRAME_WIDTH						= PLAYER_WIDTH + (PLAYER_FRAME_BORDER_SIZE * 2);
	constexpr int32_t PLAYER_FRAME_HEIGHT						= PLAYER_HEIGHT + (PLAYER_FRAME_BORDER_SIZE * 2);
	constexpr int32_t PLAYER_FRAME_RELATIVE_POS_X				= -PLAYER_FRAME_BORDER_SIZE;
	constexpr int32_t PLAYER_FRAME_RELATIVE_POS_Y				= -PLAYER_FRAME_BORDER_SIZE;
	constexpr int32_t PLAYER_FRAME_DEPTH						= 10;

	constexpr int32_t PLAYER_PRIME_INDICATOR_WIDTH				= 32;
	constexpr int32_t PLAYER_PRIME_INDICATOR_HEIGHT				= 32;
	constexpr int32_t PLAYER_PRIME_INDICATOR_RELATIVE_POS_X		= PLAYER_WIDTH - PLAYER_PRIME_INDICATOR_WIDTH;
	constexpr int32_t PLAYER_PRIME_INDICATOR_RELATIVE_POS_Y		= PLAYER_HEIGHT - PLAYER_PRIME_INDICATOR_HEIGHT;
	constexpr int32_t PLAYER_PRIME_INDICATOR_DEPTH				= 8;

	constexpr int32_t PLAYER_DEFAULT_IMAGE_WIDTH				= 520;
	constexpr int32_t PLAYER_DEFAULT_IMAGE_HEIGHT				= 400;
	constexpr int32_t PLAYER_DEFAULT_IMAGE_RELATIVE_POS_X		= (PLAYER_WIDTH / 2) - (PLAYER_DEFAULT_IMAGE_WIDTH / 2);
	constexpr int32_t PLAYER_DEFAULT_IMAGE_RELATIVE_POS_Y		= (PLAYER_HEIGHT / 2) - (PLAYER_DEFAULT_IMAGE_HEIGHT / 2);
	constexpr int32_t PLAYER_DEFAULT_IMAGE_DEPTH				= 6;

	constexpr int32_t PLAYER_STATUS_IMAGE_WIDTH					= 128;
	constexpr int32_t PLAYER_STATUS_IMAGE_HEIGHT				= 128;
	constexpr int32_t PLAYER_STATUS_IMAGE_RELATIVE_POS_X		= PLAYER_DEFAULT_IMAGE_RELATIVE_POS_X + (PLAYER_DEFAULT_IMAGE_WIDTH / 2) - (PLAYER_STATUS_IMAGE_WIDTH / 2);
	constexpr int32_t PLAYER_STATUS_IMAGE_RELATIVE_POS_Y		= PLAYER_DEFAULT_IMAGE_RELATIVE_POS_Y + (PLAYER_DEFAULT_IMAGE_HEIGHT / 2) - (PLAYER_STATUS_IMAGE_HEIGHT);
	constexpr int32_t PLAYER_STATUS_IMAGE_DEPTH					= 5;

	// UI
	constexpr int32_t MULTIPLAYER_PLAYER_NAME_TEXT_BOX_WIDTH		= 175;
	constexpr int32_t MULTIPLAYER_PLAYER_NAME_TEXT_BOX_HEIGHT		= 25;
	constexpr int32_t MULTIPLAYER_PLAYER_NAME_TEXT_BOX_DEPTH		= 4;
	constexpr int32_t MULTIPLAYER_PLAYER_NAME_OFFSET_X				= 300 + (MULTIPLAYER_PLAYER_NAME_TEXT_BOX_WIDTH / 2); // TODODB: Magic number more or less
	constexpr int32_t MULTIPLAYER_PLAYER_NAME_SPLIT_IMAGE_OFFSET_X	= 280; // TODODB: Magic number more or less
	
	constexpr int32_t MULTIPLAYER_PLAYER_NAME_OFFSET_Y			= 10;

	// ImagePosAndDimensions
	constexpr int32_t IMAGE_DEPTH								= 5;
	constexpr int32_t INVENTORY_HALF_HEIGHT						= 408;
	constexpr int32_t INVENTORY_IMAGE_WIDTH						= 77;
	constexpr int32_t INVENTORY_COUNT_WIDTH						= 43;
	constexpr int32_t INVENTORY_IMAGE_TO_COUNT_PADDING			= 1;
	constexpr int32_t INVETORY_PART_PADDING						= 5;
	constexpr int32_t INVENTORY_OFFSET_X						= 0;
	constexpr int32_t INVENTORY_OFFSET_Y						= 0;

	constexpr int32_t GEAR_SLOT_OFFSET_X						= 380;
	constexpr int32_t GEAR_SLOT_OFFSET_Y						= 65;
	constexpr int32_t GEAR_SLOT_PADDING_Y						= 10;
	constexpr int32_t GEAR_SLOT_WIDTH							= 85;
	constexpr int32_t GEAR_SLOT_HEIGHT							= 80;

	constexpr int32_t BACKPACK_STATE_TO_BODY_SLOT_PADDING		= 20;
	constexpr int32_t BACKPACK_STAT_WIDTH						= 10;
	constexpr int32_t BACKPACK_STAT_HEIGHT						= 265;

	constexpr int32_t WEAPON_OFFSET_X							= 475;
	constexpr int32_t WEAPON_OFFSET_Y							= 0;
	constexpr int32_t WEAPON_WIDTH								= 475 - PLAYER_FRAME_BORDER_SIZE;
	constexpr int32_t WEAPON_HEIGHT								= PLAYER_HEIGHT;

	constexpr int32_t FULLSCREEN_OFFSET_X						= 0;
	constexpr int32_t FULLSCREEN_OFFSET_Y						= 0;

	constexpr int32_t INVENTORY_IMAGE_1_POS_X					= INVENTORY_OFFSET_X;
	constexpr int32_t INVENTORY_COUNT_1_POS_X					= INVENTORY_IMAGE_1_POS_X + INVENTORY_IMAGE_WIDTH + INVENTORY_IMAGE_TO_COUNT_PADDING;
	constexpr int32_t INVENTORY_IMAGE_2_POS_X					= INVENTORY_COUNT_1_POS_X + INVENTORY_COUNT_WIDTH + INVETORY_PART_PADDING;
	constexpr int32_t INVENTORY_COUNT_2_POS_X					= INVENTORY_IMAGE_2_POS_X + INVENTORY_IMAGE_WIDTH + INVENTORY_IMAGE_TO_COUNT_PADDING;
	constexpr int32_t GEAR_SLOT_1_POS_Y							= GEAR_SLOT_OFFSET_Y;
	constexpr int32_t GEAR_SLOT_2_POS_Y							= GEAR_SLOT_1_POS_Y + GEAR_SLOT_HEIGHT + GEAR_SLOT_PADDING_Y;
	constexpr int32_t GEAR_SLOT_3_POS_Y							= GEAR_SLOT_2_POS_Y + GEAR_SLOT_HEIGHT + GEAR_SLOT_PADDING_Y;
	constexpr int32_t BACKPACKSTAT_POS_X						= GEAR_SLOT_OFFSET_X - BACKPACK_STATE_TO_BODY_SLOT_PADDING - BACKPACK_STAT_WIDTH;

	// CutPositions1440P
	constexpr int32_t CUT_1440P_INVENTORY_IMAGE_WIDTH			= 77;
	constexpr int32_t CUT_1440P_INVENTORY_IMAGE_HEIGHT			= 534;
	constexpr int32_t CUT_1440P_INVENTORY_COUNTER_WIDTH			= 51;
	constexpr int32_t CUT_1440P_INVENTORY_COUNTER_HEIGHT		= CUT_1440P_INVENTORY_IMAGE_HEIGHT;
	constexpr int32_t CUT_1440P_INVENTORY_IMAGE_COUNTER_PADDING = 2;
	constexpr int32_t CUT_1440P_INVENTORY_IMAGE_COUNTER_OFFSET	= 159;
	constexpr int32_t CUT_1440P_INVENTORY_1_OFFSET_X			= 487;
	constexpr int32_t CUT_1440P_INVENTORY_1_OFFSET_Y			= 185;
	constexpr int32_t CUT_1440P_INVENTORY_2_OFFSET_X			= CUT_1440P_INVENTORY_1_OFFSET_X + CUT_1440P_INVENTORY_IMAGE_WIDTH + CUT_1440P_INVENTORY_IMAGE_COUNTER_PADDING + CUT_1440P_INVENTORY_IMAGE_COUNTER_OFFSET;
	constexpr int32_t CUT_1440P_INVENTORY_2_OFFSET_Y			= CUT_1440P_INVENTORY_1_OFFSET_Y + CUT_1440P_INVENTORY_IMAGE_HEIGHT;
	constexpr int32_t CUT_1440P_GEAR_SLOT_OFFSET_X				= 878;
	constexpr int32_t CUT_1440P_GEAR_SLOT_1_OFFSET_Y			= 232;
	constexpr int32_t CUT_1440P_GEAR_SLOT_2_OFFSET_Y			= CUT_1440P_GEAR_SLOT_1_OFFSET_Y + 263;
	constexpr int32_t CUT_1440P_GEAR_SLOT_3_OFFSET_Y			= CUT_1440P_GEAR_SLOT_2_OFFSET_Y + 89;
	constexpr int32_t CUT_1440P_GEAR_SLOT_WIDTH					= 80;
	constexpr int32_t CUT_1440P_GEAR_SLOT_HEIGHT				= 80;
	constexpr int32_t CUT_1440P_BACKPACK_STAT_OFFSET_X			= 840;
	constexpr int32_t CUT_1440P_BACKPACK_STAT_OFFSET_Y			= 495;
	constexpr int32_t CUT_1440P_BACKPACK_STAT_WIDTH				= 10;
	constexpr int32_t CUT_1440P_BACKPACK_STAT_HEIGHT			= 256;
	constexpr int32_t CUT_1440P_WEAPON_OFFSET_X					= 1748;
	constexpr int32_t CUT_1440P_WEAPON_OFFSET_Y					= 185;
	constexpr int32_t CUT_1440P_WEAPON_WIDTH					= 635;
	constexpr int32_t CUT_1440P_WEAPON_HEIGHT					= 1095;

	// CutPositions1080P
	constexpr int32_t CUT_1080P_INVENTORY_IMAGE_WIDTH			= 58;
	constexpr int32_t CUT_1080P_INVENTORY_IMAGE_HEIGHT			= 401;
	constexpr int32_t CUT_1080P_INVENTORY_COUNTER_WIDTH			= 37;
	constexpr int32_t CUT_1080P_INVENTORY_COUNTER_HEIGHT		= CUT_1080P_INVENTORY_IMAGE_HEIGHT;
	constexpr int32_t CUT_1080P_INVENTORY_IMAGE_COUNTER_PADDING = 2;
	constexpr int32_t CUT_1080P_INVENTORY_IMAGE_COUNTER_OFFSET	= 123;
	constexpr int32_t CUT_1080P_INVENTORY_1_OFFSET_X			= 365;
	constexpr int32_t CUT_1080P_INVENTORY_1_OFFSET_Y			= 140;
	constexpr int32_t CUT_1080P_INVENTORY_2_OFFSET_X			= CUT_1080P_INVENTORY_1_OFFSET_X + CUT_1080P_INVENTORY_IMAGE_WIDTH + CUT_1080P_INVENTORY_IMAGE_COUNTER_PADDING + CUT_1080P_INVENTORY_IMAGE_COUNTER_OFFSET;
	constexpr int32_t CUT_1080P_INVENTORY_2_OFFSET_Y			= CUT_1080P_INVENTORY_1_OFFSET_Y + CUT_1080P_INVENTORY_IMAGE_HEIGHT;
	constexpr int32_t CUT_1080P_GEAR_SLOT_OFFSET_X				= 659;
	constexpr int32_t CUT_1080P_GEAR_SLOT_1_OFFSET_Y			= 175;
	constexpr int32_t CUT_1080P_GEAR_SLOT_2_OFFSET_Y			= CUT_1080P_GEAR_SLOT_1_OFFSET_Y + 197;
	constexpr int32_t CUT_1080P_GEAR_SLOT_3_OFFSET_Y			= CUT_1080P_GEAR_SLOT_2_OFFSET_Y + 67;
	constexpr int32_t CUT_1080P_GEAR_SLOT_WIDTH					= 60;
	constexpr int32_t CUT_1080P_GEAR_SLOT_HEIGHT				= 60;
	constexpr int32_t CUT_1080P_BACKPACK_STAT_OFFSET_X			= 630;
	constexpr int32_t CUT_1080P_BACKPACK_STAT_OFFSET_Y			= 372;
	constexpr int32_t CUT_1080P_BACKPACK_STAT_WIDTH				= 7;
	constexpr int32_t CUT_1080P_BACKPACK_STAT_HEIGHT			= 191;
	constexpr int32_t CUT_1080P_WEAPON_OFFSET_X					= 1311;
	constexpr int32_t CUT_1080P_WEAPON_OFFSET_Y					= 140;
	constexpr int32_t CUT_1080P_WEAPON_WIDTH					= 477;
	constexpr int32_t CUT_1080P_WEAPON_HEIGHT					= 820;

	const int32_t PlayerPositions[Globals::MIRAGE_MAX_PLAYERS][2]
	{
		{ PLAYER_0_POS_X, PLAYER_0_POS_Y },	// Upper left
		{ PLAYER_1_POS_X, PLAYER_1_POS_Y }, // Upper right
		{ PLAYER_2_POS_X, PLAYER_2_POS_Y }, // Lower left
		{ PLAYER_3_POS_X, PLAYER_3_POS_Y }	// Lower right
	};

	const int32_t ImagePosAndDimensions[PlayerImageSlot::Count][5]
	{
		{ INVENTORY_IMAGE_1_POS_X, INVENTORY_OFFSET_Y, IMAGE_DEPTH, INVENTORY_IMAGE_WIDTH, INVENTORY_HALF_HEIGHT },		// InventoryImage1
		{ INVENTORY_COUNT_1_POS_X, INVENTORY_OFFSET_Y, IMAGE_DEPTH, INVENTORY_COUNT_WIDTH , INVENTORY_HALF_HEIGHT },	// InventoryCount1
		{ INVENTORY_IMAGE_2_POS_X, INVENTORY_OFFSET_Y, IMAGE_DEPTH, INVENTORY_IMAGE_WIDTH, INVENTORY_HALF_HEIGHT },		// InvetoryImage2
		{ INVENTORY_COUNT_2_POS_X, INVENTORY_OFFSET_Y, IMAGE_DEPTH, INVENTORY_COUNT_WIDTH, INVENTORY_HALF_HEIGHT },		// InentoryCount2
		{ GEAR_SLOT_OFFSET_X, GEAR_SLOT_1_POS_Y, IMAGE_DEPTH, GEAR_SLOT_WIDTH, GEAR_SLOT_HEIGHT },						// Head,
		{ GEAR_SLOT_OFFSET_X, GEAR_SLOT_2_POS_Y, IMAGE_DEPTH, GEAR_SLOT_WIDTH, GEAR_SLOT_HEIGHT },						// Body,
		{ GEAR_SLOT_OFFSET_X, GEAR_SLOT_3_POS_Y, IMAGE_DEPTH, GEAR_SLOT_WIDTH, GEAR_SLOT_HEIGHT },						// Backpack,
		{ BACKPACKSTAT_POS_X, GEAR_SLOT_OFFSET_Y, IMAGE_DEPTH, BACKPACK_STAT_WIDTH, BACKPACK_STAT_HEIGHT },				// BackpackStat,
		{ WEAPON_OFFSET_X, WEAPON_OFFSET_Y, IMAGE_DEPTH, WEAPON_WIDTH, WEAPON_HEIGHT },									// Weapon,
		{ FULLSCREEN_OFFSET_X, FULLSCREEN_OFFSET_Y, IMAGE_DEPTH, PLAYER_WIDTH, PLAYER_HEIGHT }							// Fullscreen
	};

	const int32_t CutPositions1440P[PlayerImageSlot::Count - 1][4] // 2560 * 1440
	{
		{ CUT_1440P_INVENTORY_1_OFFSET_X, CUT_1440P_INVENTORY_1_OFFSET_Y, CUT_1440P_INVENTORY_IMAGE_WIDTH, CUT_1440P_INVENTORY_IMAGE_HEIGHT },		// InventoryImage1
		{ CUT_1440P_INVENTORY_2_OFFSET_X, CUT_1440P_INVENTORY_1_OFFSET_Y, CUT_1440P_INVENTORY_COUNTER_WIDTH, CUT_1440P_INVENTORY_COUNTER_HEIGHT },	// InventoryCount1
		{ CUT_1440P_INVENTORY_1_OFFSET_X, CUT_1440P_INVENTORY_2_OFFSET_Y, CUT_1440P_INVENTORY_IMAGE_WIDTH, CUT_1440P_INVENTORY_IMAGE_HEIGHT },		// InvetoryImage2
		{ CUT_1440P_INVENTORY_2_OFFSET_X, CUT_1440P_INVENTORY_2_OFFSET_Y, CUT_1440P_INVENTORY_COUNTER_WIDTH, CUT_1440P_INVENTORY_COUNTER_HEIGHT },	// InentoryCount2
		{ CUT_1440P_GEAR_SLOT_OFFSET_X, CUT_1440P_GEAR_SLOT_1_OFFSET_Y, CUT_1440P_GEAR_SLOT_WIDTH, CUT_1440P_GEAR_SLOT_HEIGHT },					// Head
		{ CUT_1440P_GEAR_SLOT_OFFSET_X, CUT_1440P_GEAR_SLOT_2_OFFSET_Y, CUT_1440P_GEAR_SLOT_WIDTH, CUT_1440P_GEAR_SLOT_HEIGHT },					// Backpack
		{ CUT_1440P_GEAR_SLOT_OFFSET_X, CUT_1440P_GEAR_SLOT_3_OFFSET_Y, CUT_1440P_GEAR_SLOT_WIDTH, CUT_1440P_GEAR_SLOT_HEIGHT },					// Body
		{ CUT_1440P_BACKPACK_STAT_OFFSET_X, CUT_1440P_BACKPACK_STAT_OFFSET_Y, CUT_1440P_BACKPACK_STAT_WIDTH, CUT_1440P_BACKPACK_STAT_HEIGHT },		// BackpackStat
		{ CUT_1440P_WEAPON_OFFSET_X, CUT_1440P_WEAPON_OFFSET_Y, CUT_1440P_WEAPON_WIDTH, CUT_1440P_WEAPON_HEIGHT },									// Weapon
	};

	const int32_t CutPositions1080P[PlayerImageSlot::Count - 1][4] // 1920 * 1080
	{
		{ CUT_1080P_INVENTORY_1_OFFSET_X, CUT_1080P_INVENTORY_1_OFFSET_Y, CUT_1080P_INVENTORY_IMAGE_WIDTH, CUT_1080P_INVENTORY_IMAGE_HEIGHT },		// InventoryImage1
		{ CUT_1080P_INVENTORY_2_OFFSET_X, CUT_1080P_INVENTORY_1_OFFSET_Y, CUT_1080P_INVENTORY_COUNTER_WIDTH, CUT_1080P_INVENTORY_COUNTER_HEIGHT },	// InventoryCount1
		{ CUT_1080P_INVENTORY_1_OFFSET_X, CUT_1080P_INVENTORY_2_OFFSET_Y, CUT_1080P_INVENTORY_IMAGE_WIDTH, CUT_1080P_INVENTORY_IMAGE_HEIGHT },		// InvetoryImage2
		{ CUT_1080P_INVENTORY_2_OFFSET_X, CUT_1080P_INVENTORY_2_OFFSET_Y ,CUT_1080P_INVENTORY_COUNTER_WIDTH, CUT_1080P_INVENTORY_COUNTER_HEIGHT },	// InentoryCount2
		{ CUT_1080P_GEAR_SLOT_OFFSET_X, CUT_1080P_GEAR_SLOT_1_OFFSET_Y, CUT_1080P_GEAR_SLOT_WIDTH, CUT_1080P_GEAR_SLOT_HEIGHT },					// Head
		{ CUT_1080P_GEAR_SLOT_OFFSET_X, CUT_1080P_GEAR_SLOT_2_OFFSET_Y, CUT_1080P_GEAR_SLOT_WIDTH, CUT_1080P_GEAR_SLOT_HEIGHT },					// Backpack
		{ CUT_1080P_GEAR_SLOT_OFFSET_X, CUT_1080P_GEAR_SLOT_3_OFFSET_Y, CUT_1080P_GEAR_SLOT_WIDTH, CUT_1080P_GEAR_SLOT_HEIGHT },					// Body
		{ CUT_1080P_BACKPACK_STAT_OFFSET_X, CUT_1080P_BACKPACK_STAT_OFFSET_Y, CUT_1080P_BACKPACK_STAT_WIDTH, CUT_1080P_BACKPACK_STAT_HEIGHT },		// BackpackStat
		{ CUT_1080P_WEAPON_OFFSET_X, CUT_1080P_WEAPON_OFFSET_Y, CUT_1080P_WEAPON_WIDTH, CUT_1080P_WEAPON_HEIGHT },									// Weapon
	};
}