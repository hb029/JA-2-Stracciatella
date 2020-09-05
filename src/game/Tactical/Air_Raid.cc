#include "Font_Control.h"
#include "Air_Raid.h"
#include "Game_Event_Hook.h"
#include "Game_Clock.h"
#include "Auto_Bandage.h"
#include "Map_Screen_Interface_Bottom.h"
#include "StrategicMap.h"
#include "PreBattle_Interface.h"
#include "ScreenIDs.h"
#include "JAScreens.h"
#include "Random.h"
#include "Overhead_Types.h"
#include "Sound_Control.h"
#include "Timer_Control.h"
#include "Dialogue_Control.h"
#include "Overhead.h"
#include "Message.h"
#include "Isometric_Utils.h"
#include "Soldier_Macros.h"
#include "LOS.h"
#include "math.h"
#include "Explosion_Control.h"
#include "Interface.h"
#include "Music_Control.h"
#include "ContentMusic.h"
#include "Campaign_Types.h"
#include "Text.h"
#include "Morale.h"
#include "Map_Screen_Helicopter.h"
#include "MapScreen.h"
#include "Structure_Wrap.h"
#include "SoundMan.h"
#include "Debug.h"
#include "FileMan.h"
#include "Environment.h"
#include "Town_Militia.h"
#include "Strategic_Town_Loyalty.h"
#include "WorldMan.h"
#include "OppList.h"
#include "Directories.h"
#include "Meanwhile.h"
#include "SamSiteModel.h"
#include "Creature_Spreading.h"

#include "CalibreModel.h"
#include "ContentManager.h"
#include "GameInstance.h"
#include "WeaponModels.h"

#include "Logger.h"

#define SCRIPT_DELAY				10
#define AIR_RAID_SAY_QUOTE_TIME		3000
#define AIR_RAID_DIVE_INTERVAL			10000
#define RAID_DELAY				40
#define TIME_FROM_DIVE_SOUND_TO_ATTACK_DELAY	8000
#define TIME_FROM_BOMB_SOUND_TO_ATTACK_DELAY	3000
#define MOVE_X					5
#define MOVE_Y					5
#define STRAFE_DIST				80
#define BOMB_DIST				150



// BEGIN SERALIZATION
extern INT32 giTimerAirRaidQuote;
extern INT32 giTimerAirRaidDiveStarted;
extern INT32 giTimerAirRaidUpdate;

BOOLEAN gfInAirRaid = FALSE;
BOOLEAN gfAirRaidScheduled = FALSE;
UINT8   gubAirRaidMode = AIR_RAID_INACTIVE;
UINT32  guiSoundSample = NO_SAMPLE;
UINT32  guiSoundVolume = 0;
UINT32  guiRaidLastUpdate;
BOOLEAN gfFadingRaidIn = FALSE;
BOOLEAN gfQuoteSaid = FALSE;
INT8    gbNumDives = 0;
INT8    gbMaxDives = 0;
BOOLEAN gfFadingRaidOut = FALSE;
INT16   gsDiveX;
INT16   gsDiveY;
INT16   gsDiveTargetLocation;
UINT8   gubDiveDirection;
INT16   gsNumGridNosMoved;
INT32   giNumTurnsSinceLastDive;
INT32   giNumTurnsSinceDiveStarted;
INT32   giNumGridNosMovedThisTurn;
BOOLEAN gfAirRaidHasHadTurn = FALSE;
UINT8   gubBeginTeamTurn = 0;
BOOLEAN gfHaveTBBatton = FALSE;
INT16   gsNotLocatedYet = FALSE;
GROUP	gChopperGroup;
static INT32 giNumFrames;

AIR_RAID_DEFINITION gAirRaidDef;


struct AIR_RAID_SAVE_STRUCT
{
	BOOLEAN fInAirRaid;
	BOOLEAN fAirRaidScheduled;
	UINT8   ubAirRaidMode;
	UINT32  uiSoundSample;
	UINT32  uiRaidLastUpdate;
	BOOLEAN fFadingRaidIn;
	BOOLEAN fQuoteSaid;
	INT8    bNumDives;
	INT8    bMaxDives;
	BOOLEAN fFadingRaidOut;
	INT16   sDiveX;
	INT16   sDiveY;
	INT16   sDiveTargetLocation;
	UINT8   ubDiveDirection;
	INT16   sNumGridNosMoved;
	INT32   iNumTurnsSinceLastDive;
	INT32   iNumTurnsSinceDiveStarted;
	INT32   iNumGridNosMovedThisTurn;
	BOOLEAN fAirRaidHasHadTurn;
	UINT8   ubBeginTeamTurn;
	BOOLEAN fHaveTBBatton;
	AIR_RAID_DEFINITION AirRaidDef;
	INT16   sRaidSoldierID;

	INT16   sNotLocatedYet;
	INT32   iNumFrames;

	INT8    bLevel;
	INT8    bTeam;
	INT8    bSide;
	UINT8   ubAttackerID;
	UINT16  usAttackingWeapon;
	FLOAT   dXPos;
	FLOAT   dYPos;
	INT16   sX;
	INT16   sY;
	INT16   sGridNo;

	UINT8   ubFiller[ 32 ]; // XXX HACK000B
};


// END SERIALIZATION
SOLDIERTYPE *gpRaidSoldier;


struct AIR_RAID_DIR
{
	INT8 bDir1;
	INT8 bDir2;
};

struct AIR_RAID_POS
{
	INT8 bX;
	INT8 bY;
};


AIR_RAID_DIR ubPerpDirections[ ] =
{
	{ 2, 6 },
	{ 3, 7 },
	{ 0, 4 },
	{ 1, 5 },
	{ 2, 6 },
	{ 3, 7 },
	{ 0, 4 },
	{ 1, 5 }
};

AIR_RAID_POS ubXYTragetInvFromDirection[ ] =
{
	{  0, -1 },
	{  1, -1 },
	{  1,  0 },
	{  1,  1 },
	{  0,  1 },
	{ -1,  1 },
	{ -1,  0 },
	{ -1, -1 }
};

extern UINT8 ubSAMControlledSectors[MAP_WORLD_X][MAP_WORLD_Y];


BOOLEAN WillAirRaidBeStopped(INT16 sSectorX, INT16 sSectorY);

void ScheduleAirRaid(AIR_RAID_DEFINITION* pAirRaidDef)
{
	// Make sure only one is cheduled...
	if ( gfAirRaidScheduled && gAirRaidDef.ubNumMinsFromCurrentTime > 0)
	{
		return;
	}

	if (!StrategicMap[(AIRPORT_X + (MAP_WORLD_X * AIRPORT_Y))].fEnemyControlled && !StrategicMap[(AIRPORT2_X + (MAP_WORLD_X * AIRPORT2_Y))].fEnemyControlled)
	{
		SLOGD("ScheduleAirRaid: enemy has no more airports");
		return;
	}

	if (pAirRaidDef->bIntensity < 2)
	{
		SLOGD("ScheduleAirRaid: intensity is less than 2");
		return;
	}

	// Copy definition structure into global struct....
	gAirRaidDef = *pAirRaidDef;

	AddSameDayStrategicEvent( EVENT_BEGIN_AIR_RAID, ( GetWorldMinutesInDay() + pAirRaidDef->ubNumMinsFromCurrentTime ), 0 );

	gfAirRaidScheduled = TRUE;
}

