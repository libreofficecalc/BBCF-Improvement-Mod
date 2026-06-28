#pragma once
#include "Game/CharData.h"
#include "Core\interfaces.h"

#include <string>

struct IdleActionToggles
{
	bool ukemiStaggerHit = false;
};

class PlayerExtendedData
{
public:
	/**
	* PlayerExtendedData constructor.
	*/
	PlayerExtendedData();

	/**
	* Updates the information of the current frame.
	*/
	void updateCharData(const Player& player);

	/**
	* Updates the useful information of the previous frame.
	*/
	void updatePreviousState();

	/**
	 * @return true if the character is considered to be in an idle action.
	*/
	bool isIdle() const;
	
	/**
	 * @return true if the character is blocking.
	*/
	bool isBlocking() const;

	/**
	 * @return true if the character is being hit or thrown.
	*/
	bool isInHitstun() const;

	/**
	 * @return true if the character is touching the ground (y = 0).
	 * Note that this doesn't equal to being grounded, as the character at y = 0 one frame before actually landing.
	*/
	bool isAirborne() const;

	/**
	 * @return true if the character is moving on the y axis. Complements isAirborne for checking if a character
	 * is static or not in the air.
	*/
	bool isMovingVertically() const;

private:
	CharData* charData;
	std::string previousAction;
	int32_t previousPositionY;
};

extern IdleActionToggles idleActionToggles;