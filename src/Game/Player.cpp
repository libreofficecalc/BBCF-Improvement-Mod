#include "Player.h"

void Player::SetCharDataPtr(const void* addr)
{
	m_charData = (CharData**)addr;
}

bool Player::IsCharDataNullPtr() const
{
	if (m_charData == 0)
		return true;

	return *m_charData == 0;
}

CharData* Player::GetData() const
{
	return *m_charData;
}

CharPaletteHandle& Player::GetPalHandle()
{
	return m_charPalHandle;
}

int Player::GetInput() const
{
	return input;
}

void Player::SetInput(int i)
{
	input = i;
}

void Player::setAnnotatedReplay(AnnotatedReplay r) {
	aReplay = r;
}
AnnotatedReplay* Player::getAnnotatedReplay() {
	return &aReplay;
}

void Player::setCbrData(CbrData c) {
	cbrData = c;
}

CbrData* Player::getCbrData() {
	return &cbrData;
}

void Player::addReversalReplay(AnnotatedReplay ar) {
	reversalReplays.push_back(ar);
}
void Player::deleteReversalReplays() {
	reversalReplays.clear();
}
void Player::deleteReversalReplay(int i) {
	reversalReplays.erase(reversalReplays.begin() + i);
}
AnnotatedReplay* Player::getReversalReplay(int i) {
	return &reversalReplays[i];
}