void ChopperAttackSector(UINT8 ubSectorX, UINT8 ubSectorY, INT8 bIntensity);
static void SetTeamStatusRed(INT8 team);

BOOLEAN BeginAirRaid( )
{
	// OK, we have been told to start.....

	// First remove scheduled flag...
	gfAirRaidScheduled = FALSE;
	gbNumDives = 0;

	if (WillAirRaidBeStopped(gAirRaidDef.sSectorX, gAirRaidDef.sSectorY))
	{
		return(FALSE);
	}
	else
	{
		gubAirRaidMode = AIR_RAID_PENDING;
	}

	// Set flag for handling raid....
	gTacticalStatus.fEnemyInSector = TRUE;

	// CHECK IF WE CURRENTLY HAVE THIS SECTOR OPEN....
	if (!PlayerMercsInSector(gAirRaidDef.sSectorX, gAirRaidDef.sSectorY, 0))
	{
		ChangeSelectedMapSector(gAirRaidDef.sSectorX, gAirRaidDef.sSectorY, (INT8)gAirRaidDef.sSectorZ);
		
		if (!AreInMeanwhile())
		{
			SetGameTimeCompressionLevel(TIME_COMPRESS_X0);
			DoScreenIndependantMessageBox(TacticalStr[AIR_RAID_TURN_STR], MSG_BOX_FLAG_OK, MapScreenDefaultOkBoxCallback); // HACK0000
		}

		EndAirRaid();
		
		return TRUE;
	}
	else
	{
		if (gAirRaidDef.sSectorX != gWorldSectorX || gAirRaidDef.sSectorY != gWorldSectorY || gbWorldSectorZ || guiCurrentScreen != GAME_SCREEN)
		{
			gChopperGroup.ubGroupSize = 1;
			gChopperGroup.ubSectorX = gAirRaidDef.sSectorX;
			gChopperGroup.ubSectorY = gAirRaidDef.sSectorY;
			gChopperGroup.ubSectorZ = 0;
			gChopperGroup.fPlayer = FALSE;

			gfCantRetreatInPBI = TRUE;
			gfEnteringMapScreenToEnterPreBattleInterface = TRUE;
			gubEnemyEncounterCode = ENEMY_AIR_RAID_CODE;

			InitPreBattleInterface(&gChopperGroup, TRUE);
			InterruptTime();
			PauseGame();
			LockPauseState(LOCK_PAUSE_PREBATTLE);
		}
	}

	// ( unless we are in prebattle interface, then ignore... )
	if ( gfPreBattleInterfaceActive )
	{
		return( FALSE );
	}
	
	gfInAirRaid = TRUE;
	gubAirRaidMode = AIR_RAID_TRYING_TO_START;
	SetTeamStatusRed(MILITIA_TEAM);
	SetTeamStatusRed(CIV_TEAM);
	
	// Set orders
	FOR_EACH_IN_TEAM(s, MILITIA_TEAM)
	{
		if (s->bInSector) s->bOrders = SEEKENEMY;
	}

	if (gAirRaidDef.sSectorX != gWorldSectorX ||
		gAirRaidDef.sSectorY != gWorldSectorY ||
		gAirRaidDef.sSectorZ != gbWorldSectorZ ||
		guiCurrentScreen == MAP_SCREEN )
	{
		// sector not loaded
		// Set flag for handling raid....
		gfQuoteSaid = TRUE;
		SayQuoteFromAnyBodyInThisSector(gAirRaidDef.sSectorX, gAirRaidDef.sSectorY,
							(INT8)gAirRaidDef.sSectorZ, QUOTE_AIR_RAID);

		class DialogueEventExitMapScreen : public DialogueEvent
		{
			public:
				DialogueEventExitMapScreen(INT16 const x, INT16 const y, INT16 const z) :
					x_(x),
					y_(y),
					z_(z)
				{}

				bool Execute()
				{
					ChangeSelectedMapSector(x_, y_, z_);
					RequestTriggerExitFromMapscreen(MAP_EXIT_TO_TACTICAL);
					return false;
				}

			private:
				INT16 const x_;
				INT16 const y_;
				INT16 const z_;
		};

		DialogueEvent::Add(new DialogueEventExitMapScreen(gAirRaidDef.sSectorX, gAirRaidDef.sSectorY, gAirRaidDef.sSectorZ));
	}
	else
	{
		gfQuoteSaid	= FALSE;
	}

	giNumFrames = 0;
	guiSoundVolume = 0;

	guiRaidLastUpdate = GetJA2Clock( );

	gfAirRaidHasHadTurn = FALSE;

	SOLDIERTYPE& s = GetMan(MAX_NUM_SOLDIERS - 1);
	s = SOLDIERTYPE{};
	s.bLevel				= 0;
	s.bTeam					= 1;
	s.bSide					= 1;
	s.ubID					= MAX_NUM_SOLDIERS - 1;
	s.attacker				= 0;
	s.usAttackingWeapon		= __ITEM_33;
	s.ubAttackingHand		= HANDPOS;
	s.inv[HANDPOS].usItem	= __ITEM_33;
	s.bLevel				= 1;
	gpRaidSoldier = &s;

	// Determine how many dives this one will be....
	gbMaxDives = gAirRaidDef.bIntensity;

	SLOGD("Begin Air Raid." );

	return(TRUE);
}


static INT16 PickLocationNearAnyMercInSector(void)
{
	// Loop through all our guys and randomly say one from someone in our sector
	INT32 num_mercs = 0;
	const SOLDIERTYPE* mercs_in_sector[40];
	CFOR_EACH_IN_TEAM(s, OUR_TEAM)
	{
		// Add guy if he's a candidate...
		if (OkControllableMerc(s)) mercs_in_sector[num_mercs++] = s;
	}
	CFOR_EACH_IN_TEAM(s, MILITIA_TEAM)
	{
		// Add guy if he's a candidate...
		if (s->bLife > 0) mercs_in_sector[num_mercs++] = s;
	}

	return num_mercs > 0 ? mercs_in_sector[Random(num_mercs)]->sGridNo : NOWHERE;
}


static INT16 PickRandomLocationAtMinSpacesAway(INT16 sGridNo, INT16 sMinValue, INT16 sRandomVar)
{
	INT16 sNewGridNo = NOWHERE;

	INT16 sX, sY, sNewX, sNewY;

	sX = CenterX( sGridNo );
	sY = CenterY( sGridNo );

	while( sNewGridNo == NOWHERE )
	{
		// First define a shift
		sNewX = sMinValue + (INT16)Random( sRandomVar );
		sNewY = sMinValue + (INT16)Random( sRandomVar );

		// Invert it randomly
		if ( Random( 2 ) )
		{
			sNewX = -1 * sNewX;
		}

		if ( Random( 2 ) )
		{
			sNewY = -1 * sNewY;
		}

		// Add the offset
		sNewX = sX + sNewX;
		sNewY = sY + sNewY;

		// Make gridno....
		sNewGridNo = GETWORLDINDEXFROMWORLDCOORDS( sNewY, sNewX );

		// Check if visible on screen....
		if ( !GridNoOnVisibleWorldTile( sNewGridNo ) )
		{
			sNewGridNo = NOWHERE;
		}
	}

	return( sNewGridNo );
}


