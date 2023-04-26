#pragma once
#include <cstdint>
// Generated using ReClass 2015

class EntityData
{
public:
	class OBJ_CCharBase* objCharbase; //0x0000
	int32_t frame_count_minus_1; //thanks to kding0
	int32_t hitstop; //thanks to kding0
	char pad_0008[4]; //0x0004
	int32_t unknownStatus1; //0x0010
	char pad_0014[4]; //0x0014
	int32_t stateChangedCount; //0x0018
	char pad_001C[24]; //0x001C
	int32_t charIndex; //0x0034
	char pad_0038[20]; //0x0038
	class JonbEntry* pJonbEntryBegin; //0x004C
	char pad_0050[68]; //0x0050
	uint32_t hurtboxCount; //0x0094
	uint32_t hitboxCount; //0x0098
	char pad_009C[60]; //0x009C
	char* current_sprite_img; //0x00D8
	char pad_00DC[120]; //0x00DC
	char unknown_status2; //0x0154  compared if 2 or not?
	char pad_0158[8]; ///0x0158

	
	
	//char pad_00DC[132]; 0x00DC[132]
	//char pad_015C[132]; //0x015C
	int32_t actionTime; //0x0160
	int32_t actionTime2; //0x0164
	int32_t actionTimeNoHitstop; //0x0170
	char pad_0174[56]; //0x0174

	int32_t lambda_nu_drive_hitcount; //0x01AC
	char pad_01B0[16]; //0x01B0
	int32_t overdriveTimeleft; //0x01C0
	int32_t overdriveTimerStartedAt; //0x01C4
	char pad_01C8[20]; //0x01C8
	int32_t moveSuperflashTime; //0x01D4
	char pad_01D8[4]; //0x01D8
	int32_t superflashTime; //0x01DC
	int32_t isDoingDistortion; //0x01E0


	char pad_01E4[4]; //0x01E4
	class CharData* ownerEntity; //0x01E8
	//char pad_01EC[120]; //0x01EC

	//exp
	char pad_01EC[4];
	class CharData* enemyChar; //0x01F0 dont know exactly what it is yet, just that it is checked and I need to save it 
	char pad_01F4[54]; //0x01F4

	//char pad_0258[12]; //0x0258
	class CharData* last_child_entity_spawned; // 0x022C not 100% yet
	class CharData* extra_child_entities[7]; // 0x0230 should hold up to 7(?) idk extra child entities
	class CharData* main_child_entity; // 0x024C holds the main child entity, varies by character but its the puppets, arakune curse circle over enemy, etc. Some characters dont have it
	char pad_0250[20]; // 0x0250
	
	//0x0254 significant?
	//exp 
	int32_t facingLeft; //0x0264 is it not facing right?
	int32_t position_x; //0x0268
	int32_t position_y; //0x026C
	char pad_0270[4]; //0x0270
	int32_t offsetX_1; //0x0274
	char pad_0278[4]; //0x0278
	int32_t rotationDegrees; //0x027C
	char pad_0280[4]; //0x0280
	int32_t scaleX; //0x0284
	int32_t scaleY; //0x0288
	char pad_028C[80]; //0x028C
	int32_t position_x_dupe; //0x02DC
	int32_t position_y_dupe; //0x02E0
	char pad_02E4[16]; //0x02E4
	int32_t offsetX_2; //0x02F4
	char pad_02F8[4]; //0x02F8
	int32_t offsetY_2; //0x02FC
	char pad_0300[28]; //0x0300

	//thanks to kding0
	int32_t BoundingX; //0x0316
	int32_t BoundingY; //0x0320
	int32_t BoundingFixX; //0x0324
	int32_t BoundingFixY; //0x0328
	int32_t BoundingAddY; //0x032C do not use not sure what it is, but it's Unknown23087?
	int32_t BoundingAddX; //0x0330
	int32_t stageEdgeTouchTimer; //0x0334
	char pad_0338[336]; //0x0338
	int32_t typeOfAttack; //0x0488 1=normal, 2=special, 3=DD/EA, 5=AH
	int32_t attackLevel; //0x048C
	int32_t moveDamage; //0x0490; raw damage
	char pad_0494[44]; //0x0494
	int32_t moveSpecialBlockstun; //0x04C0
	char pad_04C4[4]; //0x04C4
	int32_t moveGuardCrushTime; //0x04C8
	char pad_04CC[8]; //0x04CC
	int32_t vectorcheckX_1; //0x04D0
	int32_t vectorcheckY_1; //0x04D4
	int32_t vectorcheckX_2; //0x04D8
	int32_t vectorcheckY_2; //0x04DC
	int32_t ThrowRange; //0x04E4
	char pad_04E8[28]; //0x04E8
	char performedThrowName[32]; //0x0504
	char pad_0524[320]; //0x0524
	int32_t moveAirPushbackX; //0x0664
	int32_t moveAirPushbackY; //0x0668
	char pad_066C[20]; //0x066C
	int32_t moveHitstunOverwrite; //0x0680
	int32_t moveUntechOverwrite; //0x0684
	char pad_0688[12]; //0x0688
	int32_t movePushbackX; //0x0694
	char pad_0698[12]; //0x0698
	int32_t moveP1Overwrite; //0x06A4
	int32_t moveP2Overwrite; //0x06A8
	char pad_06AC[40]; //0x06AC
	int32_t moveCounterHitAirPushbackY; //0x06D4
	char pad_06D8[760]; //0x06D8


	int32_t previousHP; //0x09D0
	int32_t currentHP; //0x09D4
	int32_t maxHP; //0x09D8
	char pad_09DC[2372]; //0x09DC
	char* currentScriptActionLocationInMemory; //0x01320 points to the place in memory where the current script command is being read
	char* currentScriptStateEntryInMemory;///0x01324 points to the start of the current state in the script
	int32_t frameCounterCurrentSprite;//0x01328 Counts the frames the current sprite has been drawn on screen
	int32_t frameLengthCurrentSprite;//0x0132C the amount of frames the current sprite should last
	char pad_01330[4]; //0x01330
	int32_t framesInCurrentState; //0x01334
	int64_t currentSprite; // 0x01338 the name of the current sprite
	char pad_1340[3252]; //0x01340
	char lastAction[20]; //0x1FF4
	char pad_2008[104]; //0x2008
	char currentAction[20]; //0x2070
	char pad_2084[448]; //0x2084
	//char char_abbr[4]; //0x2248
}; //Size: 0x2248
