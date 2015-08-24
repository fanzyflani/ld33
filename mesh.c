// disabled because of display corruption
//define USE_DMA

#define MS_SHADOW (1<<0)
#define MS_FLASH (1<<1)
#define MS_NOPRIO (1<<2)

typedef struct mesh
{
	uint32_t vc;
	const vec3 *v;
	const uint16_t *i;
	const uint32_t *c;
} mesh_s;

typedef struct poly_chunk
{
	int len;
	uint32_t dma_chain;
	uint32_t cmd_list[1+4];
	vec3 pts[4];
	//vec4 fnorm;
	//uint32_t _pad0[1];
} poly_chunk_s;

//int32_t *pcorder = NULL;
//fixed *pcprio = NULL;
//poly_chunk_s *pclist = NULL;
#define POLY_MAX 2048
int32_t pcorder[POLY_MAX];
fixed pcprio[POLY_MAX];
poly_chunk_s pclist[POLY_MAX];
int pclist_num = 0;
int pclist_max = POLY_MAX;
int mesh_tstat = 0;

mat4 mat_cam, mat_obj, mat_obj_cam;
mat4 mat_icam;

#define VTX_MAX (400)
vec4 vbase[VTX_MAX];

static void mesh_clear(void)
{
	pclist_num = 0;

#ifdef USE_DMA
	// Wait for DMA
	while((DMA_n_CHCR(2) & 0x01000000) != 0) {}
	while((GP1 & 0x10000000) == 0) {}
	//while((GP1 & 0x04000000) == 0) {}

	// Wait a bit longer to prevent breakages
	//volatile int lag;
	//for(lag = 0; lag < 0x300; lag++) {};

	// Disable DMA
	gpu_send_control_gp1(0x04000001);
	gpu_send_control_gp1(0x01000000);
#endif

}

static int mesh_flush_sort_compar(const void *a, const void *b)
{
	fixed apd = pcprio[(int32_t *)a - pcorder];
	fixed bpd = pcprio[(int32_t *)b - pcorder];

	return bpd-apd;
}

static void mesh_flush(int do_sort)
{
	int i, j;

	// Sort
	if(do_sort)
		qsort(pcorder, pclist_num, sizeof(uint32_t), mesh_flush_sort_compar);

	// Draw
	for(i = 0; i < pclist_num; i++)
	{
		int ri = pcorder[i];

#ifdef USE_DMA
		// DMA
		uint32_t nptr = (i+1 < pclist_num
			? (uint32_t )&pclist[pcorder[i+1]].dma_chain
			: 0xFFFFFF)
				& 0xFFFFFF;

		pclist[ri].dma_chain = ((pclist[ri].len+1)<<24) | nptr;

#else
		// FIFO
		gpu_send_control_gp0(pclist[ri].cmd_list[0]);
		for(j = 1; j < pclist[ri].len; j++)
			gpu_send_data(pclist[ri].cmd_list[j]);
#endif
	}

#ifdef USE_DMA
	// Send DMA
	while((GP1 & 0x10000000) == 0) {}
	while((GP1 & 0x04000000) == 0) {}
	gpu_send_control_gp1(0x04000002);
	gpu_send_control_gp1(0x01000000);
	DMA_n_CHCR(2) = 0x01000401;
	DMA_DPCR = (DMA_DPCR & ~0xF00) | 0x800;
	DMA_n_MADR(2) = ((uint32_t)&pclist[pcorder[0]].dma_chain) & 0xFFFFFF;
	DMA_n_BCR(2) = 0;
	DMA_n_CHCR(2) = 0x01000401;
#endif

	// Clear
	mesh_clear();

}

static int mesh_alloc_poly()
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

		/*
		pclist = realloc(pclist, pclist_max*sizeof(poly_chunk_s));
		pcorder = realloc(pcorder, pclist_max*sizeof(int32_t));
		pcprio = realloc(pcprio, pclist_max*sizeof(fixed));
		*/
	}

	// Return!
	pcorder[idx] = idx;
	//return &pclist[idx];
	return idx;
}