static void TryToStartRaid(void)
{
	// OK, check conditions,

	// Some are:

	// Cannot be in battle ( this is handled by the fact of it begin shceduled in the first place...

	// Cannot be auto-bandaging?
	if ( gTacticalStatus.fAutoBandageMode )
	{
		return;
	}

	// Cannot be in conversation...
	if ( gTacticalStatus.uiFlags & ENGAGED_IN_CONV )
	{
		return;
	}

	// Cannot be traversing.....
	if (gfTacticalTraversal)
	{
		return;
	}

	// Ok, go...
	gubAirRaidMode = AIR_RAID_START;

}


static void AirRaidStart(void)
{
	// Begin ambient sound....
	gfFadingRaidIn = TRUE;
	guiSoundVolume = 0;
	if (guiSoundSample != NO_SAMPLE)
	{
		SoundStop(guiSoundSample);
		guiSoundSample = NO_SAMPLE;
	}

	// Setup start time....
	RESETTIMECOUNTER( giTimerAirRaidQuote, AIR_RAID_SAY_QUOTE_TIME );

	gubAirRaidMode = AIR_RAID_LOOK_FOR_DIVE;

	// If we are not in combat, change music mode...
	if ( !( gTacticalStatus.uiFlags & INCOMBAT ) )
	{
		gfUseCreatureMusic = false;
		SetMusicMode( MUSIC_TACTICAL_BATTLE );
	}
}


static void AirRaidLookForDive(void)
{
	BOOLEAN	fDoDive = FALSE;
	BOOLEAN	fDoQuote = FALSE;

	if ( !( gTacticalStatus.uiFlags & INCOMBAT ) )
	{
		if ( !gfQuoteSaid )
		{
			if ( TIMECOUNTERDONE( giTimerAirRaidQuote, AIR_RAID_SAY_QUOTE_TIME ) )
			{
				fDoQuote = TRUE;
			}
		}
	}
	else
	{
		if ( giNumTurnsSinceLastDive > 1 && !gfQuoteSaid )
		{
			fDoQuote = TRUE;
		}
	}

	// OK, check if we should say something....
	if ( fDoQuote )
	{
		gfQuoteSaid = TRUE;

		// Someone in group say quote...
		SayQuoteFromAnyBodyInSector( QUOTE_AIR_RAID );

		// Update timer
		RESETTIMECOUNTER( giTimerAirRaidDiveStarted, AIR_RAID_DIVE_INTERVAL );

		giNumTurnsSinceLastDive = 0;

		// Do morale hit on our guys
		HandleMoraleEvent( NULL, MORALE_AIRSTRIKE, gAirRaidDef.sSectorX, gAirRaidDef.sSectorY, ( INT8 ) gAirRaidDef.sSectorZ );
	}


	// If NOT in combat....
	if ( !( gTacticalStatus.uiFlags & INCOMBAT ) )
	{
		// OK, for now on, all we try to do is look for dives to make...
		if ( gfQuoteSaid )
		{
			if ( TIMECOUNTERDONE( giTimerAirRaidDiveStarted, AIR_RAID_DIVE_INTERVAL ) )
			{
				// IN realtime, give a bit more leeway for time....
				if ( Random( 2 ) )
				{
					fDoDive = TRUE;
				}
			}
		}
	}
	else
	{
		// How many turns have gone by?
		if ( (UINT32)giNumTurnsSinceLastDive > ( Random( 2 ) + 1 ) )
		{
			fDoDive = TRUE;
		}
	}

	if ( fDoDive )
	{
		// If we are are beginning game, only to gun dives..
		if ( gAirRaidDef.uiFlags & AIR_RAID_BEGINNING_GAME )
		{
			if ( gbNumDives == 0 )
			{
				gubAirRaidMode = AIR_RAID_BEGIN_DIVE;
			}
			else if ( gbNumDives == 1 )
			{
				gubAirRaidMode = AIR_RAID_BEGIN_BOMBING;
			}
			else
			{
				gubAirRaidMode = AIR_RAID_BEGIN_DIVE;
			}
		}
		else
		{
			// Randomly do dive...
			if ( Random( 2 ) )
			{
				gubAirRaidMode = AIR_RAID_BEGIN_DIVE;
			}
			else
			{
				gubAirRaidMode = AIR_RAID_BEGIN_BOMBING;
			}
		}
		gbNumDives++;
		return;
	}
	else
	{
		if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
		{
			if ( giNumGridNosMovedThisTurn == 0 )
			{
				// Free up attacker...
				FreeUpAttacker(gpRaidSoldier);
				SLOGD("Tried to free up attacker AIR RAID NO DIVE, attack count now %d",
					gTacticalStatus.ubAttackBusyCount);
			}
		}
	}

	// End if we have made desired # of dives...
	if ( gbNumDives >= 4*gbMaxDives || NumEnemyInSector() > 0)
	{
		// Air raid is over....
		gubAirRaidMode = AIR_RAID_START_END;
		guiSoundVolume = 0;
	}
}


static void AirRaidStartEnding(void)
{
	// Fade out sound.....
	gfFadingRaidOut = TRUE;
}


static void BeginBombing(void)
{
	INT16  sGridNo;
	UINT32 iSoundStartDelay;

	if ( !( gTacticalStatus.uiFlags & INCOMBAT ) )
	{
		// Start diving sound...
		PlayJA2Sample(S_RAID_WHISTLE, HIGHVOLUME, 1, MIDDLEPAN);
	}

	gubAirRaidMode = AIR_RAID_BOMBING;

	// Pick location...
	gsDiveTargetLocation = PickLocationNearAnyMercInSector( );

	if ( gsDiveTargetLocation == NOWHERE )
	{
		gsDiveTargetLocation = 10234;
	}

	// Get location of aircraft....
	sGridNo = PickRandomLocationAtMinSpacesAway( gsDiveTargetLocation , 300, 200 );

	// Save X, y:
	gsDiveX = CenterX( sGridNo );
	gsDiveY = CenterY( sGridNo );

	RESETTIMECOUNTER( giTimerAirRaidUpdate, RAID_DELAY );

	if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
	{
		iSoundStartDelay = 0;
	}
	else
	{
		iSoundStartDelay = TIME_FROM_BOMB_SOUND_TO_ATTACK_DELAY;
	}
	RESETTIMECOUNTER( giTimerAirRaidDiveStarted, iSoundStartDelay );


	giNumTurnsSinceDiveStarted = 0;

	// Get direction....
	gubDiveDirection = (INT8)GetDirectionToGridNoFromGridNo( sGridNo, gsDiveTargetLocation );

	gsNumGridNosMoved = 0;
	gsNotLocatedYet = TRUE;

}


static void BeginDive()
{
	INT16  sGridNo;
	UINT32 iSoundStartDelay;

	// Start diving sound...
	PlayJA2Sample(S_RAID_DIVE, HIGHVOLUME, 1, MIDDLEPAN);

	gubAirRaidMode = AIR_RAID_DIVING;

	// Increment attacker bust count....
	gTacticalStatus.ubAttackBusyCount++;
	SLOGD("Starting attack BEGIN DIVE %d", gTacticalStatus.ubAttackBusyCount);

	// Pick location...
	gsDiveTargetLocation = PickLocationNearAnyMercInSector( );

	if ( gsDiveTargetLocation == NOWHERE )
	{
		gsDiveTargetLocation = 10234;
	}

	// Get location of aircraft....
	sGridNo = PickRandomLocationAtMinSpacesAway( gsDiveTargetLocation, 300, 200 );

	// Save X, y:
	gsDiveX = CenterX( sGridNo );
	gsDiveY = CenterY( sGridNo );

	RESETTIMECOUNTER( giTimerAirRaidUpdate, RAID_DELAY );
	giNumTurnsSinceDiveStarted = 0;

	if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
	{
		iSoundStartDelay = 0;
	}
	else
	{
		iSoundStartDelay = TIME_FROM_DIVE_SOUND_TO_ATTACK_DELAY;
	}
	RESETTIMECOUNTER( giTimerAirRaidDiveStarted, iSoundStartDelay );


	// Get direction....
	gubDiveDirection = (INT8)GetDirectionToGridNoFromGridNo( sGridNo, gsDiveTargetLocation );

	gsNumGridNosMoved = 0;
	gsNotLocatedYet = TRUE;

}


