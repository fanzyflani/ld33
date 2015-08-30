#define HMAP_POW 7
#define HMAP_L (1<<(HMAP_POW))
#define VISRANGE 5
static fixed hmap[HMAP_L][HMAP_L];
static uint32_t hmap_c[HMAP_L][HMAP_L];

static int hmap_visx = 0;
static int hmap_visz = 0;

static fixed hmap_get(fixed x, fixed z)
{
	fixed hm00 = hmap[((z>>18)+0)&(HMAP_L-1)][((x>>18)+0)&(HMAP_L-1)];
	fixed hm01 = hmap[((z>>18)+1)&(HMAP_L-1)][((x>>18)+0)&(HMAP_L-1)];
	fixed hm10 = hmap[((z>>18)+0)&(HMAP_L-1)][((x>>18)+1)&(HMAP_L-1)];
	fixed hm11 = hmap[((z>>18)+1)&(HMAP_L-1)][((x>>18)+1)&(HMAP_L-1)];
	fixed hmintx0 = ((hm00<<10) + ((hm10 - hm00)*((x&0x3FFFF)>>8)))>>10;
	fixed hmintx1 = ((hm01<<10) + ((hm11 - hm01)*((x&0x3FFFF)>>8)))>>10;
	fixed hmint = ((hmintx0<<10) + ((hmintx1 - hmintx0)*((z&0x3FFFF)>>8)))>>10;

	return hmint;
}

static void hmap_gen(void)
{
	int x, z, i;
	int bx, bz;

	// Clear heightmap
	for(z = 0; z < HMAP_L; z++)
	for(x = 0; x < HMAP_L; x++)
		hmap[z][x] = 0;

	// Create plasma noise
	fixed amp;
	hmap[0][0] = 0;
	for(i = HMAP_POW-1, amp = 0x70000; i >= 0; i--, amp = (amp*0xC0)>>8)
	{
		int step = (1<<i);

		for(bz = 0; bz < HMAP_L; bz += (step<<1))
		for(bx = 0; bx < HMAP_L; bx += (step<<1))
		{
			int x0 = bx;
			int z0 = bz;
			int x1 = (bx+step)&(HMAP_L-1);
			int z1 = (bz+step)&(HMAP_L-1);
			int x2 = (bx+(step<<1))&(HMAP_L-1);
			int z2 = (bz+(step<<1))&(HMAP_L-1);

			// Square
			hmap[z1][x0] = ((hmap[z2][x0] + hmap[z0][x0])>>1) + fixmulf(amp, fixrand1s());
			hmap[z0][x1] = ((hmap[z0][x2] + hmap[z0][x0])>>1) + fixmulf(amp, fixrand1s());
			hmap[z1][x2] = ((hmap[z0][x2] + hmap[z2][x2])>>1) + fixmulf(amp, fixrand1s());
			hmap[z2][x1] = ((hmap[z2][x0] + hmap[z2][x2])>>1) + fixmulf(amp, fixrand1s());

			// Diamond
			hmap[z1][x1] =
				((hmap[z1][x0] + hmap[z0][x1] + hmap[z1][x2] + hmap[z2][x1])>>2)
				+ fixmulf(amp, fixrand1s());
		}
	}

	// Calculate ambient occlusion
	for(z = 0; z < HMAP_L; z++)
	for(x = 0; x < HMAP_L; x++)
	{
		fixed ynx = hmap[z][(x-1)&(HMAP_L-1)];
		fixed ynz = hmap[(z-1)&(HMAP_L-1)][x];
		fixed ypx = hmap[z][(x+1)&(HMAP_L-1)];
		fixed ypz = hmap[(z+1)&(HMAP_L-1)][x];
		fixed y00 = hmap[z][x];
		fixed ysm = (ynx+ynz+ypx+ypz+2)>>2;
		fixed col = ysm-y00;
		col >>= 10;
		col += 0x40;
		if(col < 0x00) col = 0x00;
		if(col > 0xDF) col = 0xDF;

		hmap_c[z][x] = (((uint32_t)col)<<8) | (0x34<<24);
	}

	// Add heightmap as texture
	gpu_send_control_gp0(0xA0000000);
	gpu_send_data((512)|(64<<16));
	gpu_send_data((32)|(32<<16));
	for(z = 0; z < 32; z++)
	for(x = 0; x < 32; x+=2)
	{
		uint32_t c0 = (hmap[z<<(HMAP_POW-5)][(x+0)<<(HMAP_POW-5)]);
		uint32_t c1 = (hmap[z<<(HMAP_POW-5)][(x+1)<<(HMAP_POW-5)]);
		c0 = (fixsin(c0>>5)+0x10000)>>(17-5);
		c1 = (fixsin(c1>>5)+0x10000)>>(17-5);
		c0 >>= 1; c0 += 16;
		c1 >>= 1; c1 += 16;
		c0 *= 0x421;
		c1 *= 0x421;
		gpu_push_vertex(c0, c1);
	}

	// Place hotspots
	bx = 0;
	bz = 0;
	for(z = -2; z <= 5; z++)
	for(x = -2; x <= 2; x++)
		hmap[(bz+z)&(HMAP_L-1)][(bx+x)&(HMAP_L-1)] = 0x10000;
}

