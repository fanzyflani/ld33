//
// BUILDING
//
static const vec3 poly_building_v[] = {
	{-0x10000, -0x38000, 0x00000},
	{-0x10000,  0x00000, 0x00000},
	{ 0x00000, -0x30000,-0x10000},
	{ 0x00000,  0x00000,-0x10000},
	{ 0x10000, -0x38000, 0x00000},
	{ 0x10000,  0x00000, 0x00000},
	{ 0x00000, -0x40000, 0x10000},
	{ 0x00000,  0x00000, 0x10000},
};
static const uint16_t poly_building_i[] = {
	0, 6, 2, 4,

	0, 2, 1, 3,
	2, 4, 3, 5,
	4, 6, 5, 7,
	6, 0, 7, 1,
};
static const uint32_t poly_building_c[] = {
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
//
static const vec3 poly_tree1_v[] = {
	{ 0x00000, -0x38000, 0x00000},
	{ 0x00000, -0x10000,-0x18000},
	{-0x10000, -0x10000, 0x08000},
	{ 0x10000, -0x10000, 0x08000},

	{ 0x00000, -0x10000,-0x18000/3},
	{-0x10000/3, -0x10000, 0x08000/3},
	{ 0x10000/3, -0x10000, 0x08000/3},
	{ 0x00000,  0x00000,-0x18000/3},
	{-0x10000/3,  0x00000, 0x08000/3},
	{ 0x10000/3,  0x00000, 0x08000/3},
};
static const uint16_t poly_tree1_i[] = {
	1, 2, 0,
	2, 3, 0,
	3, 1, 0,
	1, 3, 2,

	7, 8, 4, 5,
	8, 9, 5, 6,
	9, 7, 6, 4,
};
static const uint32_t poly_tree1_c[] = {
	0x2000DF00,
	0x20008F00,
	0x20006F00,
	0x20003F00,

	0x285555AA,
	0x28444499,
	0x28333388,

	0
};
mesh_s poly_tree1 = {
	10,
	poly_tree1_v,
	poly_tree1_i,
	poly_tree1_c,
};

//
// JET1
//
static const vec3 poly_jet1_v[] = {
	{ 0x00000,  0x00000, 0x10000},
	{-0x08000,  0x00000,-0x10000},
	{ 0x08000,  0x00000,-0x10000},
	{ 0x00000, -0x05000,-0x10000},

	{-0x03800,  0x00000, 0x00000},
	{ 0x03800,  0x00000, 0x00000},
	{-0x18000,  0x00000,-0x13000},
	{ 0x18000,  0x00000,-0x13000},

};
static const uint16_t poly_jet1_i[] = {
	//0, 1, 2,
	3, 0, 2,
	3, 1, 0,
	3, 2, 1,

	1, 4, 6,
	2, 5, 7,
};
static const uint32_t poly_jet1_c[] = {
	//0xA0336699,
	0xA055AAFF,
	0xA04499EE,
	0xA0EE9944,

	0xB03388DD, 0xB03388DD, 0xB055AAFF,
	0xB02877CC, 0xB02877CC, 0xB04499EE,

	0
};
mesh_s poly_jet1 = {
	8,
	poly_jet1_v,
	poly_jet1_i,
	poly_jet1_c,
};

//
// SHOT_MGUN
//
static const vec3 poly_shot_mgun_v[] = {
	{ 0x00000,  0x02000, 0x20000},
	{-0x03000,  0x02000, 0x00000},
	{ 0x03000,  0x02000, 0x00000},

};
static const uint16_t poly_shot_mgun_i[] = {
	0, 1, 2,
};
static const uint32_t poly_shot_mgun_c[] = {
	0xA0FFFFFF,

	0
};
mesh_s poly_shot_mgun = {
	8,
	poly_shot_mgun_v,
	poly_shot_mgun_i,
	poly_shot_mgun_c,
};

