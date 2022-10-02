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

char* Player::GetInput() const
{
	return input;
}

void Player::SetInputValue(char* inputval) 
{
	*input = *inputval;
}
void Player::SetInputPtr(char* ptr)
{
	input = ptr;
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

