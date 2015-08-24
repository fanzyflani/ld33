// because if I make a file called game.c it should motivate me to actually make a game, right?

vec4 player_pos = {0, 0, 0, 0x10000};
fixed player_ry = 0;
fixed player_rx = 0;
fixed player_tilt_y = 0;
fixed player_tspd = 1<<13;

#define HMAP_POW 7
#define HMAP_L (1<<(HMAP_POW))
#define VISRANGE 4
static fixed hmap[HMAP_L][HMAP_L];

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

static void game_update_frame(void)
{
	volatile int lag;
	int x, y, z, i, j;
	uint8_t update_str_buf[64];

	// Flip pages
	gpu_display_start(0, screen_buffer + 8);
	screen_buffer = (screen_buffer == 0 ? 240 : 0);
	gpu_draw_range(0, screen_buffer, 320, 240 + screen_buffer);
	gpu_draw_offset(0 + 160, screen_buffer + 120);

	// Build matrices
	mat4_load_identity(&mat_cam);
	mat4_translate_vec4_neg(&mat_cam, &player_pos);
	mat4_rotate_y(&mat_cam, player_ry);
	//mat4_rotate_x(&mat_cam, player_rx);
	mat4_translate_imm3(&mat_cam, 0, 0, 0x40000);
	mat4_translate_imm3(&mat_cam, 0, 0x10000, 0);

	mat4_load_identity(&mat_icam);
	//mat4_rotate_x(&mat_icam, -player_rx);
	mat4_rotate_y(&mat_icam, -player_ry);
	mat4_load_identity(&mat_iplr);
	mat4_rotate_x(&mat_iplr, -player_rx);
	mat4_rotate_y(&mat_iplr, -player_ry);

	// Calculate sky
	int sky = 0;//((fixsin(-player_rx)*120)>>16);
	int skyswap = 0;//(fixcos(-player_rx) <= 0);
	if(skyswap)
		sky = -sky;

	sky += 120;

	if(sky < 0) sky = 0;
	if(sky > 240) sky = 240;

	// Clear screen
	//gpu_send_control_gp0((screen_buffer == 0 ? 0x027D7D7D : 0x024D4D4D));
	if(sky < 240)
	{
		gpu_send_control_gp0((skyswap ? 0x021D0000 : 0x02001D00));
		gpu_send_data(0x00000000 + (screen_buffer+sky<<16));
		gpu_send_data((320) | ((240-sky)<<16));
	}

	if(sky > 0)
	{
		gpu_send_control_gp0((!skyswap ? 0x021D0000 : 0x02001D00));
		gpu_send_data(0x00000000 + (screen_buffer<<16));
		gpu_send_data((320) | ((sky)<<16));
	}

	// Draw geometry
	mesh_clear();

	// world
	static vec3 mworld_v[(VISRANGE*2+2)*(VISRANGE*2+2)];
	static uint16_t mworld_i[4*(VISRANGE*2+1)*(VISRANGE*2+1)];
	static uint32_t mworld_c[1*(VISRANGE*2+1)*(VISRANGE*2+1)+1];
	mesh_s mworld = {
		4,
		(const vec3 *)mworld_v,
		mworld_i,
		mworld_c,
	};

	// TODO: correct ordering so we don't need to qsort this
	mat4_load_identity(&mat_obj);
#if 1
	fixed hdist = 0x80000;
	int xoffs = ((player_pos[0] + fixmul(hdist, mat_icam[2][0]))>>18)-VISRANGE;
	int zoffs = ((player_pos[2] + fixmul(hdist, mat_icam[2][2]))>>18)-VISRANGE;
	for(x = 0, i = 0; x < VISRANGE*2+2; x++)
	for(z = 0; z < VISRANGE*2+2; z++, i++)
	{
		int x0r = (xoffs+x+0);
		int z0r = (zoffs+z+0);
		int x0 = (xoffs+x+0) & (HMAP_L-1);
		int z0 = (zoffs+z+0) & (HMAP_L-1);
		fixed y00 = hmap[z0][x0];
		vec3_set(&mworld_v[i], x0r<<18, y00, z0r<<18);
	}
	mworld.vc = i;

	for(x = 0, i = 0; x < VISRANGE*2+1; x++)
	for(z = 0; z < VISRANGE*2+1; z++, i++)
	{
		// Generate faces
		// TODO: order properly and allow splits
		mworld_i[i*4+0] = (z+0)+(x+0)*10;
		mworld_i[i*4+1] = (z+1)+(x+0)*10;
		mworld_i[i*4+2] = (z+0)+(x+1)*10;
		mworld_i[i*4+3] = (z+1)+(x+1)*10;
		mworld_c[i] = (((x^z^xoffs^zoffs)&1) == 0 ? 0xA8003F00 : 0xA8005F00);
	}
	mworld_c[i] = 0;
	mesh_draw(&mworld, 0);
	mesh_flush(1);
	//mesh_clear();
#endif

	// buildings
	mat4_load_identity(&mat_obj);
	mat4_translate_imm3(&mat_obj, 0, 0, 0x180000);
	mesh_draw(&poly_building, 0);

	mat4_load_identity(&mat_obj);
	mat4_translate_imm3(&mat_obj, 0x50000, 0, 0x100000);
	mesh_draw(&poly_building, 0);

	mat4_load_identity(&mat_obj);
	mat4_translate_imm3(&mat_obj, -0x28000, 0, 0x130000);
	mesh_draw(&poly_tree1, 0);

	mat4_load_identity(&mat_obj);
	mat4_translate_imm3(&mat_obj, 0x18000, -0x20000, 0x150000);
	mesh_draw(&poly_ship1, 0);

	// player
	//mesh_flush();
	mat4_load_identity(&mat_obj);
	mat4_rotate_z(&mat_obj, player_tilt_y);
	mat4_rotate_x(&mat_obj, -player_rx);
	mat4_rotate_y(&mat_obj, -player_ry);
	mat4_translate_vec4(&mat_obj, &player_pos);
	mesh_draw(&poly_ship1, 0);

	mat4_translate_imm3(&mat_obj,
		0, hmap_get(player_pos[0], player_pos[2]) - player_pos[1], 0);
	mesh_draw(&poly_ship1, MS_SHADOW);

	// finish drawing
	mesh_flush(1);

	// Draw strings
	gpu_send_control_gp1(0x01000000);
	sprintf(update_str_buf, "joypad=%04X", pad_data);
	screen_print(16, 16+8*1, 0x007F7F, update_str_buf);
	sprintf(update_str_buf, "vtime=%5i/314", TMR_n_COUNT(1));
	screen_print(16, 16+8*2, 0x7F7F7F, update_str_buf);
	//sprintf(update_str_buf, "pc=%i %p %p %p", pclist_max, pclist, pcorder, pcprio);
	//screen_print(16, 16+8*3, 0x7F7F7F, update_str_buf);
	sprintf(update_str_buf, "pp=%08X %08X", player_pos[0], player_pos[2]);
	screen_print(16, 16+8*4, 0x7F7F7F, update_str_buf);
	//sprintf(update_str_buf, "halp=%08X", fixrand1s());
	//screen_print(16, 16+8*5, 0x7F7F7F, update_str_buf);

	if(pcsxr_detected)
		screen_print(150, 8, 0x4F4F7F, "PCSXR IS PRETTY BUGGY");

	//while((GP1 & 0x10000000) == 0) {}

	// Read joypad
	pad_old_data = pad_data;
	joy_readpad();

	// Apply input
	fixed mvspd_x = 1<<13;
	fixed applied_rx = 0;
	fixed applied_ry = 0;
	fixed applied_tspd = 1<<13;

	if((pad_data & PAD_X) != 0)
		applied_tspd = 1<<15;
	
	player_tspd += (applied_tspd - player_tspd)>>4;
	fixed mvspd = player_tspd;

	if((pad_data & PAD_LEFT) != 0)
		applied_ry -= 1;
	if((pad_data & PAD_RIGHT) != 0)
		applied_ry += 1;
	if((pad_data & PAD_UP) != 0)
		applied_rx -= 1;
	if((pad_data & PAD_DOWN) != 0)
		applied_rx += 1;
	
	player_rx += applied_rx<<9;
	player_ry += applied_ry<<9;
	player_tilt_y += ((applied_ry*0x3000)-player_tilt_y)>>4;

	if((pad_data & PAD_L1) != 0)
	{
		player_pos[0] -= fixmul(mat_iplr[0][0], mvspd_x);
		player_pos[1] -= fixmul(mat_iplr[0][1], mvspd_x);
		player_pos[2] -= fixmul(mat_iplr[0][2], mvspd_x);
	}
	if((pad_data & PAD_R1) != 0)
	{
		player_pos[0] += fixmul(mat_iplr[0][0], mvspd_x);
		player_pos[1] += fixmul(mat_iplr[0][1], mvspd_x);
		player_pos[2] += fixmul(mat_iplr[0][2], mvspd_x);
	}
	player_pos[0] += fixmul(mat_iplr[2][0], mvspd);
	player_pos[1] += fixmul(mat_iplr[2][1], mvspd);
	player_pos[2] += fixmul(mat_iplr[2][2], mvspd);
}

void game_init(void)
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

	// Place hotspots
	bx = 0;
	bz = 0;
	for(z = -2; z <= 5; z++)
	for(x = -2; x <= 2; x++)
		hmap[(bz+z)&(HMAP_L-1)][(bx+x)&(HMAP_L-1)] = 0x10000;
}


