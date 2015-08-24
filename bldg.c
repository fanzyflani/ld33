typedef struct bldg
{
	vec3 pos;
	mesh_s *mesh;
} bldg_s;

#define BLDG_MAX 100
int bldg_count = 0;
bldg_s bldg_list[BLDG_MAX];

static int bldg_add(fixed x, fixed z, mesh_s *mesh)
{
	int idx = bldg_count++;
	bldg_s *bldg = &bldg_list[idx];
	memset(bldg, 0, sizeof(bldg_s));
	bldg->pos[0] = x;
	bldg->pos[1] = hmap_get(x, z);
	bldg->pos[2] = z;
	bldg->mesh = mesh;

	return idx;
}

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
	//mat4_translate_imm3(&mat_obj, 0, hmap_get(bldg->pos[0], bldg->pos[2]), 0);
	mesh_draw(bldg->mesh, 0);
}

