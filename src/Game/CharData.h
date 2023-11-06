#pragma once
#include <cstdint>
// Generated using ReClass 2015


class CharData
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
	char unknown_status2; //0x0154  compared if 2 or not? I think 2 might mean its an entity that is anchored to the character that spawned it 
	char pad_0158[8]; ///0x0158



	//char pad_00DC[132]; 0x00DC[132]
	///char pad_015C[132]; //0x015C
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
	//char pad_01EC[64];
	class CharData* last_child_entity_spawned; // 0x022C 
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
	int16_t moveSpecialBlockstun; //0x04C0
	char pad_04C2[6]; //0x04C2
	int16_t moveGuardCrushTime; //0x04C8
	char pad_04CA[10]; //0x04CA
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
	char* nextScriptLineLocationInMemory; //0x01320 points to the place in memory where the next line of the script to be read is
	char* currentScriptStateEntryInMemory;///0x01324 points to the start of the current state in the script
	int32_t frameCounterCurrentSprite;//0x01328 Counts the frames the current sprite has been drawn on screen
	int32_t frameLengthCurrentSprite;//0x0132C the amount of frames the current sprite should last
	int32_t frameLengthCurrentSprite2; //0x01330 the amount of frames the current sprite should last, this one seems to be the "correct" one
	int32_t linesReadInCurrentState; //0x01334 counts the amount of "lines" read from the start of the script, in looping actions take the modulo
	int64_t currentSprite; // 0x01338 the name of the current sprite
	char pad_1340[3252]; //0x01340
	char lastAction[20]; //0x1FF4
	char pad_2008[12]; //0x2008
	char current_action2[20];//0x2014 another current action I guess, gets set by the set_action_override
	char pad_2028[24]; //0x2028
	char set_action_override[20]; //0x2040 this sets the current action overriding the script order I guess? not sure, still need more testing
	char pad_2054[28]; //0x2054
	char currentAction[20]; //0x2070
	char pad_2084[452]; //0x2084
	char char_abbr[4]; //0x2248
	char pad_224C[40]; //0x224C

	//thanks to kding0
	int32_t blockstun; //0x2274
	char pad_2278[12024]; //0x2278
	int32_t hitstun; //0x5170
	char pad_5174[140]; //0x5174
	uint32_t hardLandingRecovery;
	char pad_5204[16]; //0x5204
	int32_t defaultProration[6]; //0x5214-0x5227, for Lv0-Lv5
	char pad_5228[1348]; //0x5228
	int32_t hitCount; //0x5770
	int32_t hitCount2; //0x5774
	int32_t timeAfterTechIsPerformed; //0x5778
	int32_t timeAfterLatestHit; //0x577C
	int32_t comboDamage; //0x5780
	int32_t comboDamage2; //0x5784
	int32_t lastcomboDamage; //0x5788
	int32_t comboProration; //0x578C
	int32_t starterRating; //0x5790
	int32_t comboTime; //0x5794
	int32_t singleHitDamage; //0x5798
	char pad_579C[4]; //0x579C
	int32_t realTimeComboTime; //0x57A0; THIS IS NOT COMBO TIME!
	int32_t heatGeneratedForCombo; //0x57A4; this does not take heatGainCooldown into account
	char pad_57A4[56]; //0x57A4
	char hitByWhichAction[32]; //0x57E0
	char pad_5800[28]; //0x5800
	char sameMoveProrationStack[32]; //0x581C; idk how long the stack is but it's pretty long lmao
	char pad_583C[664]; //0x583C









	int32_t heatMeter; //0x5AD4
	char pad_5AD8[4]; //0x5AD8
	int32_t heatGainCooldown; //0x5ADC
	char pad_5AE0[4]; //0x5AE0
	int32_t overdriveMeter; //0x5AE4
	char pad_5AE8[16]; //0x5AE8
	int32_t overdriveMaxtime; //0x5AF8
	char pad_5AFC[8]; //0x5AFC
	int32_t barrier; //0x5B04
	//char pad_5B08[102072]; //0x5b08
	char pad_5B08[101524]; //0x5b08





	/*input buffers*/
	char L_input_buffer_flag_base; //0x1e79c
	/*input buffer for Left side, size=0x10A this will skip all the non motion inputs for now*/
	char pad_1E79D[0xAB];//0x1e79d the "non-special" inputs should be here
	/*motion buffers Left side, the offsets listed are the offsets from input_buffer_flag_base*/
	int8_t buffer_L_236;//0x1e848, +ac
	int8_t buffer_L_623;//0x1e849, +ad
	int8_t buffer_L_214;//0x1e84A, +ae
	int8_t buffer_L_41236;//0x1e84B, +af
	int8_t buffer_L_421;//0x1e84C, + b0
	int8_t buffer_L_63214;//0x1e84D, + b1
	int8_t buffer_L_236236;//0x1e84E, + b2
	int8_t buffer_L_214214;//0x1e84F, + b3
	char padding_1E850;//0x1e850, + b4
	int8_t buffer_L_6321463214;//0x1e851, + b5 (Double HCB, jubei astral)
	int8_t buffer_L_632146_2;//0x1e852, + b6 (terumi 100 meter damage super)
	char padding_1E853;//0x1E853, + b7
	int8_t buffer_L_2141236;//0x1E854, + b8 susanoo astral
	int8_t buffer_L_2363214;//0x1E855, + b9 rachel, makoto astraleliussomething
	int8_t buffer_L_22;//0x1E856, + ba
	int8_t buffer_L_46; //0x1E857, + bb
	char padding_1e858;//0x1E858, + bc
	int8_t buffer_L_2H6; //0x1E859, + BD 2H6 -> hold 2 -> 6, charge(?) its actually 2H8 it seems for an unreleased kagura special?
	int8_t buffer_L_6428;//0x1E85A, +BE
	int8_t buffer_L_4H128;//0x1E85B, + BF taokaka astral
	int8_t buffer_L_64641236;//0x1E85C, + C0 carl astral
	int8_t buffer_L_412364123641236;//0x1E85D, + C1   Triple HCF (412364123641236) Litchi OD astral
	int8_t buffer_L_KS28_;//0x1E85E, + C2, kagura stance, maybe it includes all characters not just kagura as long as D is pressed?
	int8_t buffer_L_646;//0x1E85F, + C3
	char padding_1E860[2]; //0x1E860
	int8_t buffer_L_CMASH;//0x1E862, + C6 susanoo's c mash special
	char padding_1E863[3]; //0x1E863
	int8_t buffer_L_360; //0x1E867, + CA
	int8_t buffer_L_720; //0x1E868, + CB
	char padding_1E869; //0x1E869
	int8_t buffer_L_222;//0x1E86A, + CD
	int8_t padding_1E86B;//0x1E86B
	int8_t buffer_L_236236LambdaSuper;//0x1E86C, + CF specific to lambda-11 super it seems
	int8_t padding_1E86D[9];//0x1E86D
	int8_t buffer_L_8izanamifloat; //0x1E875, +D9 input for izanami float
	int8_t buffer_L_66; //0x1E876, +DA
	int8_t buffer_L_44; //0x1E877, +DB
	char padding_1E878[3];//0x1E878

	int8_t buffer_L_22jinsekkajin; //0x1E87B, +DF specific to jins sekkajin
	char padding_1E87B;//0x1E87C
	int8_t buffer_L_44nineseconddash; //0x1E87D, +E1 specific for nine's second dash
	int8_t buffer_L_66nineseconddash; //0x1E87E, +E2 specific for nine's second dash
	char padding_1E87F;//0x1E87F
	int8_t buffer_L_88nineseconddash; //0x1E880, +E4 specific for nine's second dash
	char padding_1E877[17];//0x1E881
	int8_t buffer_L_1080; //0x1E892, +F6
	int8_t buffer_L_1632143; //0x1E893, +F7
	char padding_1E895; //0x1E894
	int8_t buffer_L_632146; //0x1E895, +F9
	char padding_1E897[3]; //0x1E896
	int8_t buffer_L_takemikazuchi_shit; //0x1E899, +FD some specific stuff for the game's onslaught
	int8_t buffer_L_4H6; //0x1E89A, +FE
	int8_t buffer_L_2H8; //0x1E89B, +FF
	char padding_1E89D; //0x1E89C
	int8_t buffer_L_4H1236; //0x1E89D, +101
	int8_t buffer_L_82; //0x1E89E, +102
	int8_t buffer_L_8H2; //0x1E8AF, +103
	char padding_1E8A0[4]; //0x1E8A0
	int8_t buffer_L_236_5f; //0x1E8A4, +108 236 within 5 frames of input valid, for mai's enhanced thrust
	int8_t buffer_L_214_5f; //0x1E8A5, +109 214 for mai backflip idk why its special

	
	char R_input_buffer_flag_base; //0x1E8A6
	/*input buffer for Right side, size=0x10A this will skip all the non motion inputs for now*/
	char pad_1E8A7[0xAB];//0x1E8A7 the "non-special" inputs should be here, they should be duplicated maybe?

	/*motion buffers Right side, the offsets listed are the offsets from input_buffer_flag_base*/
	int8_t buffer_R_236;//0x1E952, +ac +10A
	int8_t buffer_R_623;//0x1E953, +ad +10A
	int8_t buffer_R_214;//0x1E954, +ae +10A
	int8_t buffer_R_41236;//0x1E955, +af +10A
	int8_t buffer_R_421;//0x1E956, + b0 +10A
	int8_t buffer_R_63214;//0x1E957, + b1 +10A
	int8_t buffer_R_236236;//0x1E958, + b2 +10A
	int8_t buffer_R_214214;//0x1E959, + b3 +10A
	char padding_1E95A;//0x1E95A, + b4 +10A
	int8_t buffer_R_6321463214;//0x1E95B, + b5 +10A(Double HCB, jubei astral)
	int8_t buffer_R_632146_2;//0x1E95C, + b6 +10A(terumi 100 meter damage super)
	char padding_1E95D;//0x1E95D, + b7 +10A
	int8_t buffer_R_2141236;//0x1E95E, + b8 +10A susanoo astral
	int8_t buffer_R_2363214;//0x1E95F, + b9 +10A rachel, makoto astraleliussomething
	int8_t buffer_R_22;//0x1E960, + ba +10A
	int8_t buffer_R_46; //0x1E961, + bb +10A
	char padding_1E962;//0x1E962, + bc +10A
	int8_t buffer_R_2H6; //0x1E963, + BD +10A 2H6 -> hold 2 -> 6, charge(?) its actually 2H8 it seems for an unreleased kagura special?
	int8_t buffer_R_6428;//0x1E964, +BE +10A
	int8_t buffer_R_4H128;//0x1E965, + BF +10A taokaka astral
	int8_t buffer_R_64641236;//0x1E966, + C0 +10A carl astral
	int8_t buffer_R_412364123641236;//0x1E967, + C1 +10A   Triple HCF (412364123641236) Litchi OD astral
	int8_t buffer_R_KS28_;//0x1E968, + C2 +10A, kagura stance, maybe it includes all characters not just kagura as long as D is pressed?
	int8_t buffer_R_646;//0x1E969, + C3 +10A
	char padding_1E96A[2]; //0x1E96A
	int8_t buffer_R_CMASH;//0x1E96C, + C6 +10A susanoo's c mash special
	char padding_1E96D[3]; //0x1E96D
	int8_t buffer_R_360; //0x1E970, + CA +10A
	int8_t buffer_R_720; //0x1E971, + CB +10A
	char padding_1E972; //0x1E972
	int8_t buffer_R_222;//0x1E973, + CD +10A
	char padding_1E974;//0x1E974
	int8_t buffer_R_236236LambdaSuper;//0x1E975, + CF +10A specific to lambda-11 super it seems
	char padding_1E976[9];//0x1E976
	int8_t buffer_R_8izanamifloat; //0x1E97f, +D9 +10A input for izanami float
	int8_t buffer_R_66; //0x1E980, +DA +10A
	int8_t buffer_R_44; //0x1E981, +DB +10A
	char padding_1E982[3];//0x1E982

	int8_t buffer_R_22jinsekkajin; //0x1E985, +DF +10A specific to jins sekkajin
	char padding_1E986;//0x1E986
	int8_t buffer_R_44nineseconddash; //0x1E987, +E1 +10A specific for nine's second dash
	int8_t buffer_R_66nineseconddash; //0x1E988, +E2 +10A specific for nine's second dash
	char padding_1E989;//0x1E989
	int8_t buffer_R_88nineseconddash; //0x1E98A, +E4 +10A specific for nine's second dash
	char padding_1E98B[17];//0x1E98B
	int8_t buffer_R_1080; //0x1E99C, +F6 +10A
	int8_t buffer_R_1632143; //0x1E99D, +F7 +10A
	char padding_1E99E; //0x1E99E
	int8_t buffer_R_632146; //0x1E99F, +F9 +10A
	char padding_1E9A0[3]; //0x1E9A0
	int8_t buffer_R_takemikazuchi_shit; //0x1E9A3, +FD +10A some specific stuff for the game's onslaught
	int8_t buffer_R_4H6; //0x1E9A4, +FE +10A
	int8_t buffer_R_2H8; //0x1E9A5, +FF +10A
	char padding_1E9A6; //0x1E9A6
	int8_t buffer_R_4H1236; //0x1E9A7, +101 +10A
	int8_t buffer_R_82; //0x1E9A8, +102 +10A
	int8_t buffer_R_8H2; //0x1E9A9, +103 +10A
	char padding_1E9AA[4]; //0x1E9AA
	int8_t buffer_R_236_5f; //0x1E9AE, +108 +10A 236 within 5 frames of input valid, for mai's enhanced thrust
	int8_t buffer_R_214_5f; //0x1E9AF, +109 +10A 214 for mai backflip idk why its special
	char pad_1E9B0[16];//0x1E9B0
	//char pad_1E9B0[5980];//0x1E9B0

	/*MOTION INPUT BUFFERS SHOULD END AT 0x1E9B0*/
	int32_t slot2_or_slot4; //0x1E9C0 referring to the scr SLOTs, naoto only for now until i figure out more about this shit !!! this is actually the buffer for dash



	char pad_1E9C4[5960]; //0x1E9C4 108036
	int32_t Drive1; //0x2010C
	char pad_20110[12]; //0x20110
	int32_t Drive1_type; //0x2011C
	char pad_20120[16]; //0x20120
	int32_t Drive2; //0x20130
	char pad_20134[32]; //0x20134
	int32_t Drive3; //0x20154
	char pad_20158[4972]; //0x20158
}; //Size: 0x214C4