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
	vec4 pts[4];
	vec4 fnorm;
	//uint32_t _pad0[1];
} poly_chunk_s;

poly_chunk_s *pclist = NULL;
int pclist_num = 0;
int pclist_max = 0;

mat4 mat_cam, mat_obj;
mat4 mat_icam;
mat4 mat_iplr;

vec4 vbase[128];

static void mesh_clear(void)
{
	pclist_num = 0;
}

static int mesh_flush_sort_compar(const void *a, const void *b)
{
	poly_chunk_s *ap = (poly_chunk_s *)a;
	poly_chunk_s *bp = (poly_chunk_s *)b;

	fixed apd = ap->pts[0][2];
	fixed bpd = bp->pts[0][2];

	return bpd-apd;
}

static void mesh_flush(int do_sort)
{
	int i, j;

	// Sort
	if(do_sort)
		qsort(pclist, pclist_num, sizeof(poly_chunk_s), mesh_flush_sort_compar);

	// Draw
	for(i = 0; i < pclist_num; i++)
	{
		gpu_send_control_gp0(pclist[i].cmd_list[0]);
		for(j = 1; j < pclist[i].len; j++)
			gpu_send_data(pclist[i].cmd_list[j]);
	}

	// Clear
	mesh_clear();

}

static poly_chunk_s *mesh_alloc_poly()
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

	// Return!
	return &pclist[idx];
}

static void mesh_add_poly(const mesh_s *mesh, vec4 *vp, int ic, int ii)
{
	int i, j;
	const uint16_t *l = &mesh->i[ii];

	// Get poly parameters
	int vcount = ((mesh->c[ic] & 0x08000000) != 0
		? 4
		: 3);

	// Check facing + range
	for(j = 0; j < vcount; j++)
	{
		if(vp[l[j]][2] <= 0x0008) return;
		if(vp[l[j]][0] <= -1023) return;
		if(vp[l[j]][0] >=  1023) return;
		if(vp[l[j]][1] <= -1023) return;
		if(vp[l[j]][1] >=  1023) return;
	}

	// Check direction (2D cross product)
	// Abuse top bit for bidirectional flag
	if((mesh->c[ic] & 0x80000000) == 0)
	{
		int dx0 = (vp[l[1]][0] - vp[l[0]][0]);
		int dy0 = (vp[l[1]][1] - vp[l[0]][1]);
		int dx1 = (vp[l[2]][0] - vp[l[0]][0]);
		int dy1 = (vp[l[2]][1] - vp[l[0]][1]);
		if(dx0*dy1 - dy0*dx1 < 0)
			return;
	}

	// Calculate cross product
	vec4 fnorm;
	vec4_cross_origin(&fnorm, &vbase[l[0]], &vbase[l[1]], &vbase[l[2]]);

	// Calculate priority
	//fixed priority = vec4_dot_3(&fnorm, mat_cam[3]) - vec4_dot_3(&fnorm, v[l[1]]);
	// kinda bad priority, but it'll do for now
	/*
	vec4 vsum;
	for(j = 0; j < 3; j++)
	{
		vsum[j] = 0;
		for(i = 0; i < vcount; i++)
			vsum[j] += mesh->v[l[i]][j];

		vsum[j] /= vcount;
	}
	fixed priority = -vec4_dot_3(&mat_icam[2], &vsum);
	*/
	// priority will be handled on a case-by-case basis

	// Draw
	poly_chunk_s *pc = mesh_alloc_poly();
	if(pc != NULL)
	{
		pc->len = 1+vcount;
		vec4_copy(&pc->fnorm, &fnorm);
		pc->cmd_list[0] = mesh->c[ic] & 0x7FFFFFFF;
		for(j = 0; j < vcount; j++)
		{
			vec4_copy((vec4 *)&pc->pts[j], (vec4 *)&vbase[l[j]]);
			pc->cmd_list[j+1] = (
				(vp[l[j]][0]&0xFFFF) +
				(vp[l[j]][1]<<16));
		}
	}
}

static void mesh_draw(const mesh_s *mesh)
{
	volatile int lag;
	int i, j;

	// Build points
	vec4 *v = vbase;
	static vec4 vp[128];
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
		vp[i][0] = v[i][0]*112/(v[i][2] > 1 ? v[i][2] : 1);
		vp[i][1] = v[i][1]*112/(v[i][2] > 1 ? v[i][2] : 1);
		vp[i][2] = v[i][2];
		if(vp[i][0] < -1023) vp[i][0] = -1023;
		if(vp[i][0] >  1023) vp[i][0] =  1023;
		if(vp[i][1] < -1023) vp[i][1] = -1023;
		if(vp[i][1] >  1023) vp[i][1] =  1023;
	}

	// Draw mesh
	gpu_send_control_gp1(0x01000000);
	for(ic = 0, ii = 0; mesh->c[ic] != 0;
		ii += ((mesh->c[ic++] & 0x08000000) != 0 ? 4 : 3))
	{
		mesh_add_poly(mesh, vp, ic, ii);
	}
}

