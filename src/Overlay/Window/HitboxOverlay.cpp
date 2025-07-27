#include "HitboxOverlay.h"

#include "Core/interfaces.h"
#include "Game/gamestates.h"
#include "Game/Jonb/JonbReader.h"
#include "imgui_internal.h"
#include "Core/utils.h"

void HitboxOverlay::Update()
{
	if (HasNullptrInData() || !m_windowOpen)
	{
		return;
	}

	if (!isHitboxOverlayEnabledInCurrentState())
	{
		return;
	}
	BeforeDraw();

	ImGui::Begin("##HitboxOverlay", nullptr, m_overlayWindowFlags);

	Draw();

	ImGui::End();

	AfterDraw();
}

void HitboxOverlay::BeforeDraw()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));

	this->io = ImGui::GetIO();
	displayRatio = io.DisplaySize.x / io.DisplaySize.y;
	aspectRatioAddress = GetBbcfBaseAdress() + 0x65A5E4;
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));
}

void HitboxOverlay::Draw()
{
	for (int i = 0; i < g_gameVals.entityCount; i++)
	{
		CharData* pEntity = (CharData*)g_gameVals.pEntityList[i];
		const bool isCharacter = i < 2;
		const bool isEntityActive = pEntity->unknownStatus1 == 1 && pEntity->pJonbEntryBegin;

		if (isCharacter || isEntityActive)
		{
			if (!IsOwnerEnabled(pEntity->ownerEntity))
			{
				continue;
			}

			const ImVec2 entityWorldPos = CalculateObjWorldPosition(pEntity);
			DrawCollisionAreas(pEntity, entityWorldPos);
		}
	}
}

void HitboxOverlay::AfterDraw()
{
ImGui::PopStyleColor();
ImGui::PopStyleVar(2);
}

bool HitboxOverlay::IsOwnerEnabled(CharData* ownerCharInfo)
{
	for (int i = 0; i < 2; i++)
	{
		if (ownerCharInfo == (CharData*)g_gameVals.pEntityList[i])
		{
			return drawCharacterHitbox[i];
		}
	}

	return false;
}

bool HitboxOverlay::HasNullptrInData()
{
	return !g_gameVals.pEntityList;
}

ImVec2 HitboxOverlay::CalculateObjWorldPosition(const CharData* charObj)
{
	float posX = charObj->position_x_dupe - charObj->offsetX_1 + charObj->offsetX_2;
	float posY = charObj->position_y_dupe + charObj->offsetY_2;

	return ImVec2(
		floor(posX / 1000 * m_scale),
		floor(posY / 1000 * m_scale)
	);
}

ImVec2 HitboxOverlay::CalculateScreenPosition(ImVec2 worldPos)
{
	D3DXVECTOR3 result;
	D3DXVECTOR3 vec3WorldPos(worldPos.x, worldPos.y, 0.0f);
	WorldToScreen(g_interfaces.pD3D9ExWrapper, g_gameVals.viewMatrix, g_gameVals.projMatrix, &vec3WorldPos, &result);

	return ImVec2(floor(result.x), floor(result.y));
}

ImVec2 HitboxOverlay::RotatePoint(ImVec2 center, float angleInRad, ImVec2 point)
{
	if (!angleInRad)
	{
		return point;
	}

	// translate point back to origin:
	point.x -= center.x;
	point.y -= center.y;

	float s = sin(angleInRad);
	float c = cos(angleInRad);

	// rotate point
	float xNew = point.x * c - point.y * s;
	float yNew = point.x * s + point.y * c;

	// translate point back:
	point.x = xNew + center.x;
	point.y = yNew + center.y;
	return point;
}

void HitboxOverlay::fixAspectRatio(ImVec2& point)
{	
	if (*this->aspectRatioAddress == 1) {
		if (displayRatio > aspectRatio) {
			float scaling = (io.DisplaySize.x / (io.DisplaySize.y * aspectRatio));
			float offset = (io.DisplaySize.x - io.DisplaySize.y * aspectRatio) / 2;

			point.x = point.x / scaling + offset;
		}
		else if (displayRatio < aspectRatio) {
			float scaling = (io.DisplaySize.y / (io.DisplaySize.x / aspectRatio));
			float offset = (io.DisplaySize.y - io.DisplaySize.x / aspectRatio) / 2;

			point.y = point.y / scaling + offset;
		}
	}
}

