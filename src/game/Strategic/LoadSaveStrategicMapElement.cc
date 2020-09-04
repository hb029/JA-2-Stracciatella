#include "Debug.h"
#include "FileMan.h"
#include "LoadSaveStrategicMapElement.h"
#include "LoadSaveData.h"


void ExtractStrategicMapElementFromFile(HWFILE const f, StrategicMapElement& e)
{
	BYTE data[41];
	FileRead(f, data, sizeof(data));

	DataReader d{data};
	EXTR_SKIP(d, 16)
	EXTR_I8(  d, e.bNameId)
	EXTR_BOOL(d, e.fEnemyControlled)
	EXTR_BOOL(d, e.fEnemyAirControlled)
	EXTR_SKIP(d, 1)
	EXTR_I8(  d, e.bSAMCondition)
	EXTR_SKIP(d, 20)
	Assert(d.getConsumed() == lengthof(data));
}


void InjectStrategicMapElementIntoFile(HWFILE const f, StrategicMapElement const& e)
{
	BYTE  data[41];
	DataWriter d{data};
	INJ_SKIP(d, 16)
	INJ_I8(  d, e.bNameId)
	INJ_BOOL(d, e.fEnemyControlled)
	INJ_BOOL(d, e.fEnemyAirControlled)
	INJ_SKIP(d, 1)
	INJ_I8(  d, e.bSAMCondition)
	INJ_SKIP(d, 20)
	Assert(d.getConsumed() == lengthof(data));

	FileWrite(f, data, sizeof(data));
}