static void MoveDiveAirplane(FLOAT dAngle)
{
	FLOAT dDeltaPos;

	// Find delta Movement for X pos
	dDeltaPos = MOVE_X * (FLOAT)sin( dAngle );

	// Find new position
	gsDiveX = (INT16)( gsDiveX + dDeltaPos );

	// Find delta Movement for Y pos
	dDeltaPos = MOVE_X * (FLOAT)cos( dAngle );

	// Find new pos
	gsDiveY = (INT16)( gsDiveY + dDeltaPos );
}


static void DoDive(void)
{
	INT16 sRange;
	INT16 sGridNo, sOldGridNo;

	INT16 sTargetX, sTargetY;
	INT16 sStrafeX, sStrafeY;
	FLOAT dDeltaX, dDeltaY, dAngle, dDeltaXPos, dDeltaYPos;
	INT16 sX, sY;


	// Delay for a specific perion of time to allow sound to Q up...
	if ( TIMECOUNTERDONE( giTimerAirRaidDiveStarted, 0 ) )
	{
		// OK, rancomly decide to not do this dive...
		if ( gAirRaidDef.uiFlags & AIR_RAID_CAN_RANDOMIZE_TEASE_DIVES )
		{
			if ( Random( 10 ) == 0 )
			{
				// Finish....
				gubAirRaidMode = AIR_RAID_END_DIVE;
				return;
			}
		}

		if ( gsNotLocatedYet && gTacticalStatus.uiFlags & INCOMBAT )
		{
			gsNotLocatedYet = FALSE;
			LocateGridNo( gsDiveTargetLocation );
		}

		sOldGridNo = GETWORLDINDEXFROMWORLDCOORDS( gsDiveY, gsDiveX );

		// Dive until we are a certain range to target....
		sRange = PythSpacesAway( sOldGridNo, gsDiveTargetLocation );

		// If sRange
		if ( sRange < 3 )
		{
			// Finish....
			gubAirRaidMode = AIR_RAID_END_DIVE;
			return;
		}

		if ( TIMECOUNTERDONE( giTimerAirRaidUpdate, RAID_DELAY ) )
		{
			RESETTIMECOUNTER( giTimerAirRaidUpdate, RAID_DELAY );

			// Move Towards target....
			sTargetX = CenterX( gsDiveTargetLocation );
			sTargetY = CenterY( gsDiveTargetLocation );

			// Determine deltas
			dDeltaX = (FLOAT)( sTargetX - gsDiveX );
			dDeltaY = (FLOAT)( sTargetY - gsDiveY );

			// Determine angle
			dAngle = (FLOAT)atan2( dDeltaX, dDeltaY );

			MoveDiveAirplane( dAngle );

			gpRaidSoldier->dXPos = gsDiveX;
			gpRaidSoldier->sX = gsDiveX;
			gpRaidSoldier->dYPos = gsDiveY;
			gpRaidSoldier->sY = gsDiveY;

			// Figure gridno....
			sGridNo = GETWORLDINDEXFROMWORLDCOORDS( gsDiveY, gsDiveX );
			gpRaidSoldier->sGridNo = sGridNo;

			if (giNumGridNosMovedThisTurn % 2 == 0)
			{
				char zBurstString[50];
				// Pick sound file baed on how many bullets we are going to fire...
				sprintf(zBurstString, SOUNDSDIR "/weapons/%s%d.wav",
					GCM->getWeapon(__ITEM_33)->calibre->burstSoundString.c_str(),
					2);

				PlayJA2Sample(zBurstString, GCM->getWeapon(__ITEM_33)->ubAttackVolume / 2, 1, MIDDLEPAN);
			}

			if ( sOldGridNo != sGridNo )
			{
				gsNumGridNosMoved++;

				giNumGridNosMovedThisTurn++;

				// OK, shoot bullets....
				// Get positions of guns...

				// Get target.....
				dDeltaXPos = STRAFE_DIST * (FLOAT)sin( dAngle );
				sStrafeX = (INT16)( gsDiveX + dDeltaXPos );

				// Find delta Movement for Y pos
				dDeltaYPos = STRAFE_DIST * (FLOAT)cos( dAngle );
				sStrafeY = (INT16)( gsDiveY + dDeltaYPos );

				if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
				{
					LocateGridNo( sGridNo );
				}

				if (GridNoOnVisibleWorldTile((INT16)(GETWORLDINDEXFROMWORLDCOORDS(sStrafeY, sStrafeX))))
				{
					//if ( gsNotLocatedYet && !( gTacticalStatus.uiFlags & INCOMBAT ) )
					//{
					//	gsNotLocatedYet = FALSE;
					//	LocateGridNo( sGridNo );
					//}

					//if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
					{
						// Increase attacker busy...
						//gTacticalStatus.ubAttackBusyCount++;
						//SLOGD("Starting attack AIR RAID ( fire gun ), attack count now %d",
						//	gTacticalStatus.ubAttackBusyCount);

						// INcrement bullet fired...
						gpRaidSoldier->bBulletsLeft++;
					}

					// For now use first position....
					
					gpRaidSoldier->target = WhoIsThere2(GETWORLDINDEXFROMWORLDCOORDS(sStrafeY, sStrafeX), 1);
					if (gpRaidSoldier->target == NULL)
					{
						gpRaidSoldier->target = WhoIsThere2(GETWORLDINDEXFROMWORLDCOORDS(sStrafeY, sStrafeX), 0);
					}
					if (gpRaidSoldier->target)
					{
						gpRaidSoldier->bAimShotLocation = AIM_SHOT_TORSO;
					}

					FireBulletGivenTarget( gpRaidSoldier, sStrafeX, sStrafeY, 0, gpRaidSoldier->usAttackingWeapon, 10, FALSE, FALSE );
				}

				// Do second one.... ( ll )
				sX = (INT16)( gsDiveX + ( (FLOAT)sin( dAngle + ( PI/2) ) * 40 ) );
				sY = (INT16)( gsDiveY + ( (FLOAT)cos( dAngle + ( PI/2) ) * 40 ) );

				gpRaidSoldier->dXPos = sX;
				gpRaidSoldier->sX = sX;
				gpRaidSoldier->dYPos = sY;
				gpRaidSoldier->sY = sY;
				gpRaidSoldier->sGridNo = GETWORLDINDEXFROMWORLDCOORDS( sY, sX );

				// Get target.....
				sStrafeX = (INT16)( sX + dDeltaXPos );

				// Find delta Movement for Y pos
				sStrafeY = (INT16)( sY + dDeltaYPos );

				if (GridNoOnVisibleWorldTile((INT16)(GETWORLDINDEXFROMWORLDCOORDS(sStrafeY, sStrafeX))))
				{
					//if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
					{
						// Increase attacker busy...
						//gTacticalStatus.ubAttackBusyCount++;
						//SLOGD("Starting attack AIR RAID ( second one ), attack count now %d",
						//	gTacticalStatus.ubAttackBusyCount);

						// INcrement bullet fired...
						gpRaidSoldier->bBulletsLeft++;

					}

					// Stick to targets at crosshair, rooftop first
					gpRaidSoldier->target = WhoIsThere2(GETWORLDINDEXFROMWORLDCOORDS(sStrafeY, sStrafeX), 1);
					if (gpRaidSoldier->target == NULL)
					{
						gpRaidSoldier->target = WhoIsThere2(GETWORLDINDEXFROMWORLDCOORDS(sStrafeY, sStrafeX), 0);
					}
					if (gpRaidSoldier->target)
					{
						gpRaidSoldier->bAimShotLocation = AIM_SHOT_TORSO;
					}

					FireBulletGivenTarget(gpRaidSoldier, sStrafeX, sStrafeY, 0, gpRaidSoldier->usAttackingWeapon, 10, FALSE, FALSE);
				}

			}

			if ( giNumGridNosMovedThisTurn >= 6 )
			{
				if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
				{
					// Free up attacker...
					FreeUpAttacker(gpRaidSoldier);
					SLOGD("Tried to free up attacker AIR RAID DIVE DONE FOR THIS TURN, attack count now %d",
						gTacticalStatus.ubAttackBusyCount);
				}
			}
		}
	}
}


