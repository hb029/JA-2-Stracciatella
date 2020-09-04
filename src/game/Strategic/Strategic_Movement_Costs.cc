#include "Campaign_Types.h"
#include "Strategic_Movement.h"
#include "Strategic_Movement_Costs.h"
#include "GameInstance.h"
#include "DefaultContentManager.h"
#include "MovementCostsModel.h"


void InitStrategicMovementCosts()
{
	auto movementCosts = GCM->getMovementCosts();
	for (INT32 y = 0; y < 16; ++y)
	{
		for (INT32 x = 0; x < 16; ++x)
		{
			SECTORINFO& s = SectorInfo[SECTOR(x + 1, y + 1)];
			s.ubTravelRating                           = movementCosts->getTravelRating(x, y);
			s.ubTraversability[WEST_STRATEGIC_MOVE]    = movementCosts->getTraversibilityWestEast(x, y);
			s.ubTraversability[EAST_STRATEGIC_MOVE]    = movementCosts->getTraversibilityWestEast(x + 1, y);
			s.ubTraversability[NORTH_STRATEGIC_MOVE]   = movementCosts->getTraversibilityNorthSouth(x, y);
			s.ubTraversability[SOUTH_STRATEGIC_MOVE]   = movementCosts->getTraversibilityNorthSouth(x, y + 1);
			s.ubTraversability[THROUGH_STRATEGIC_MOVE] = movementCosts->getTraversibilityThrough(x, y);
		}
	}
}


UINT8 GetTraversability( INT16 sStartSector, INT16 sEndSector )
{
	UINT8 ubDirection = 0;
	INT16 sDifference = 0;

	// given start and end sectors
	sDifference = sEndSector - sStartSector;


	if( sDifference == -1 )
	{
		ubDirection = WEST_STRATEGIC_MOVE;
	}
	else if( sDifference == 1 )
	{
		ubDirection = EAST_STRATEGIC_MOVE;
	}
	else if( sDifference == 16 )
	{
		ubDirection = SOUTH_STRATEGIC_MOVE;
	}
	else
	{
		ubDirection = NORTH_STRATEGIC_MOVE;
	}

	return( SectorInfo[ sStartSector ].ubTraversability[ ubDirection ] );
}


bool SectorIsPassable(INT16 const sSector)
{
	// returns true if the sector is impassable in all directions
	const UINT8 t = SectorInfo[sSector].ubTraversability[THROUGH_STRATEGIC_MOVE];
	return t != GROUNDBARRIER && t != EDGEOFWORLD;
}

