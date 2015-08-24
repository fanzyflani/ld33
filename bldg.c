typedef struct bldg
{
	vec3 pos;
	mesh_s *mesh;
} bldg_s;

int bldg_num = 3;
bldg_s bldg_list[3] = {
	{{0, 0, 0x180000}, &poly_building},
	{{0x50000, 0, 0x100000}, &poly_tree1},
	{{-0x28000, 0, 0x130000}, &poly_tree1},
};

static void bldg_draw(const bldg_s *bldg)
{
	// Check if in view dist
	int hx = (bldg->pos[0]>>18);
	int hz = (bldg->pos[2]>>18);
	hx -= hmap_visx;
	hz -= hmap_visz;
	if(hx < 0) hx = -hx;
	if(hz < 0) hz = -hz;

	if(hx > VISRANGE || hz > VISRANGE) return;

	mat4_load_identity(&mat_obj);
	mat4_translate_vec3(&mat_obj, (vec3 *)&bldg->pos);
	mat4_translate_imm3(&mat_obj, 0, hmap_get(bldg->pos[0], bldg->pos[2]), 0);
	mesh_draw(bldg->mesh, 0);
}