static void DoBombing(void)
{
	INT16   sRange;
	INT16   sGridNo, sOldGridNo, sBombGridNo;

	INT16   sTargetX, sTargetY;
	UINT16  usItem;
	INT16   sStrafeX, sStrafeY;
	FLOAT   dDeltaX, dDeltaY, dAngle, dDeltaXPos, dDeltaYPos;
	BOOLEAN fLocate = FALSE;

	// Delay for a specific perion of time to allow sound to Q up...
	if ( TIMECOUNTERDONE( giTimerAirRaidDiveStarted, 0 ) )
	{
		// OK, rancomly decide to not do this dive...
		if ( gAirRaidDef.uiFlags & AIR_RAID_CAN_RANDOMIZE_TEASE_DIVES )
		{
			if ( Random( 10 ) == 0 )
			{
				// Finish....
				gubAirRaidMode = AIR_RAID_END_BOMBING;
				return;
			}
		}

		if ( gsNotLocatedYet && gTacticalStatus.uiFlags & INCOMBAT )
		{
			gsNotLocatedYet = FALSE;
			LocateGridNo( gsDiveTargetLocation );
		}

		sOldGridNo = GETWORLDINDEXFROMWORLDCOORDS( gsDiveY, gsDiveX );

		// Dive until we are a certain range to target....
		sRange = PythSpacesAway( sOldGridNo, gsDiveTargetLocation );

		// If sRange
		if ( sRange < 3 )
		{
			// Finish....
			gubAirRaidMode = AIR_RAID_END_BOMBING;
			return;
		}

		if ( TIMECOUNTERDONE( giTimerAirRaidUpdate, RAID_DELAY ) )
		{
			RESETTIMECOUNTER( giTimerAirRaidUpdate, RAID_DELAY );

			// Move Towards target....
			sTargetX = CenterX( gsDiveTargetLocation );
			sTargetY = CenterY( gsDiveTargetLocation );

			// Determine deltas
			dDeltaX = (FLOAT)( sTargetX - gsDiveX );
			dDeltaY = (FLOAT)( sTargetY - gsDiveY );

			// Determine angle
			dAngle = (FLOAT)atan2( dDeltaX, dDeltaY );

			MoveDiveAirplane( dAngle );

			gpRaidSoldier->dXPos = gsDiveX;
			gpRaidSoldier->sX = gsDiveX;
			gpRaidSoldier->dYPos = gsDiveY;
			gpRaidSoldier->sY = gsDiveY;

			// Figure gridno....
			sGridNo = GETWORLDINDEXFROMWORLDCOORDS( gsDiveY, gsDiveX );
			gpRaidSoldier->sGridNo = sGridNo;

			if ( sOldGridNo != sGridNo )
			{
				// Every once and a while, drop bomb....
				gsNumGridNosMoved++;

				giNumGridNosMovedThisTurn++;

				if ( ( gsNumGridNosMoved % 4 ) == 0 )
				{
					// Get target.....
					dDeltaXPos = BOMB_DIST * (FLOAT)sin( dAngle );
					sStrafeX = (INT16)( gsDiveX + dDeltaXPos );

					// Find delta Movement for Y pos
					dDeltaYPos = BOMB_DIST * (FLOAT)cos( dAngle );
					sStrafeY = (INT16)( gsDiveY + dDeltaYPos );

					if ( GridNoOnVisibleWorldTile( (INT16)( GETWORLDINDEXFROMWORLDCOORDS( sStrafeY, sStrafeX ) ) ) )
					{
						//if ( gsNotLocatedYet && !( gTacticalStatus.uiFlags & INCOMBAT ) )
						//{
						//	gsNotLocatedYet = FALSE;
						//	LocateGridNo( sGridNo );
						//}


						if ( Random( 2 ) )
						{
							usItem = HAND_GRENADE;
						}
						else
						{
							usItem = RDX;
						}

						// Pick random gridno....
						sBombGridNo = PickRandomLocationAtMinSpacesAway( (INT16)( GETWORLDINDEXFROMWORLDCOORDS( sStrafeY, sStrafeX ) ) , 40, 40 );

						if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
						{
							fLocate = TRUE;
							// Increase attacker busy...
							gTacticalStatus.ubAttackBusyCount++;
							SLOGD("Starting attack AIR RAID (bombs away), attack count now %d",
								gTacticalStatus.ubAttackBusyCount);
						}

						// Drop bombs...
						InternalIgniteExplosion(NULL, CenterX(sBombGridNo), CenterY(sBombGridNo), 0, sBombGridNo, usItem, fLocate, IsRoofPresentAtGridno(sBombGridNo));
					}

				}

				if ( giNumGridNosMovedThisTurn >= 6 )
				{
					if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
					{
						// Free up attacker...
						FreeUpAttacker(gpRaidSoldier);
						SLOGD("Tried to free up attacker AIR RAID BOMB ATTACK DONE FOR THIS TURN, attack count now %d",
							gTacticalStatus.ubAttackBusyCount);
					}
				}
			}
		}
	}
}


