#pragma once

#include "BobbyRMailOrder.h"
#include "Campaign_Types.h"
#include "JsonObject.h"

#include "rapidjson/document.h"


class ShippingDestinationModel
{
public:
	ShippingDestinationModel(uint8_t locationId_,
		uint16_t chargeRateOverNight_, uint16_t chargeRate2Days_, uint16_t chargeRateStandard_,
		uint8_t flowersNextDayDeliveryCost_, uint8_t flowersWhenItGetsThereCost_,
		bool canDeliver_, bool isPrimary_,
		uint8_t deliverySectorId_, uint8_t deliverySectorZ_, int16_t deliverySectorGridNo_, 
		int16_t emailOffset_, int16_t emailLength_);

	uint8_t getDeliverySector() const;

	static ShippingDestinationModel* deserialize(JsonObjectReader& obj);
	static void validateData(std::vector<const ShippingDestinationModel*> destinations, std::vector<const ST::string*> destinationNames);

	const uint8_t locationId;

	const uint16_t chargeRateOverNight;
	const uint16_t chargeRate2Days;
	const uint16_t chargeRateStandard;

	const uint8_t flowersNextDayDeliveryCost;
	const uint8_t flowersWhenItGetsThereCost;

	const bool canDeliver;
	const bool isPrimary;

	const uint8_t deliverySectorX;
	const uint8_t deliverySectorY;
	const uint8_t deliverySectorZ;
	const int16_t deliverySectorGridNo;
	const int32_t emailOffset;
	const int32_t emailLength;
};
