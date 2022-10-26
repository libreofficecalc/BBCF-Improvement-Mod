#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include "Helper.h"



Helper::Helper()
{
}
//p1PosX, p2PosX, p1PosY, p2PosY, facing, P1Action, P2Action, P1Blockstun, P2Blockstun, P1Hitstun, P2Hitstun, P1AttackType, P2AttackType, p1Hitstop, p2Hitstop, p1ActionTimeNHS, p2ActionTimeNHS
Helper::Helper(int p1x, int p1y, bool b, std::string p1action, int P1Hitstun, int P1AttackType, int p1Hitstop, int p1ActionTimeNHS)
{
	posX = p1x;
	posY = p1y;
	facing = b;
	currentAction = p1action;
	hitstun = P1Hitstun;
	attackType = P1AttackType;
	hitstop = p1Hitstop;
	actionTimeNoHitstop = p1ActionTimeNHS;

}

int Helper::getPosX() {
	return posX;
}
int Helper::getPosY() {
	return posY;
}
bool Helper::getFacing() {
	return facing;
}

std::string Helper::getCurrentAction() {
	return currentAction;
}

size_t Helper::getCurrentActionHash() {
	return currentActionHash;
}

int Helper::getHitstun() {
	return hitstun;
}
int Helper::getAttackType() {
	return attackType;
}
int Helper::getHitstop() {
	return hitstop;
}
int Helper::getActionTimeNHS() {
	return actionTimeNoHitstop;
}
bool Helper::getAttack() {
	return attack;
}
bool Helper::getHit() {
	return hit;
}
bool Helper::getHitThisFrame() {
	return hitThisFrame;
}


std::array< std::string, 13> adaActions = { "CmnActStand",
"_NEUTRAL",
"CmnActBWalk",
"CmnActFWalk",
"CmnActCrouch",
"CmnActStand2Crouch",
"CmnActCrouch2Stand",
"CmnActJumpPre", "CmnActJumpUpper",
"CmnActJumpUpperEnd", "CmnActJumpDown",
"CmnActJumpLanding" };

void Helper::computeMetaData(std::string charName) {
	attack = attackType > 0;
	hit = hitstun > 0;
	hitThisFrame = (hitstun > 0) && (hitstop > 0) && (actionTimeNoHitstop == 1);
	currentActionHash = std::hash<std::string>{}(currentAction);

	if (charName != "ca" || charName != "rl") {
		type = currentAction;
		typeHash = std::hash<std::string>{}(currentAction);
	}

}

bool Helper::CheckNeutralState(std::string state) {
	for (std::size_t i = 0; i < adaActions.size(); ++i) {
		if (state == adaActions[i]) {
			return true;
		}
	}
	return false;
}



std::string Helper::PrintState() {
	std::string str = "";

	str += "PosX: " + std::to_string(posX) + "\n";
	str += "PosY: " + std::to_string(posY) + "\n";
	str += "CurAction: " + currentAction + "\n";
	str += "Attack: " + std::to_string(attack) + "\n";
	//str += "AtkType: " + std::to_string(attackType[0]) + " - " + std::to_string(attackType[1]) + "\n";
	str += "Hit: " + std::to_string(hit) + "\n";
	str += "HitTF: " + std::to_string(hitThisFrame) + "\n";

	return str;
}