typedef struct mesh
{
	const uint32_t vc;
	const vec3 *v;
	const uint16_t *i;
	const uint32_t *c;
} mesh_s;

typedef struct poly_chunk
{
	fixed priority;
	int len;
	uint32_t cmd_list[1+4];
	uint32_t _pad0[1];
} poly_chunk_s;

poly_chunk_s *pclist = NULL;
int pclist_num = 0;
int pclist_max = 0;

mat4 mat_cam, mat_obj;
mat4 mat_icam;

static void mesh_clear(void)
{
	pclist_num = 0;
}

static void mesh_flush(void)
{
	int i, j;

	for(i = 0; i < pclist_num; i++)
	{
		gpu_send_control_gp0(pclist[i].cmd_list[0]);
		for(j = 1; j < pclist[i].len; j++)
			gpu_send_data(pclist[i].cmd_list[j]);
	}

	mesh_clear();

}

static poly_chunk_s *mesh_alloc_poly(fixed priority)
{
	// Get index
	int idx = pclist_num++;

	// Ensure space
	if(idx >= pclist_max)
	{
		if(pclist_max == 0)
			pclist_max = 10;
		else
			pclist_max = (pclist_max*3)/2+1;

		if(pclist_max < idx+1)
			pclist_max = idx+1;

		pclist = realloc(pclist, pclist_max*sizeof(poly_chunk_s));
	}

	// Sort
	pclist[idx].priority = priority;
	// TODO

	// Return!
	return &pclist[idx];
}

static void mesh_add_poly(const mesh_s *mesh, vec4 *v, int ic, int ii)
{
	int j;
	const uint16_t *l = &mesh->i[ii];

	// Get poly parameters
	int vcount = ((mesh->c[ic] & 0x08000000) != 0
		? 4
		: 3);

	// Check facing + range
	for(j = 0; j < vcount; j++)
	{
		if(v[l[j]][2] <= 0x0080) return;
		if(v[l[j]][0] <= -1023) return;
		if(v[l[j]][0] >=  1023) return;
		if(v[l[j]][1] <= -1023) return;
		if(v[l[j]][1] >=  1023) return;
	}

	// Check direction (2D cross product)
	int dx0 = (v[l[1]][0] - v[l[0]][0]);
	int dy0 = (v[l[1]][1] - v[l[0]][1]);
	int dx1 = (v[l[2]][0] - v[l[0]][0]);
	int dy1 = (v[l[2]][1] - v[l[0]][1]);
	if(dx0*dy1 - dy0*dx1 < 0)
		return;

	// Calculate cross product
	vec4 fnorm;
	vec4_cross_origin(&fnorm, &mesh->v[l[0]], &mesh->v[l[1]], &mesh->v[l[2]]);

	// Draw
	fixed priority = 0;
	poly_chunk_s *pc = mesh_alloc_poly(priority);
	if(pc != NULL)
	{
		pc->len = 1+vcount;
		pc->cmd_list[0] = mesh->c[ic];
		for(j = 0; j < vcount; j++)
		{
			pc->cmd_list[j+1] = (
				(v[l[j]][0]&0xFFFF) +
				(v[l[j]][1]<<16));
		}
	}
}

static void mesh_draw(const mesh_s *mesh)
{
	volatile int lag;
	int i, j;

	// Build points
	static vec4 v[128];
	int iv, ii, ic;
	for(i = 0; i < mesh->vc && i < 128; i++)
	{
		v[i][0] = mesh->v[i][0];
		v[i][1] = mesh->v[i][1];
		v[i][2] = mesh->v[i][2];
		v[i][3] = 0x10000;
		mat4_apply_vec4(&v[i], &mat_obj);
		mat4_apply_vec4(&v[i], &mat_cam);
	}

	// Light + project points
	// (lighting them will be optional here)
	/*
	vec4 lite_dst = {0x2000, -0xA000, -0xA000, 0};
	vec4_normalize_3(&lite_dst);
	*/
	for(i = 0; i < mesh->vc && i < 128; i++)
	{
		v[i][0] = v[i][0]*112/v[i][2];
		v[i][1] = v[i][1]*112/v[i][2];
	}

	// Draw mesh
	gpu_send_control_gp1(0x01000000);
	for(ic = 0, ii = 0; mesh->c[ic] != 0;
		ii += ((mesh->c[ic++] & 0x08000000) != 0 ? 4 : 3))
	{
		mesh_add_poly(mesh, v, ic, ii);
	}
}

