#define USE_DMA
#define USE_GTE_PERSP
// This requires USE_GTE_PERSP! (and is disabled because it's inconvenient, broken, and slower)
//define USE_GTE_RTPT

#ifdef USE_GTE_RTPT
#ifndef USE_GTE_PERSP
#error "USE_GTE_RTPT requires USE_GTE_PERSP"
#endif
#endif

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
	uint32_t cmd_list[4+4+4];
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

#define VTX_MAX (1000)
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

		pclist[ri].dma_chain = ((pclist[ri].len)<<24) | nptr;

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

static void mesh_add_poly(const mesh_s *mesh, int ic, int ii, int flags)
{
	int i, j;
	const uint16_t *l = &mesh->i[ii];

	// Get poly parameters
	int vcount = ((mesh->c[ic] & 0x08000000) != 0
		? 4
		: 3);
	int ccount = ((mesh->c[ic] & 0x10000000) != 0
		? vcount
		: 1);
	int tcount = ((mesh->c[ic] & 0x04000000) != 0
		? vcount
		: 0);

	int vstep = 1;
	if(ccount != 1) vstep++;
	int toffs = vstep;
	if(tcount != 0) vstep++;

	// Check facing + range
	for(j = 0; j < vcount; j++)
	{
		if(vbase[l[j]][2] <= 0x0008) return;
		/*
		if(vbase[l[j]][0] <= -1023) return;
		if(vbase[l[j]][0] >=  1023) return;
		if(vbase[l[j]][1] <= -1023) return;
		if(vbase[l[j]][1] >=  1023) return;
		*/
		/*
		if(vbase[l[j]][0] <= -160) return;
		if(vbase[l[j]][0] >=  160) return;
		if(vbase[l[j]][1] <= -120) return;
		if(vbase[l[j]][1] >=  120) return;
		*/
	}

	// Check direction (2D cross product)
	// Abuse top bit for bidirectional flag
	if((mesh->c[ic] & 0x80000000) == 0)
	{
		int dx0 = (vbase[l[1]][0] - vbase[l[0]][0]);
		int dy0 = (vbase[l[1]][1] - vbase[l[0]][1]);
		int dx1 = (vbase[l[2]][0] - vbase[l[0]][0]);
		int dy1 = (vbase[l[2]][1] - vbase[l[0]][1]);
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
		pc->len = ccount+vcount+tcount;
		//vec4_copy(&pc->fnorm, &fnorm);
		pc->cmd_list[0] = mesh->c[ic] & 0x7FFFFFFF;

		if(ccount != 1)
		{
			int cc = vstep;
			for(i = 1; i < vcount; i++)
			{
				pc->cmd_list[cc] = mesh->c[ic+i] & 0x7FFFFFFF;
				cc += vstep;
			}
		}

		if(tcount != 0)
		{
			for(i = 0; i < tcount; i++)
				pc->cmd_list[i*vstep+toffs] = mesh->c[ic+ccount+i];
		}

		if((flags & MS_SHADOW) != 0)
		{
			pc->cmd_list[0] |= 0x02000000;
			for(i = 0; i < ccount; i++)
				pc->cmd_list[i*vstep] &= 0xFF000000;
		}
		else if((flags & MS_FLASH) != 0)
		{
			for(i = 0; i < ccount; i++)
				pc->cmd_list[i*vstep] |= 0x00FFFFFF;
		}
		fixed zsum = 0;

		if((flags & MS_NOPRIO) == 0)
		{
			/*
			// broken, and oddly enough slower than what we have
			asm volatile(
				"\tmtc2 %1, $19\n"
				"\tmtc2 %2, $18\n"
				"\tmtc2 %3, $17\n"
				"\tcop2 0x158002D\n" // AVSZ3
				"\tmfc2 %0, $7\n"
				:
				"=r"(pcprio[pcidx])
				:
				"r"(vbase[l[0]][2]),
				"r"(vbase[l[1]][2]),
				"r"(vbase[l[2]][2])
				:);
			*/
			int cc = 1;
			for(i = 0; i < vcount; i++)
			{
				zsum += vbase[l[i]][2];

				pc->cmd_list[cc] = (
					(vbase[l[i]][0]&0xFFFF) +
					(vbase[l[i]][1]<<16));
				cc += vstep;
			}

			pcprio[pcidx] = zsum/vcount;
		} else {
			int cc = 1;
			for(i = 0; i < vcount; i++)
			{
				pc->cmd_list[cc] = (
					(vbase[l[i]][0]&0xFFFF) +
					(vbase[l[i]][1]<<16));
				cc += vstep;
			}

			pcprio[pcidx] = 0;
		}
	}
}