void HandleAirRaid( )
{
	UINT32 uiClock;

	// OK,
	if ( gfInAirRaid )
	{
		if (gubAirRaidMode > AIR_RAID_TRYING_TO_START &&
			gubAirRaidMode < AIR_RAID_END)
		{
			// Air raid is audible
			INT32 iVol;

			if (gfFadingRaidIn)
			{
				iVol = guiSoundVolume;
				if (iVol >= HIGHVOLUME)
				{
					giNumGridNosMovedThisTurn = 0;
					gfFadingRaidIn = FALSE;
				}
			}
			else if (gfFadingRaidOut)
			{
				iVol = HIGHVOLUME - guiSoundVolume;
				if (iVol <= 0)
				{
					gfFadingRaidOut = FALSE;
					gubAirRaidMode = AIR_RAID_END;
				}
			}
			else
			{
				iVol = HIGHVOLUME;
			}

			iVol = __min(HIGHVOLUME, iVol + 1);
			iVol = __max(0, iVol - 1);

			if (guiSoundSample == NO_SAMPLE)
			{
				guiSoundSample = PlayJA2Sample(S_RAID_AMBIENT, iVol, 10000, MIDDLEPAN);
			}
			else
			{
				SoundSetVolume(guiSoundSample, iVol);
			}
		}

		// Are we in TB?
		if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
		{
			// Do we have the batton?
			if ( !gfHaveTBBatton )
			{
				// Don't do anything else!
				return;
			}
		}

		if (WillAirRaidBeStopped(gAirRaidDef.sSectorX, gAirRaidDef.sSectorY))
		{
			SLOGD("HandleAirRaid: SAM just taken over");
			if (gfFadingRaidIn)
			{
				guiSoundVolume = HIGHVOLUME - guiSoundVolume;
			}
			gfFadingRaidIn = FALSE;
			gfFadingRaidOut = TRUE;
		}

		uiClock = GetJA2Clock( );

		if ( ( uiClock - guiRaidLastUpdate ) >  SCRIPT_DELAY )
		{
			giNumFrames++;

			guiRaidLastUpdate = uiClock;

			if ((gfFadingRaidIn || gfFadingRaidOut) && (giNumFrames % 10) == 0)
			{
				guiSoundVolume++;
			}

			switch( gubAirRaidMode )
			{
				case AIR_RAID_TRYING_TO_START:

					TryToStartRaid( );
					break;

				case AIR_RAID_START:

					AirRaidStart( );
					break;

				case AIR_RAID_LOOK_FOR_DIVE:

					AirRaidLookForDive( );
					break;

				case AIR_RAID_START_END:

					AirRaidStartEnding( );
					break;

				case AIR_RAID_END:

					EndAirRaid( );
					break;

				case AIR_RAID_BEGIN_DIVE:

					BeginDive( );
					break;

				case AIR_RAID_DIVING:
					// If in combat, check if we have reached our max...
					if (!(gTacticalStatus.uiFlags & INCOMBAT) ||
						giNumGridNosMovedThisTurn < 6)
					{
						DoDive();
					}
					break;

				case AIR_RAID_END_DIVE:

					giNumTurnsSinceLastDive = 0;
					RESETTIMECOUNTER( giTimerAirRaidDiveStarted, AIR_RAID_DIVE_INTERVAL );

					if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
					{
						// Free up attacker...
						FreeUpAttacker(gpRaidSoldier);
						SLOGD("Tried to free up attacker AIR RAID ENDING DIVE, attack count now %d",
							gTacticalStatus.ubAttackBusyCount);
					}

					gubAirRaidMode = AIR_RAID_LOOK_FOR_DIVE;
					break;

				case AIR_RAID_END_BOMBING:

					RESETTIMECOUNTER( giTimerAirRaidDiveStarted, AIR_RAID_DIVE_INTERVAL );
					giNumTurnsSinceLastDive = 0;

					if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
					{
						// Free up attacker...
						FreeUpAttacker(gpRaidSoldier);
						SLOGD("Tried to free up attacker AIR RAID ENDING DIVE, attack count now %d",
							gTacticalStatus.ubAttackBusyCount);
					}

					gubAirRaidMode = AIR_RAID_LOOK_FOR_DIVE;
					break;

				case AIR_RAID_BEGIN_BOMBING:
					BeginBombing( );
					break;

				case AIR_RAID_BOMBING:
					DoBombing( );
					break;
			}
		}

		if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
		{
			// Are we through with attacker busy count?
			if ( gTacticalStatus.ubAttackBusyCount == 0 )
			{
				// Relinquish control....
				gfAirRaidHasHadTurn = TRUE;
				gfHaveTBBatton = FALSE;
				BeginTeamTurn( gubBeginTeamTurn );
			}
		}
	}
	else
	{
		if (gubAirRaidMode == AIR_RAID_PENDING)
		{
			BeginAirRaid();
		}
	}
}


BOOLEAN InAirRaid( )
{
	return( gfInAirRaid || gubAirRaidMode == AIR_RAID_PENDING);
}


BOOLEAN HandleAirRaidEndTurn( UINT8 ubTeam )
{
	if ( !gfInAirRaid )
	{
		return( TRUE );
	}

	if ( gfAirRaidHasHadTurn )
	{
		gfAirRaidHasHadTurn = FALSE;
		return( TRUE );
	}

	giNumTurnsSinceLastDive++;
	giNumTurnsSinceDiveStarted++;
	giNumGridNosMovedThisTurn = 0;
	gubBeginTeamTurn = ubTeam;
	gfHaveTBBatton = TRUE;

	// ATE: Even if we have an attacker busy problem.. init to 0 now
	gTacticalStatus.ubAttackBusyCount = 0;

	// Increment attacker bust count....
	gTacticalStatus.ubAttackBusyCount++;
	SLOGD("Starting attack AIR RAID, attack count now %d", gTacticalStatus.ubAttackBusyCount);

	AddTopMessage(AIR_RAID_TURN_MESSAGE);

	// OK, handle some sound effects, depending on the mode we are in...
	if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
	{
		switch( gubAirRaidMode )
		{
			case AIR_RAID_BOMBING:

				// Start diving sound...
				PlayJA2Sample(S_RAID_TB_BOMB, HIGHVOLUME, 1, MIDDLEPAN);
				break;

			case AIR_RAID_BEGIN_DIVE:

				PlayJA2Sample(S_RAID_TB_DIVE, HIGHVOLUME, 1, MIDDLEPAN);
				break;
		}
	}

	return( FALSE );
}