static void mesh_add_poly(const mesh_s *mesh, vec4 *vp, int ic, int ii, int flags)
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
		/*
		if(vp[l[j]][0] <= -1023) return;
		if(vp[l[j]][0] >=  1023) return;
		if(vp[l[j]][1] <= -1023) return;
		if(vp[l[j]][1] >=  1023) return;
		*/
		/*
		if(vp[l[j]][0] <= -160) return;
		if(vp[l[j]][0] >=  160) return;
		if(vp[l[j]][1] <= -120) return;
		if(vp[l[j]][1] >=  120) return;
		*/
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

	// Update stats
	mesh_tstat += (vcount-2);

	// Calculate cross product
	//vec4 fnorm;
	//vec4_cross_origin(&fnorm, &vbase[l[0]], &vbase[l[1]], &vbase[l[2]]);

	// Draw
	int pcidx = mesh_alloc_poly();
	if(pcidx >= 0)
	{
		poly_chunk_s *pc = &pclist[pcidx];
		pc->len = 1+vcount;
		//vec4_copy(&pc->fnorm, &fnorm);
		pc->cmd_list[0] = mesh->c[ic] & 0x7FFFFFFF;
		if((flags & MS_SHADOW) != 0)
		{
			pc->cmd_list[0] |= 0x02000000;
			pc->cmd_list[0] &= 0xFF000000;
		}
		else if((flags & MS_FLASH) != 0)
		{
			pc->cmd_list[0] |= 0x00FFFFFF;
		}
		fixed zsum = 0;

		if((flags & MS_NOPRIO) == 0)
		{
			for(i = 0; i < vcount; i++)
			{
				//vec4_copy((vec4 *)&pc->pts[i], (vec4 *)&vbase[l[i]]);
				pc->pts[i][0] = vbase[l[i]][0];
				pc->pts[i][1] = vbase[l[i]][1];
				pc->pts[i][2] = vbase[l[i]][2];
				zsum += pc->pts[i][2];

				pc->cmd_list[i+1] = (
					(vp[l[i]][0]&0xFFFF) +
					(vp[l[i]][1]<<16));
			}

			pcprio[pcidx] = zsum/vcount;
		} else {
			for(i = 0; i < vcount; i++)
			{
				pc->cmd_list[i+1] = (
					(vp[l[i]][0]&0xFFFF) +
					(vp[l[i]][1]<<16));
			}
			pcprio[pcidx] = 0;
		}
	}
}

static void mesh_draw(const mesh_s *mesh, int flags)
{
	volatile int lag;
	int i, j;

	// Combine matrices
	//mat4_mul_mat4_mat4(&mat_obj_cam, &mat_cam, &mat_obj);
	mat4_mul_mat4_mat4(&mat_obj_cam, &mat_obj, &mat_cam);

	// Build points
	vec4 *v = vbase;
	static vec4 vp[VTX_MAX];
	int iv, ii, ic;
	for(i = 0; i < mesh->vc && i < VTX_MAX; i++)
	{
		memcpy(&v[i], &mesh->v[i], sizeof(fixed)*3);
		v[i][3] = 0x10000;
		mat4_apply_vec4(&v[i], &mat_obj_cam);
		//mat4_apply_vec4(&v[i], &mat_obj);
		//mat4_apply_vec4(&v[i], &mat_cam);
	}

	// Light + project points
	// (lighting them will be optional here)
	/*
	vec4 lite_dst = {0x2000, -0xA000, -0xA000, 0};
	vec4_normalize_3(&lite_dst);
	*/
	for(i = 0; i < mesh->vc && i < VTX_MAX; i++)
	{
		vp[i][0] = v[i][0]*120/(v[i][2] > 1 ? v[i][2] : 1);
		vp[i][1] = v[i][1]*120/(v[i][2] > 1 ? v[i][2] : 1);
		vp[i][2] = v[i][2];
		/*
		if(vp[i][0] < -1023) vp[i][0] = -1023;
		if(vp[i][0] >  1023) vp[i][0] =  1023;
		if(vp[i][1] < -1023) vp[i][1] = -1023;
		if(vp[i][1] >  1023) vp[i][1] =  1023;
		*/
		//if(vp[l[j]][2] <= 0x0008) return;

		// mark as "destroy this primitive" early
		// saves on the number of checks we have to do
		if(vp[i][0] <= -1023) vp[i][2] = 1;
		else if(vp[i][0] >=  1023) vp[i][2] = 1;
		else if(vp[i][1] <= -1023) vp[i][2] = 1;
		else if(vp[i][1] >=  1023) vp[i][2] = 1;
	}

	// Draw mesh
	gpu_send_control_gp1(0x01000000);
	for(ic = 0, ii = 0; mesh->c[ic] != 0;
		ii += ((mesh->c[ic++] & 0x08000000) != 0 ? 4 : 3))
	{
		mesh_add_poly(mesh, vp, ic, ii, flags);
	}
}