void HitboxOverlay::DrawOriginLine(ImVec2 worldPos, float rotationRad)
{
	const unsigned int colorOrange = 0xFFFF9900;
	const int horizontalLength = 20;
	const int verticalLength = 50;

	ImVec2 horizontalFrom = RotatePoint(worldPos, rotationRad, ImVec2(worldPos.x - horizontalLength / 2, worldPos.y));
	ImVec2 horizontalTo = RotatePoint(worldPos, rotationRad, ImVec2(worldPos.x + horizontalLength / 2, worldPos.y));
	horizontalFrom = CalculateScreenPosition(horizontalFrom);
	horizontalTo = CalculateScreenPosition(horizontalTo);
	fixAspectRatio(horizontalFrom);
	fixAspectRatio(horizontalTo);
	RenderLine(horizontalFrom, horizontalTo, colorOrange, m_rectThickness);

	ImVec2 verticalFrom = worldPos;
	ImVec2 verticalTo = RotatePoint(verticalFrom, rotationRad, ImVec2(verticalFrom.x, verticalFrom.y + verticalLength));
	verticalFrom = CalculateScreenPosition(verticalFrom);
	verticalTo = CalculateScreenPosition(verticalTo);
	fixAspectRatio(verticalFrom);
	fixAspectRatio(verticalTo);
	RenderLine(verticalFrom, verticalTo, colorOrange, m_rectThickness);
}
void HitboxOverlay::DrawCollisionBoxes(ImVec2 worldPos, float rotationRad, const CharData* charObj)
{
	const unsigned int colorPurple = 0xFF8E00FF;
	int baddY = 0;
	if (charObj->BoundingAddY>0) {
		 baddY = charObj->BoundingAddY;
	}
	int baddX = 0;
	if (charObj->BoundingAddX > 0) {
		baddX = charObj->BoundingAddX;
	}
	int h_len_from_origin = floor((charObj->BoundingX / 2 + (baddX)) * m_scale);
	int v_len_from_origin = floor((charObj->BoundingY + baddY) * m_scale);
	//So far only izanami was found to need this, otherwise during float BoundingAddY adds 110000 units to her collision box height
	if (charObj->BoundingFixY > 0) {
		v_len_from_origin = floor((charObj->BoundingY) * m_scale);
	}


	ImVec2 horizontalFrom = RotatePoint(worldPos, rotationRad, ImVec2(worldPos.x - h_len_from_origin, worldPos.y));
	ImVec2 horizontalTo = RotatePoint(worldPos, rotationRad, ImVec2(worldPos.x + h_len_from_origin, worldPos.y));
	ImVec2 colA = CalculateScreenPosition(horizontalFrom);
	ImVec2 colB = CalculateScreenPosition(horizontalTo);
	fixAspectRatio(colA);
	fixAspectRatio(colB);


	ImVec2 verticalFrom = RotatePoint(worldPos, rotationRad, ImVec2(worldPos.x - h_len_from_origin, worldPos.y + v_len_from_origin));
	ImVec2 verticalTo = RotatePoint(worldPos, rotationRad, ImVec2(worldPos.x + h_len_from_origin, worldPos.y + v_len_from_origin));
	ImVec2 colD = CalculateScreenPosition(verticalFrom);
	ImVec2 colC = CalculateScreenPosition(verticalTo);
	fixAspectRatio(colC);
	fixAspectRatio(colD);





	RenderRect(colA, colB, colC, colD, colorPurple, m_rectThickness);
}