void SaveAirRaidInfoToSaveGameFile(HWFILE const hFile)
{
	AIR_RAID_SAVE_STRUCT sAirRaidSaveStruct;

	// Put all the globals into the save struct
	sAirRaidSaveStruct.fInAirRaid = gfInAirRaid;
	sAirRaidSaveStruct.fAirRaidScheduled = gfAirRaidScheduled;
	sAirRaidSaveStruct.ubAirRaidMode = gubAirRaidMode;
	// HACK: The sound engine is not save-persistent so this id is invalid
	//sAirRaidSaveStruct.uiSoundSample = guiSoundSample;
	sAirRaidSaveStruct.uiSoundSample = guiSoundVolume;
	sAirRaidSaveStruct.uiRaidLastUpdate = guiRaidLastUpdate;
	sAirRaidSaveStruct.fFadingRaidIn = gfFadingRaidIn;
	sAirRaidSaveStruct.fQuoteSaid = gfQuoteSaid;
	sAirRaidSaveStruct.bNumDives = gbNumDives;
	sAirRaidSaveStruct.bMaxDives = gbMaxDives;
	sAirRaidSaveStruct.fFadingRaidOut = gfFadingRaidOut;
	sAirRaidSaveStruct.sDiveX = gsDiveX;
	sAirRaidSaveStruct.sDiveY = gsDiveY;
	sAirRaidSaveStruct.sDiveTargetLocation = gsDiveTargetLocation;
	sAirRaidSaveStruct.ubDiveDirection = gubDiveDirection;
	sAirRaidSaveStruct.sNumGridNosMoved = gsNumGridNosMoved;
	sAirRaidSaveStruct.iNumTurnsSinceLastDive = giNumTurnsSinceLastDive;
	sAirRaidSaveStruct.iNumTurnsSinceDiveStarted = giNumTurnsSinceDiveStarted;
	sAirRaidSaveStruct.iNumGridNosMovedThisTurn = giNumGridNosMovedThisTurn;
	sAirRaidSaveStruct.fAirRaidHasHadTurn = gfAirRaidHasHadTurn;
	sAirRaidSaveStruct.ubBeginTeamTurn = gubBeginTeamTurn;
	sAirRaidSaveStruct.fHaveTBBatton = gfHaveTBBatton;

	sAirRaidSaveStruct.sNotLocatedYet = gsNotLocatedYet;
	sAirRaidSaveStruct.iNumFrames = giNumFrames;


	if( gpRaidSoldier )
	{
		sAirRaidSaveStruct.bLevel = gpRaidSoldier->bLevel;
		sAirRaidSaveStruct.bTeam = gpRaidSoldier->bTeam;
		sAirRaidSaveStruct.bSide = gpRaidSoldier->bSide;
		sAirRaidSaveStruct.ubAttackerID = Soldier2ID(gpRaidSoldier->attacker);
		sAirRaidSaveStruct.usAttackingWeapon = gpRaidSoldier->usAttackingWeapon;
		sAirRaidSaveStruct.dXPos = gpRaidSoldier->dXPos;
		sAirRaidSaveStruct.dYPos = gpRaidSoldier->dYPos;
		sAirRaidSaveStruct.sX = gpRaidSoldier->sX;
		sAirRaidSaveStruct.sY = gpRaidSoldier->sY;
		sAirRaidSaveStruct.sGridNo = gpRaidSoldier->sGridNo;

		sAirRaidSaveStruct.sRaidSoldierID = MAX_NUM_SOLDIERS - 1;
	}
	else
		sAirRaidSaveStruct.sRaidSoldierID = -1;


	sAirRaidSaveStruct.AirRaidDef = gAirRaidDef;


	//Save the Air Raid Save Struct
	FileWrite(hFile, &sAirRaidSaveStruct, sizeof(AIR_RAID_SAVE_STRUCT));
}


void LoadAirRaidInfoFromSaveGameFile(HWFILE const hFile)
{
	AIR_RAID_SAVE_STRUCT sAirRaidSaveStruct;

	FileRead(hFile, &sAirRaidSaveStruct, sizeof(AIR_RAID_SAVE_STRUCT));

	// Put all the globals into the save struct
	gfInAirRaid = sAirRaidSaveStruct.fInAirRaid;
	gfAirRaidScheduled = sAirRaidSaveStruct.fAirRaidScheduled;
	gubAirRaidMode = sAirRaidSaveStruct.ubAirRaidMode;
	//guiSoundSample = sAirRaidSaveStruct.uiSoundSample;
	// HACK: The sound engine is not save-persistent so this id is invalid
	if (guiSoundSample != NO_SAMPLE)
	{
		SoundStop(guiSoundSample);
		guiSoundSample = NO_SAMPLE;
	}
	guiSoundVolume = sAirRaidSaveStruct.uiSoundSample;
	guiRaidLastUpdate = sAirRaidSaveStruct.uiRaidLastUpdate;
	gfFadingRaidIn = sAirRaidSaveStruct.fFadingRaidIn;
	gfQuoteSaid = sAirRaidSaveStruct.fQuoteSaid;
	gbNumDives = sAirRaidSaveStruct.bNumDives;
	gbMaxDives = sAirRaidSaveStruct.bMaxDives;
	gfFadingRaidOut = sAirRaidSaveStruct.fFadingRaidOut;
	gsDiveX = sAirRaidSaveStruct.sDiveX;
	gsDiveY = sAirRaidSaveStruct.sDiveY;
	gsDiveTargetLocation = sAirRaidSaveStruct.sDiveTargetLocation;
	gubDiveDirection = sAirRaidSaveStruct.ubDiveDirection;
	gsNumGridNosMoved = sAirRaidSaveStruct.sNumGridNosMoved;
	giNumTurnsSinceLastDive = sAirRaidSaveStruct.iNumTurnsSinceLastDive;
	giNumTurnsSinceDiveStarted = sAirRaidSaveStruct.iNumTurnsSinceDiveStarted;
	giNumGridNosMovedThisTurn = sAirRaidSaveStruct.iNumGridNosMovedThisTurn;
	gfAirRaidHasHadTurn = sAirRaidSaveStruct.fAirRaidHasHadTurn;
	gubBeginTeamTurn = sAirRaidSaveStruct.ubBeginTeamTurn;
	gfHaveTBBatton = sAirRaidSaveStruct.fHaveTBBatton;

	gsNotLocatedYet = sAirRaidSaveStruct.sNotLocatedYet;
	giNumFrames = sAirRaidSaveStruct.iNumFrames;


	if( sAirRaidSaveStruct.sRaidSoldierID != -1 )
	{
		SOLDIERTYPE& s = GetMan(sAirRaidSaveStruct.sRaidSoldierID);
		s.bLevel            = sAirRaidSaveStruct.bLevel;
		s.bTeam             = sAirRaidSaveStruct.bTeam;
		s.bSide             = sAirRaidSaveStruct.bSide;
		s.attacker          = ID2Soldier(sAirRaidSaveStruct.ubAttackerID);
		s.usAttackingWeapon = sAirRaidSaveStruct.usAttackingWeapon;
		s.dXPos             = sAirRaidSaveStruct.dXPos;
		s.dYPos             = sAirRaidSaveStruct.dYPos;
		s.sX                = sAirRaidSaveStruct.sX;
		s.sY                = sAirRaidSaveStruct.sY;
		s.sGridNo           = sAirRaidSaveStruct.sGridNo;
		gpRaidSoldier = &s;
	}
	else
		gpRaidSoldier = NULL;

	gAirRaidDef = sAirRaidSaveStruct.AirRaidDef;
}


static void SetTeamStatusGreen(INT8 team)
{
	FOR_EACH_IN_TEAM(s, team)
	{
		if (s->bInSector)
		{
			s->bAlertStatus = STATUS_GREEN;
			s->bUnderFire = 0;
		}
	}
	gTacticalStatus.Team[team].bAwareOfOpposition = FALSE;
}


static void SetTeamStatusRed(INT8 team)
{
	FOR_EACH_IN_TEAM(s, team)
	{
		if (s->bInSector)
		{
			s->bAlertStatus = STATUS_RED;
			s->bUnderFire = gAirRaidDef.bIntensity;
		}
	}
	gTacticalStatus.Team[team].bAwareOfOpposition = TRUE;
}


