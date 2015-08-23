const static vec3 poly_building_v[] = {
	{-0x10000, -0x28000, 0x30000},
	{-0x10000,  0x10000, 0x30000},
	{ 0x00000, -0x20000, 0x20000},
	{ 0x00000,  0x10000, 0x20000},
	{ 0x10000, -0x28000, 0x30000},
	{ 0x10000,  0x10000, 0x30000},
	{ 0x00000, -0x30000, 0x40000},
	{ 0x00000,  0x10000, 0x40000},
};
const static uint16_t poly_building_i[] = {
	0, 6, 2, 4,

	0, 2, 1, 3,
	2, 4, 3, 5,
	4, 6, 5, 7,
	6, 0, 7, 1,
};
const static uint32_t poly_building_c[] = {
	0x28FFFFFF,

	0x28CCCCCC,
	0x28999999,
	0x28777777,
	0x28999999,

	0
};
mesh_s poly_building = {
	8,
	poly_building_v,
	poly_building_i,
	poly_building_c,
};


