#ifndef EQEMU_WATER_MAP_V2_H
#define EQEMU_WATER_MAP_V2_H

#include "water_map.h"
#include "oriented_bounding_box.h"
#include <vector>
#include <utility>

class WaterMapV2 : public WaterMap
{
public:
	WaterMapV2();
	~WaterMapV2();

	virtual WaterRegionType ReturnRegionType(const xyz_location& location) const;
	virtual bool InWater(const xyz_location& location) const;
	virtual bool InVWater(const xyz_location& location) const;
	virtual bool InLava(const xyz_location& location) const;
	virtual bool InLiquid(const xyz_location& location) const;

protected:
	virtual bool Load(FILE *fp);

	std::vector<std::pair<WaterRegionType, OrientedBoundingBox>> regions;
	friend class WaterMap;
};

#endif