void HitboxOverlay::DrawRangeCheckBoxes(ImVec2 worldPos, float rotationRad, const CharData* charObj) {

	//right now the current situation is:
	// Throw range check must reach the other player's x origin to throw
	// Vector check box must intersect with other player's collision box to throw
	// Prob those obs are incomplete, but that's the current progress
	const int h_col_len = floor((charObj->BoundingX + (charObj->BoundingAddX * 2)) * m_scale);

	const unsigned int colorGreen = 0xFF00FF00;
	const unsigned int colorYellow = 0xFFFFFF00;
	int vcX_1 = 0;
	int vcX_2 = 0;
	int vcY_1 = 0;
	int vcY_2 = 0;

	if (charObj->scaleX != 0) {
		vcX_1 = floor(floor(charObj->vectorcheckX_1 / charObj->scaleX) * m_scale);
		vcX_2 = floor(floor(charObj->vectorcheckX_2 / charObj->scaleX) * m_scale);
	}
	if (charObj->scaleY != 0) {
		vcY_1 = floor(floor(charObj->vectorcheckY_1 / charObj->scaleY) * m_scale);
		vcY_2 = floor(floor(charObj->vectorcheckY_2 / charObj->scaleY) * m_scale);
	}
	if (charObj->facingLeft)
	{
		vcX_2 = -vcX_2;
		vcX_1 = -vcX_1;
	}

	if (charObj->vectorcheckX_1 != -1 || charObj->vectorcheckX_2 != -1 || charObj->vectorcheckY_1 != -1 || charObj->vectorcheckY_2 != -1) {

		if (charObj->vectorcheckX_1 == -1) {
			vcX_1 = -40000;
		}
		if (charObj->vectorcheckX_2 == -1) {
			vcX_2 = 40000;
		}
		if (charObj->vectorcheckY_1 == -1) {
			vcY_1 = 40000;
		}
		if (charObj->vectorcheckY_2 == -1) {
			vcY_2 = -40000;
		}

		ImVec2 tstA = CalculateScreenPosition(RotatePoint(worldPos, rotationRad, ImVec2(worldPos.x + vcX_2, worldPos.y + vcY_2)));
		ImVec2 tstB = CalculateScreenPosition(RotatePoint(worldPos, rotationRad, ImVec2(worldPos.x + vcX_1, worldPos.y + vcY_2)));
		ImVec2 tstC = CalculateScreenPosition(RotatePoint(worldPos, rotationRad, ImVec2(worldPos.x + vcX_1, worldPos.y + vcY_1)));
		ImVec2 tstD = CalculateScreenPosition(RotatePoint(worldPos, rotationRad, ImVec2(worldPos.x + vcX_2, worldPos.y + vcY_1)));
		fixAspectRatio(tstA);
		fixAspectRatio(tstB);
		fixAspectRatio(tstC);
		fixAspectRatio(tstD);
		//RenderLine(tstA, tst2, colorGreen, 3);

		RenderRect(tstA, tstB, tstC, tstD, colorGreen, m_rectThickness / 2);
	}
	if (charObj->ThrowRange > 0) {
		int ThrowRange = 0;
		if (charObj->scaleX != 0) {
			ThrowRange = (charObj->ThrowRange / charObj->scaleX) * m_scale;
		}
		if (charObj->vectorcheckX_1 == -1) {
			vcY_1 = -40000;
		}
		if (charObj->vectorcheckX_2 == -1) {
			vcY_2 = 40000;
		}


		ImVec2 trA = CalculateScreenPosition(RotatePoint(worldPos, rotationRad, ImVec2(worldPos.x - (ThrowRange + h_col_len / 2), worldPos.y + vcY_2)));
		ImVec2 trB = CalculateScreenPosition(RotatePoint(worldPos, rotationRad, ImVec2(worldPos.x + ThrowRange + h_col_len / 2, worldPos.y + vcY_2)));
		ImVec2 trC = CalculateScreenPosition(RotatePoint(worldPos, rotationRad, ImVec2(worldPos.x + ThrowRange + h_col_len / 2, worldPos.y + vcY_1)));
		ImVec2 trD = CalculateScreenPosition(RotatePoint(worldPos, rotationRad, ImVec2(worldPos.x - (ThrowRange + h_col_len / 2), worldPos.y + vcY_1)));
		fixAspectRatio(trA);
		fixAspectRatio(trB);
		fixAspectRatio(trC);
		fixAspectRatio(trD);
		RenderRect(trA, trB, trC, trD, colorYellow, m_rectThickness / 2);



		int a = 0;
	}
}
void HitboxOverlay::DrawCollisionAreas(const CharData* charObj, const ImVec2 playerWorldPos)
{
	std::vector<JonbEntry> entries = JonbReader::getJonbEntries(charObj);

	for (const JonbEntry& entry : entries)
	{
		//this will skip the drawing of an inactive hitbox due to multihit(ID 2002) script call.
		if (entry.type == JonbChunkType_Hitbox
			&&
			((charObj->bitflags_for_curr_state_properties_or_smth & 0xF00) == 0x400)
			 )
{
			continue;
		}

		float scaleX = charObj->scaleX / 1000.0f;
		float scaleY = charObj->scaleY / 1000.0f;
		float offsetX =  floor(entry.offsetX * m_scale * scaleX);
		float offsetY = -floor(entry.offsetY * m_scale * scaleY);
		float width =    floor(entry.width * m_scale * scaleX);
		float height =  -floor(entry.height * m_scale * scaleY);

		float rotationDeg = charObj->rotationDegrees / 1000.0f;

		if (!charObj->facingLeft)
		{
			offsetX = -offsetX;
			width = -width;
			if (rotationDeg)
			{
				rotationDeg = 360.0f - rotationDeg;
			}
		}

		ImVec2 pointA(playerWorldPos.x + offsetX, playerWorldPos.y + offsetY);
		ImVec2 pointB(playerWorldPos.x + offsetX + width, playerWorldPos.y + offsetY);
		ImVec2 pointC(playerWorldPos.x + offsetX + width, playerWorldPos.y + offsetY + height);
		ImVec2 pointD(playerWorldPos.x + offsetX, playerWorldPos.y + offsetY + height);

		float rotationRad = D3DXToRadian(rotationDeg);

		pointA = RotatePoint(playerWorldPos, rotationRad, pointA);
		pointB = RotatePoint(playerWorldPos, rotationRad, pointB);
		pointC = RotatePoint(playerWorldPos, rotationRad, pointC);
		pointD = RotatePoint(playerWorldPos, rotationRad, pointD);
		
		pointA = CalculateScreenPosition(pointA);
		pointB = CalculateScreenPosition(pointB);
		pointC = CalculateScreenPosition(pointC);
		pointD = CalculateScreenPosition(pointD);

		//Fixes aspect ratio when the game is in fullscreen/borderless
		fixAspectRatio(pointA);
		fixAspectRatio(pointB);
		fixAspectRatio(pointC);
		fixAspectRatio(pointD);

		const unsigned int colorBlue = 0xFF0033CC;
		const unsigned int colorRed = 0xFFFF0000;
		const unsigned int rectBorderColor = entry.type == JonbChunkType_Hurtbox ? colorBlue : colorRed;
		if (this->drawHitboxHurtbox) {
			RenderRect(pointA, pointB, pointC, pointD, rectBorderColor, m_rectThickness);
		}
		const unsigned char transparency = 0xFF * m_rectFillTransparency;
		unsigned int clearedTransparencyBits = (rectBorderColor & ~0xFF000000);
		unsigned int transparencyPercentage = ((int)transparency << 24) & 0xFF000000;
		const unsigned int rectFillColor = clearedTransparencyBits | transparencyPercentage;
		if (this->drawHitboxHurtbox) {
			RenderRectFilled(pointA, pointB, pointC, pointD, rectFillColor);
		}
		
		if (this->drawCollisionBoxes) {
			DrawCollisionBoxes(playerWorldPos, rotationRad, charObj);
		}
		
		if (this->drawRangeCheckBoxes) {
			DrawRangeCheckBoxes(playerWorldPos, rotationRad, charObj);

		}
		if (this->drawOriginLine)
		{
			DrawOriginLine(playerWorldPos, rotationRad);

		}
	
	}
}

