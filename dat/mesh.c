//
// BUILDING
//
const static vec3 poly_building_v[] = {
	{-0x10000, -0x28000, 0x00000},
	{-0x10000,  0x10000, 0x00000},
	{ 0x00000, -0x20000,-0x10000},
	{ 0x00000,  0x10000,-0x10000},
	{ 0x10000, -0x28000, 0x00000},
	{ 0x10000,  0x10000, 0x00000},
	{ 0x00000, -0x30000, 0x10000},
	{ 0x00000,  0x10000, 0x10000},
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

//
// TREE1
// TODO: make this a tree and not a building
//
const static vec3 poly_tree1_v[] = {
	{-0x10000, -0x28000, 0x00000},
	{-0x10000,  0x10000, 0x00000},
	{ 0x00000, -0x20000,-0x10000},
	{ 0x00000,  0x10000,-0x10000},
	{ 0x10000, -0x28000, 0x00000},
	{ 0x10000,  0x10000, 0x00000},
	{ 0x00000, -0x30000, 0x10000},
	{ 0x00000,  0x10000, 0x10000},
};
const static uint16_t poly_tree1_i[] = {
	0, 6, 2, 4,

	0, 2, 1, 3,
	2, 4, 3, 5,
	4, 6, 5, 7,
	6, 0, 7, 1,
};
const static uint32_t poly_tree1_c[] = {
	0x28FFFFFF,

	0x28CCCCCC,
	0x28999999,
	0x28777777,
	0x28999999,

	0
};
mesh_s poly_tree1 = {
	8,
	poly_tree1_v,
	poly_tree1_i,
	poly_tree1_c,
};

//
// SHIP1
//
const static vec3 poly_ship1_v[] = {
	{ 0x00000,  0x00000, 0x20000},
	{-0x08000,  0x00000, 0x00000},
	{ 0x08000,  0x00000, 0x00000},
	{ 0x00000, -0x05000, 0x00000},

	{-0x04000,  0x00000, 0x10000},
	{ 0x04000,  0x00000, 0x10000},
	{-0x18000,  0x00000,-0x03000},
	{ 0x18000,  0x00000,-0x03000},

};
const static uint16_t poly_ship1_i[] = {
	//0, 1, 2,
	3, 0, 2,
	3, 1, 0,
	3, 2, 1,

	1, 4, 6,
	2, 5, 7,
};
const static uint32_t poly_ship1_c[] = {
	//0xA0336699,
	0xA055AAFF,
	0xA04499EE,
	0xA0EE9944,

	0xA055AAFF,
	0xA04499EE,

	0
};
mesh_s poly_ship1 = {
	8,
	poly_ship1_v,
	poly_ship1_i,
	poly_ship1_c,
};