void EndAirRaid( )
{
	// Update enemy in sector status
	gTacticalStatus.fEnemyInSector = NumEnemyInSector() > 0;
	gfBlitBattleSectorLocator = gTacticalStatus.fEnemyInSector;

	// Stop sound
	SoundStop( guiSoundSample );

	if (gbNumDives < gbMaxDives)
	{
		ChopperAttackSector(gAirRaidDef.sSectorX, gAirRaidDef.sSectorY, gbMaxDives - gbNumDives);
	}

	// Change music back...
	if ( !( gTacticalStatus.uiFlags & INCOMBAT ) )
	{
		if (!IsTeamActive(ENEMY_TEAM) && !IsTeamActive(CREATURE_TEAM))
		{
			SetMusicMode(MUSIC_TACTICAL_NOTHING);

			SetTeamStatusGreen(MILITIA_TEAM);
			SetTeamStatusGreen(CIV_TEAM);
			gubEnemyEncounterCode = NO_ENCOUNTER_CODE;
		}
		else
		{
			gubEnemyEncounterCode = ENEMY_INVASION_CODE;
		}
	}
	else
	{
		// Free up attacker...
		FreeUpAttacker(gpRaidSoldier);
		SLOGD("Tried to free up attacker AIR RAID NO DIVE, attack count now %d",
			gTacticalStatus.ubAttackBusyCount);
	}

	// Reset the globals of this "class"
	gfInAirRaid = FALSE;
	gfAirRaidScheduled = FALSE;
	gubAirRaidMode = AIR_RAID_INACTIVE;
	guiSoundSample = NO_SAMPLE;
	gfFadingRaidIn = FALSE;
	gfFadingRaidOut = FALSE;
	gfAirRaidHasHadTurn = FALSE;
	gubBeginTeamTurn = 0;
	gfHaveTBBatton = FALSE;
	gsNotLocatedYet = FALSE;
	guiSoundVolume = 0;

	SLOGD("Ending Air Raid." );
}

BOOLEAN WillAirRaidBeStopped(INT16 sSectorX, INT16 sSectorY)
{
	INT8 bSamNumber = -1;
	INT8 bSAMCondition;
	UINT8 ubChance;

	if (IsItRaining())
	{
		SLOGD("WillAirRaidBeStopped: it is raining");
		return(TRUE);
	}

	if (NightTime())
	{
		SLOGD("WillAirRaidBeStopped: it is night");
		return(TRUE);
	}

	SLOGD("WillAirRaidBeStopped: enemy air controlled = %d", StrategicMap[CALCULATE_STRATEGIC_INDEX(sSectorX, sSectorY)].fEnemyAirControlled);

	// if enemy controls this SAM site, then it can't stop an air raid
	if (StrategicMap[CALCULATE_STRATEGIC_INDEX(sSectorX, sSectorY)].fEnemyAirControlled == TRUE)
	{
		return(FALSE);
	}

	if (!StrategicMap[(AIRPORT_X + (MAP_WORLD_X * AIRPORT_Y))].fEnemyControlled && !StrategicMap[(AIRPORT2_X + (MAP_WORLD_X * AIRPORT2_Y))].fEnemyControlled)
	{
		SLOGD("WillAirRaidBeStopped: enemy has no more airports");
		return(TRUE);
	}

	// which SAM controls this sector?
	bSamNumber = GCM->getControllingSamSite(SECTOR(sSectorX, sSectorY));

	SLOGD("WillAirRaidBeStopped: SAM number = %d", bSamNumber);

	// if none of them
	if (bSamNumber < 0)
	{
		return(FALSE);
	}

	// get the condition of that SAM site (NOTE: SAM #s are 1-4, but indexes are 0-3!!!)
	auto samList = GCM->getSamSites();
	Assert(bSamNumber < NUMBER_OF_SAMS);
	bSAMCondition = StrategicMap[SECTOR_INFO_TO_STRATEGIC_INDEX(samList[bSamNumber]->sectorId)].bSAMCondition;

	// if it's too busted to work, then it can't stop an air raid
	SLOGD("WillAirRaidBeStopped: SAM condition = %d", bSAMCondition);
	if (bSAMCondition < MIN_CONDITION_FOR_SAM_SITE_TO_WORK)
	{
		// no problem, SAM site not working
		return(FALSE);
	}


	// Friendly airspace controlled by a working SAM site, so SAM site fires a SAM at air raid bomber

	// calc chance that chopper will be shot down
	ubChance = bSAMCondition;

	if (PreRandom(100) < ubChance)
	{
		SLOGD("WillAirRaidBeStopped: return true");
		return(TRUE);
	}

	SLOGD("WillAirRaidBeStopped: return false");
	return(FALSE);
}

void ChopperAttackSector(UINT8 ubSectorX, UINT8 ubSectorY, INT8 bIntensity)
{
	// TODO: Balance this effect as soon as the queen uses air-raids
	UINT8 ubCasualties = Random(bIntensity + 1);
	UINT8 ubNumMilitia = CountAllMilitiaInSector(ubSectorX, ubSectorY);
	UINT8 const bTownId = GetTownIdForSector(SECTOR(ubSectorX, ubSectorY));
	SECTORINFO *pSectorInfo = &(SectorInfo[SECTOR(ubSectorX, ubSectorY)]);

	// Update the schedule
	gfInAirRaid = FALSE;
	gbNumDives = gbMaxDives;

	switch (Random(4))
	{
		case 0:	PlayJA2Sample(S_RAID_TB_DIVE, MIDVOLUME, 1, MIDDLEPAN); break;
		case 1: PlayJA2Sample(S_RAID_TB_BOMB, MIDVOLUME, 1, MIDDLEPAN); break;
		case 2: PlayJA2Sample(S_RAID_DIVE, MIDVOLUME, 1, MIDDLEPAN); break;
		default: PlayJA2Sample(S_RAID_WHISTLE, MIDVOLUME, 1, MIDDLEPAN);
	}

	if (!gfTownUsesLoyalty[bTownId])
	{
		return;
	}

	if (!pSectorInfo->fSurfaceWasEverPlayerControlled)
	{
		return;
	}

	if (ubNumMilitia > 0)
	{
		HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_ABANDON_MILITIA, ubSectorX, ubSectorY, 0);
		SLOGD("ChopperAttackSector: Militia abandoned");
	}

	UINT8 i;
	for (i = 0; i < ubCasualties; i++)
	{
		// Kill lowly soldiers first
		if (pSectorInfo->ubNumberOfCivsAtLevel[GREEN_MILITIA] > 0)
		{
			HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_NATIVE_KILLED, ubSectorX, ubSectorY, 0);
			StrategicRemoveMilitiaFromSector(ubSectorX, ubSectorY, GREEN_MILITIA, 1);
			continue;
		}
		if (pSectorInfo->ubNumberOfCivsAtLevel[REGULAR_MILITIA] > 0)
		{
			HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_NATIVE_KILLED, ubSectorX, ubSectorY, 0);
			StrategicRemoveMilitiaFromSector(ubSectorX, ubSectorY, REGULAR_MILITIA, 1);
			continue;
		}
		if (pSectorInfo->ubNumberOfCivsAtLevel[ELITE_MILITIA] > 0)
		{
			HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_NATIVE_KILLED, ubSectorX, ubSectorY, 0);
			StrategicRemoveMilitiaFromSector(ubSectorX, ubSectorY, ELITE_MILITIA, 1);
			continue;
		}
		break;
	}

	for (; i < ubCasualties; i++)
	{
		HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_NATIVE_KILLED, ubSectorX, ubSectorY, 0);
		HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_NATIVE_KILLED, ubSectorX, ubSectorY, 0);
		HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_NATIVE_KILLED, ubSectorX, ubSectorY, 0);
		HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_NATIVE_KILLED, ubSectorX, ubSectorY, 0);
		HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_NATIVE_KILLED, ubSectorX, ubSectorY, 0);
	}
}

#ifdef WITH_UNITTESTS
#include "gtest/gtest.h"

TEST(AirRaid, asserts)
{
	EXPECT_EQ(sizeof(AIR_RAID_SAVE_STRUCT), 132u);
	EXPECT_EQ(sizeof(AIR_RAID_DEFINITION), 24u);
}

#endif