float& HitboxOverlay::GetScale()
{
	return m_scale;
}

void HitboxOverlay::DrawRectThicknessSlider()
{
	ImGui::SliderFloat("Border thickness", &m_rectThickness, 0.0f, 5.0f, "%.1f");
}

void HitboxOverlay::DrawRectFillTransparencySlider()
{
	ImGui::SliderFloat("Fill transparency", &m_rectFillTransparency, 0.0f, 1.0f, "%.2f");
}

void HitboxOverlay::RenderLine(const ImVec2& from, const ImVec2& to, uint32_t color, float thickness)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	float a = (color >> 24) & 0xff;
	float r = (color >> 16) & 0xff;
	float g = (color >> 8) & 0xff;
	float b = (color) & 0xff;

	window->DrawList->AddLine(from, to, ImGui::GetColorU32({ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f }), thickness);
}

void HitboxOverlay::RenderCircle(const ImVec2& position, float radius, uint32_t color, float thickness, uint32_t segments)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	float a = (color >> 24) & 0xff;
	float r = (color >> 16) & 0xff;
	float g = (color >> 8) & 0xff;
	float b = (color) & 0xff;

	window->DrawList->AddCircle(position, radius, ImGui::GetColorU32({ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f }), segments, thickness);
}

void HitboxOverlay::RenderCircleFilled(const ImVec2& position, float radius, uint32_t color, uint32_t segments)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	float a = (color >> 24) & 0xff;
	float r = (color >> 16) & 0xff;
	float g = (color >> 8) & 0xff;
	float b = (color) & 0xff;

	window->DrawList->AddCircleFilled(position, radius, ImGui::GetColorU32({ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f }), segments);
}