static void mesh_draw(const mesh_s *mesh, int flags)
{
	volatile int lag;
	int i, j;

	// Set texpage + mask
	gpu_send_control_gp0(0xE1000708);
	gpu_draw_texmask(32, 32, 0, 64);

	// Combine matrices
	//mat4_mul_mat4_mat4(&mat_obj_cam, &mat_cam, &mat_obj);
	mat4_mul_mat4_mat4(&mat_obj_cam, &mat_obj, &mat_cam);
	//mat4_scale(&mat_obj_cam, 1<<(16+4));
	// TODO: find appropriate scale factor for GTE

	// Build points
	vec4 *v = vbase;
	int iv, ii, ic;

	// Load rotation matrix
	asm volatile (
		"\tctc2 %0, $0\n"
		"\tctc2 %1, $1\n"
		"\tctc2 %2, $2\n"
		"\tctc2 %3, $3\n"
		"\tctc2 %4, $4\n"
		: :
		"r"(((mat_obj_cam[0][0]>>4)&0xFFFF)|((mat_obj_cam[1][0]>>4)<<16)),
		"r"(((mat_obj_cam[2][0]>>4)&0xFFFF)|((mat_obj_cam[0][1]>>4)<<16)),
		"r"(((mat_obj_cam[1][1]>>4)&0xFFFF)|((mat_obj_cam[2][1]>>4)<<16)),
		"r"(((mat_obj_cam[0][2]>>4)&0xFFFF)|((mat_obj_cam[1][2]>>4)<<16)),
		"r"(mat_obj_cam[2][2]>>4)
		: );
	
	// Load translation vector
	// TODO: fix wrapping issues
	const int gte_loss_invec = 9;
	const int gte_tshift = gte_loss_invec;
	fixed tx_prep = mat_obj_cam[3][0]>>gte_tshift;
	fixed ty_prep = mat_obj_cam[3][1]>>gte_tshift;
	fixed tz_prep = mat_obj_cam[3][2]>>gte_tshift;
	asm volatile (
		"\tctc2 %0, $5\n"
		"\tctc2 %1, $6\n"
		"\tctc2 %2, $7\n"
		: :
		"r"(tx_prep),
		"r"(ty_prep),
		"r"(tz_prep)
		: );

	// Load screen offset and whatnot
	// TODO: only do this once per frame
	asm volatile (
		"\tctc2 %0, $24\n"
		"\tctc2 %1, $25\n"
		"\tctc2 %2, $26\n"
		"\tctc2 %3, $29\n"
		"\tctc2 %4, $30\n"
		: :
		"r"(0), // OFX
		"r"(0), // OFY
		"r"(120), // H
		"r"(0x1000/3), // ZSF3
		"r"(0x1000/4) // ZSF4
		: );

	// Apply transformation
#ifdef USE_GTE_RTPT
	const int gte_stride = 3;
#else
	const int gte_stride = 1;
#endif
	for(i = 0; i < (int)mesh->vc && i < VTX_MAX; i += gte_stride)
	{
		// Load vec3
		asm volatile (
			"\tmtc2 %0, $0\n"
			"\tmtc2 %1, $1\n"
#ifdef USE_GTE_RTPT
			"\tmtc2 %2, $2\n"
			"\tmtc2 %3, $3\n"
			"\tmtc2 %4, $4\n"
			"\tmtc2 %5, $5\n"
#endif
			: :
			"r"(((mesh->v[i][0]>>gte_loss_invec)&0xFFFF)
			|((mesh->v[i][1]>>gte_loss_invec)<<16)),
			"r"(mesh->v[i][2]>>gte_loss_invec)
#ifdef USE_GTE_RTPT
			,
			"r"(((mesh->v[i+1][0]>>gte_loss_invec)&0xFFFF)
			|((mesh->v[i+1][1]>>gte_loss_invec)<<16)),
			"r"(mesh->v[i+1][2]>>gte_loss_invec),
			"r"(((mesh->v[i+2][0]>>gte_loss_invec)&0xFFFF)
			|((mesh->v[i+2][1]>>gte_loss_invec)<<16)),
			"r"(mesh->v[i+2][2]>>gte_loss_invec)
#endif
			: );

		//memcpy(&v[i], &mesh->v[i], sizeof(fixed)*3);
		//v[i][3] = 0x10000;

		// RTPS
#ifdef USE_GTE_RTPT
		asm volatile ("\tcop2 0x0180001\n");
#else
		asm volatile ("\tcop2 0x0180001\n");
#endif

		// Stash result
#ifdef USE_GTE_RTPT
		fixed res0_xy, res0_z;
		fixed res1_xy, res1_z;
		fixed res2_xy, res2_z;
#else
#ifdef USE_GTE_PERSP
		fixed res0_xy, res0_z;
#else
		fixed res0_ix, res0_iy, res0_iz;
#endif
#endif


#ifdef USE_GTE_RTPT
		asm volatile(
			"\tmfc2 %0, $14\n"
			"\tmfc2 %1, $19\n"
			"\tmfc2 %2, $14\n"
			"\tmfc2 %3, $19\n"
			"\tmfc2 %4, $14\n"
			"\tmfc2 %5, $19\n"
			:
			"=r"(res0_xy),
			"=r"(res0_z),
			"=r"(res1_xy),
			"=r"(res1_z),
			"=r"(res2_xy),
			"=r"(res2_z)
			::);
#else

#ifdef USE_GTE_PERSP
		asm volatile(
			"\tmfc2 %0, $14\n"
			"\tmfc2 %1, $19\n"
			:
			"=r"(res0_xy),
			"=r"(res0_z)
			::);
#else
		asm volatile(
			"\tmfc2 %0, $9\n"
			"\tmfc2 %1, $10\n"
			"\tmfc2 %2, $11\n"
			:
			"=r"(res0_ix),
			"=r"(res0_iy),
			"=r"(res0_iz)
			::);
#endif

#endif

		//mat4_apply_vec4(&v[i], &mat_obj_cam);
		//mat4_apply_vec4(&v[i], &mat_obj);
		//mat4_apply_vec4(&v[i], &mat_cam);

		// Project points
#ifdef USE_GTE_RTPT
		vbase[i+0][0] = (res0_xy<<16)>>16;
		vbase[i+0][1] = (res0_xy>>16);
		vbase[i+0][2] = res0_z;
		vbase[i+1][0] = (res1_xy<<16)>>16;
		vbase[i+1][1] = (res1_xy>>16);
		vbase[i+1][2] = res1_z;
		vbase[i+2][0] = (res2_xy<<16)>>16;
		vbase[i+2][1] = (res2_xy>>16);
		vbase[i+2][2] = res2_z;
#else
#ifdef USE_GTE_PERSP
		vbase[i][0] = (res0_xy<<16)>>16;
		vbase[i][1] = (res0_xy>>16);
		vbase[i][2] = res0_z;
#else
		fixed z = res0_iz;
		z = (z > 1 ? z : 1);
		vbase[i][0] = res0_ix*120/z;
		vbase[i][1] = res0_iy*120/z;
		vbase[i][2] = z;
#endif
#endif
		/*
		if(vbase[i][0] < -1023) vbase[i][0] = -1023;
		if(vbase[i][0] >  1023) vbase[i][0] =  1023;
		if(vbase[i][1] < -1023) vbase[i][1] = -1023;
		if(vbase[i][1] >  1023) vbase[i][1] =  1023;
		*/
		//if(vbase[l[j]][2] <= 0x0008) return;

		// mark as "destroy this primitive" early
		// saves on the number of checks we have to do
		// TODO: get the RTPT version (may be unobtainable)
#ifdef USE_GTE_RTPT
#else
		uint32_t gte_flag;
		asm volatile("\tcfc2 %0, $31\n" : "=r"(gte_flag) : : );
		if((gte_flag & 0x00006000) != 0)
			vbase[i][2] = 1;
		/*
		if(vbase[i][0] <= -1023) vbase[i][2] = 1;
		else if(vbase[i][0] >=  1023) vbase[i][2] = 1;
		else if(vbase[i][1] <= -1023) vbase[i][2] = 1;
		else if(vbase[i][1] >=  1023) vbase[i][2] = 1;
		*/
#endif
	}

	// Draw mesh
	gpu_send_control_gp1(0x01000000);
	for(ic = 0, ii = 0; mesh->c[ic] != 0;
		)
	{
		mesh_add_poly(mesh, ic, ii, flags);
		int vcount = ((mesh->c[ic] & 0x08000000) != 0 ? 4 : 3);
		int ccount = ((mesh->c[ic] & 0x10000000) != 0 ? vcount : 1);
		int tcount = ((mesh->c[ic] & 0x04000000) != 0 ? vcount : 0);
		ii += vcount;
		ic += ccount+tcount;
	}
}

