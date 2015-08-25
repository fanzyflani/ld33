// because if I make a file called game.c it should motivate me to actually make a game, right?

#define SHOW_DEBUG

void game_init(void);

static void game_update_frame(void)
{
	volatile int lag;
	int x, y, z, i, j;
	uint8_t update_str_buf[64];

	// Flip pages
	gpu_display_start(0, screen_buffer+1);
	screen_buffer = (screen_buffer == 0 ? 240 : 0);
	gpu_draw_range(0, screen_buffer, 320, 240 + screen_buffer);
	gpu_draw_offset(0 + 160, screen_buffer + 120);
	mesh_tstat = 0;

	// Build matrices
	mat4_load_identity(&mat_cam);
	mat4_translate_vec4_neg(&mat_cam, &player->pos);
	mat4_rotate_y(&mat_cam, player->ry);
	//mat4_rotate_x(&mat_cam, player->rx);
	mat4_translate_imm3(&mat_cam, 0, 0, 0x40000);
	mat4_translate_imm3(&mat_cam, 0, 0x10000, 0);

	mat4_load_identity(&mat_icam);
	//mat4_rotate_x(&mat_icam, -player->rx);
	mat4_rotate_y(&mat_icam, -player->ry);

	// Calculate sky
	int sky = 40;//((fixsin(-player->rx)*120)>>16);
	int skyswap = 0;//(fixcos(-player->rx) <= 0);
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
	static uint16_t mworld_i[6*(VISRANGE*2+1)*(VISRANGE*2+1)];
	static uint32_t mworld_c[2*(VISRANGE*2+1)*(VISRANGE*2+1)+1];
	mesh_s mworld = {
		4,
		(const vec3 *)mworld_v,
		mworld_i,
		mworld_c,
	};

	// TODO: correct ordering so we don't need to qsort this
	mat4_load_identity(&mat_obj);
#if 1
	fixed hdist = 0xA0000;
	int xoffs = ((player->pos[0] + fixmul(hdist, mat_icam[2][0]))>>18)-VISRANGE;
	int zoffs = ((player->pos[2] + fixmul(hdist, mat_icam[2][2]))>>18)-VISRANGE;
	hmap_visx = xoffs+VISRANGE;
	hmap_visz = zoffs+VISRANGE;
	mat4_translate_imm3(&mat_obj, xoffs<<18, 0, zoffs<<18);
	// translate so the GTE doesn't break
	for(x = 0, i = 0; x < VISRANGE*2+2; x++)
	for(z = 0; z < VISRANGE*2+2; z++, i++)
	{
		int x0r = (x+0);
		int z0r = (z+0);
		int x0 = (xoffs+x+0) & (HMAP_L-1);
		int z0 = (zoffs+z+0) & (HMAP_L-1);
		fixed y00 = hmap[z0][x0];
		vec3_set(&mworld_v[i], x0r<<18, y00, z0r<<18);
	}
	mworld.vc = i;

	for(x = 0, i = 0; x < VISRANGE*2+1; x++)
	for(z = 0; z < VISRANGE*2+1; z++, i++)
	{
		int iv = i*6;
		// Generate faces
		// TODO: order properly and allow splits
		mworld_i[iv+0] = (z+0)+(x+0)*(VISRANGE*2+2);
		//mworld_i[iv+1] = (z+1)+(x+0)*(VISRANGE*2+2);
		//mworld_i[iv+2] = (z+0)+(x+1)*(VISRANGE*2+2);
		//mworld_i[iv+5] = (z+1)+(x+1)*(VISRANGE*2+2);
		mworld_i[iv+1] = mworld_i[iv+0]+1;
		mworld_i[iv+2] = mworld_i[iv+0]+(VISRANGE*2+2);
		mworld_i[iv+5] = mworld_i[iv+2]+1;
		mworld_i[iv+4] = mworld_i[iv+1];
		mworld_i[iv+3] = mworld_i[iv+2];

		// would need gouraud shading for this to look any good
		/*
		fixed y00 = mworld_v[mworld_i[i*4+0]][1];
		fixed y01 = mworld_v[mworld_i[i*4+1]][1];
		fixed y10 = mworld_v[mworld_i[i*4+2]][1];
		fixed y11 = mworld_v[mworld_i[i*4+3]][1];

		// Do a cross product and normalise
		fixed yd0 = y01-y00;
		fixed yd1 = y10-y00;
		const fixed ydz2 = (1<<16);
		fixed ydlen = fixsqrt(fixmulf(yd0, yd0) + fixmulf(yd1, yd1) + ydz2);
		fixed ydilen = fixdiv(0x4000, ydlen);
		if(ydilen > 0x10000) ydilen = 0x10000;
		uint32_t col = (ydilen * 0xFF)>>16;
		col <<= 8;
		mworld_c[i] = 0xA8000000 | col;
		*/

		mworld_c[2*i+0] = mworld_c[2*i+1] =
			(((x^z^xoffs^zoffs)&1) == 0 ? 0x20003F00 : 0x20005F00);
	}
	mworld_c[2*i] = 0;
	mesh_draw(&mworld, MS_NOPRIO);
	mesh_flush(0); // faster with sort disabled, and negligible graphical effect
	//mesh_clear();
#endif

	// Shadows
	for(i = 0; i < jet_count; i++)
		jet_draw(&jet_list[i], 1);

	mesh_flush(1);

	// SHOTS SHOTS SHOTS SHOTS SHOTS SHOTS SHOTS SHOTS EVERYBODY
	shot_draw();
	mesh_flush(0);

	// Buildings
	for(i = 0; i < bldg_count; i++)
		bldg_draw(&bldg_list[i]);

	// Jets
	for(i = 0; i < jet_count; i++)
		jet_draw(&jet_list[i], 0);

	// Finish drawing
	mesh_flush(1);

	// Draw strings
	gpu_send_control_gp1(0x01000000);
	sprintf(update_str_buf, "Enemies: %i/%i", jet_enemy_rem, jet_enemy_max);
	screen_print(16, 16+8*0, 0x7F7F7F, update_str_buf);
	/*
	sprintf(update_str_buf, "joypad=%04X", pad_data);
	screen_print(16, 16+8*1, 0x007F7F, update_str_buf);
	*/
#ifdef SHOW_DEBUG
	sprintf(update_str_buf, "vtime=%5i/314", TMR_n_COUNT(1));
	screen_print(16, 16+8*2, 0x7F7F7F, update_str_buf);
	sprintf(update_str_buf, "tris=%5i", mesh_tstat);
	screen_print(16, 16+8*4, 0x7F7F7F, update_str_buf);
#endif
	//sprintf(update_str_buf, "pc=%i %p %p %p", pclist_max, pclist, pcorder, pcprio);
	//screen_print(16, 16+8*3, 0x7F7F7F, update_str_buf);
	//sprintf(update_str_buf, "shot=%4i %4i", shot_head, shot_tail);
	//screen_print(16, 16+8*4, 0x7F7F7F, update_str_buf);
	//sprintf(update_str_buf, "halp=%08X", fixrand1s());
	//screen_print(16, 16+8*5, 0x7F7F7F, update_str_buf);
	screen_print(12, 240-8-20, 0x7F7F7F, "LIFE");

	const int bar_width = 320-(12+8*4+4)-16;
	int bar_filled = (player->health * bar_width/50 + 8)&~0xF;
	gpu_send_control_gp0(0x0200007F);
	gpu_push_vertex(12+8*4+4, 240-8-20 + screen_buffer);
	gpu_push_vertex(bar_width, 8);
	if(bar_filled > 0)
	{
		gpu_send_control_gp0(0x02007F00);
		gpu_push_vertex(12+8*4+4, 240-8-20 + screen_buffer);
		gpu_push_vertex(bar_filled, 8);
	}

	if(pcsxr_detected)
		screen_print(150, 8, 0x4F4F7F, "PCSXR IS PRETTY BUGGY");

	if(player->crashed > 0)
	{
		const char *msg = "PRESS START TO RETRY";
		screen_print(160-strlen(msg)*4, 120-4, 0x7F7F7F, msg);
	}

	//while((GP1 & 0x10000000) == 0) {}

	// Read joypad
	pad_old_data = pad_data;
	joy_readpad();


	// Update jets
	for(i = 0; i < jet_count; i++)
		jet_update(&jet_list[i]);

	// Update shots
	shot_update();

	// Wrap player pos
	player->pos[0] += (1<<(18+HMAP_POW-1));
	player->pos[2] += (1<<(18+HMAP_POW-1));
	player->pos[0] &= ((1<<(18+HMAP_POW))-1);
	player->pos[2] &= ((1<<(18+HMAP_POW))-1);
	player->pos[0] -= (1<<(18+HMAP_POW-1));
	player->pos[2] -= (1<<(18+HMAP_POW-1));

	// If no more enemies, and player not dead, advance level
	if(jet_enemy_rem <= 0 && player->health > 0)
	{
		jet_level++;
		game_init();
	}

	// Final vtime
#ifdef SHOW_DEBUG
	sprintf(update_str_buf, "ltime=%5i/314", TMR_n_COUNT(1));
	screen_print(16, 16+8*3, 0x7F7F7F, update_str_buf);
#endif

}

void game_init(void)
{
	int i;

	randseed = jet_seeds[jet_level];
	hmap_gen();

	jet_count = 0;
	jet_enemy_max = 0;
	jet_enemy_rem = 0;
	player = &jet_list[jet_add(0, 0, 0, 50, 1, JAI_PLAYER)];

	for(i = 0; i < jet_level; i++)
	{
		fixed xspawn = fixrand1s()*40;
		fixed zspawn = fixrand1s()*40;
		jet_add(xspawn, hmap_get(xspawn, zspawn)-0x50000, zspawn, 50, 2, JAI_HUNT_MAIN);
	}

	bldg_count = 0;
	for(i = 0; i < 25; i++)
	{
		fixed xspawn = fixrand1s()*40;
		fixed zspawn = fixrand1s()*40;
		fixrand1s();
		bldg_add(xspawn, zspawn,
			fixrand1s() > 0
				? &poly_building
				: &poly_tree1
			);

	}
}


