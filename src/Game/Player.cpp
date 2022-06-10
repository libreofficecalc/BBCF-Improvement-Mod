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
char* Player::GetCBRInput() const
{
	return CbrInputOverride;
}
void Player::SetCBRInputValue(int inputInt)
{
	*CbrInputOverride = inputInt;
}


bool Player::GetCBROverride() const
{
	return CBROverride;
}
void Player::SetCBROverride(bool b)
{
	CBROverride = b;
}
void Player::makeCBRInput()
{
	CbrInputOverride = new char();
}