void HitboxOverlay::RenderRect(const ImVec2& from, const ImVec2& to, uint32_t color, float rounding, uint32_t roundingCornersFlags, float thickness)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	float a = (color >> 24) & 0xFF;
	float r = (color >> 16) & 0xFF;
	float g = (color >> 8) & 0xFF;
	float b = (color) & 0xFF;

	window->DrawList->AddRect(from, to, ImGui::GetColorU32({ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f }), rounding, roundingCornersFlags, thickness);
}

void HitboxOverlay::RenderRect(const ImVec2& pointA, const ImVec2& pointB, const ImVec2& pointC, const ImVec2& pointD, uint32_t color, float thickness)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	float a = (color >> 24) & 0xFF;
	float r = (color >> 16) & 0xFF;
	float g = (color >> 8) & 0xFF;
	float b = (color) & 0xFF;

	window->DrawList->AddQuad(pointA, pointB, pointC, pointD, ImGui::GetColorU32({ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f }), thickness);
}

void HitboxOverlay::RenderRectFilled(const ImVec2& from, const ImVec2& to, uint32_t color, float rounding, uint32_t roundingCornersFlags)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	float a = (color >> 24) & 0xFF;
	float r = (color >> 16) & 0xFF;
	float g = (color >> 8) & 0xFF;
	float b = (color) & 0xFF;

	window->DrawList->AddRectFilled(from, to, ImGui::GetColorU32({ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f }), rounding, roundingCornersFlags);
}

void HitboxOverlay::RenderRectFilled(const ImVec2& pointA, const ImVec2& pointB, const ImVec2& pointC, const ImVec2& pointD, uint32_t color)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	float a = (color >> 24) & 0xFF;
	float r = (color >> 16) & 0xFF;
	float g = (color >> 8) & 0xFF;
	float b = (color) & 0xFF;

	window->DrawList->AddQuadFilled(pointA, pointB, pointC, pointD, ImGui::GetColorU32({ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f }));
}

bool HitboxOverlay::WorldToScreen(LPDIRECT3DDEVICE9 pDevice, D3DXMATRIX* view, D3DXMATRIX* proj, D3DXVECTOR3* pos, D3DXVECTOR3* out)
{
	D3DVIEWPORT9 viewPort;
	D3DXMATRIX world;

	pDevice->GetViewport(&viewPort);
	D3DXMatrixIdentity(&world);

	D3DXVec3Project(out, pos, &viewPort, proj, view, &world);
	if (out->z < 1) {
		return true;
	}
	return false;
}
