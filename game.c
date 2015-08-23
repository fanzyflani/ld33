// because if I make a file called game.c it should motivate me to actually make a game, right?

fixed player_x = 0;
fixed player_y = 0;
fixed player_ry = 0;

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

	// Clear screen
	//gpu_send_control_gp0((screen_buffer == 0 ? 0x027D7D7D : 0x024D4D4D));
	gpu_send_control_gp0(0x02001D00);
	gpu_send_data(0x00000000 + (screen_buffer<<16));
	gpu_send_data((320) | ((240)<<16));

	// Build matrices
	mat4_load_identity(&mat_cam);
	mat4_translate_imm3(&mat_cam, -player_x, 0x40000, -player_y + 0x40000);
	mat4_rotate_y(&mat_cam, player_ry);

	mat4_load_identity(&mat_icam);
	mat4_rotate_y(&mat_icam, -player_ry);

	// Draw geometry
	mesh_clear();
	mat4_load_identity(&mat_obj);
	mesh_draw(&poly_building);
	mat4_translate_imm3(&mat_obj, 0x50000, 0, 0);
	mesh_draw(&poly_building);
	mat4_translate_imm3(&mat_obj, -0x28000, 0, 0x30000);
	mesh_draw(&poly_building);
	mesh_flush();

	// Draw player
	/*
	gpu_send_control_gp0(0x285F5F3F);
	gpu_push_vertex(fixtoi(player_x)-7,fixtoi(player_y)-7);
	gpu_push_vertex(fixtoi(player_x)+7,fixtoi(player_y)-7);
	gpu_push_vertex(fixtoi(player_x)-7,fixtoi(player_y)+7);
	gpu_push_vertex(fixtoi(player_x)+7,fixtoi(player_y)+7);
	for(lag = 0; lag < 0x100; lag++) {}
	*/

	// Draw strings
	gpu_send_control_gp1(0x01000000);
	sprintf(update_str_buf, "ord=%02i row=%02i speed=%03i/%i"
		, s3mplayer.cord
		, s3mplayer.crow
		, s3mplayer.tempo
		, s3mplayer.speed
		);
	screen_print(16, 16+8*0, 0x7F7F7F, update_str_buf);
	screen_print(16, 16+8*1, 0x7F7F7F, s3mplayer.mod->name);

	sprintf(update_str_buf, "joypad=%04X", pad_data);
	screen_print(16, 16+8*3, 0x007F7F, update_str_buf);
	sprintf(update_str_buf, "vtime=%5i/314", TMR_n_COUNT(1));
	screen_print(16, 16+8*2, 0x7F7F7F, update_str_buf);

	if(pcsxr_detected)
		screen_print(150, 8, 0x4F4F7F, "PCSXR IS PRETTY BUGGY");

	//while((GP1 & 0x10000000) == 0) {}

	// Read joypad
	pad_old_data = pad_data;
	joy_readpad();

	// Apply input
	fixed mvspd = 1<<13;
	if((pad_data & PAD_L1) != 0)
		player_ry -= 1<<8;
	if((pad_data & PAD_R1) != 0)
		player_ry += 1<<8;
	if((pad_data & PAD_LEFT) != 0)
	{
		player_x -= fixmul(mat_icam[0][0], mvspd);
		player_y -= fixmul(mat_icam[0][2], mvspd);
	}
	if((pad_data & PAD_RIGHT) != 0)
	{
		player_x += fixmul(mat_icam[0][0], mvspd);
		player_y += fixmul(mat_icam[0][2], mvspd);
	}
	if((pad_data & PAD_UP) != 0)
	{
		player_x += fixmul(mat_icam[2][0], mvspd);
		player_y += fixmul(mat_icam[2][2], mvspd);
	}
	if((pad_data & PAD_DOWN) != 0)
	{
		player_x -= fixmul(mat_icam[2][0], mvspd);
		player_y -= fixmul(mat_icam[2][2], mvspd);
	}
}

void game_init(void)
{

}


