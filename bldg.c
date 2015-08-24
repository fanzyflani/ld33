typedef struct bldg
{
	vec3 pos;
	mesh_s *mesh;
} bldg_s;

bldg_s bldg_tests[3] = {
	{{0, 0, 0x180000}, &poly_building},
	{{0x50000, 0, 0x100000}, &poly_tree1},
	{{-0x28000, 0, 0x130000}, &poly_tree1},
};

static void bldg_draw(const bldg_s *bldg)
{
	// TODO: check if in view dist

	mat4_load_identity(&mat_obj);
	mat4_translate_vec3(&mat_obj, (vec3 *)&bldg->pos);
	mat4_translate_imm3(&mat_obj, 0, hmap_get(bldg->pos[0], bldg->pos[2]), 0);
	mesh_draw(bldg->mesh, 0);
}

