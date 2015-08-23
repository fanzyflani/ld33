// because if I make a file called game.c it should motivate me to actually make a game, right?

vec4 player_pos = {0, 0, 0, 0x10000};
fixed player_ry = 0;
fixed player_rx = 0;
fixed player_tilt_x = 0;
fixed player_tilt_y = 0;

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

	// buildings
	mat4_load_identity(&mat_obj);
	mat4_translate_imm3(&mat_obj, 0, 0, 0x180000);
	mesh_draw(&poly_building);

	mat4_load_identity(&mat_obj);
	mat4_translate_imm3(&mat_obj, 0x50000, 0, 0x100000);
	mesh_draw(&poly_building);

	mat4_load_identity(&mat_obj);
	mat4_translate_imm3(&mat_obj, -0x28000, 0, 0x130000);
	mesh_draw(&poly_tree1);

	mat4_load_identity(&mat_obj);
	mat4_translate_imm3(&mat_obj, 0x18000, -0x20000, 0x150000);
	mesh_draw(&poly_ship1);

	// player
	//mesh_flush();
	mat4_load_identity(&mat_obj);
	mat4_rotate_z(&mat_obj, player_tilt_y);
	mat4_rotate_x(&mat_obj, -player_rx);
	mat4_rotate_y(&mat_obj, -player_ry);
	mat4_translate_vec4(&mat_obj, &player_pos);
	mesh_draw(&poly_ship1);

	// finish drawing
	mesh_flush();

	// Draw strings
	gpu_send_control_gp1(0x01000000);
	sprintf(update_str_buf, "joypad=%04X", pad_data);
	screen_print(16, 16+8*1, 0x007F7F, update_str_buf);
	sprintf(update_str_buf, "vtime=%5i/314", TMR_n_COUNT(1));
	screen_print(16, 16+8*2, 0x7F7F7F, update_str_buf);

	if(pcsxr_detected)
		screen_print(150, 8, 0x4F4F7F, "PCSXR IS PRETTY BUGGY");

	//while((GP1 & 0x10000000) == 0) {}

	// Read joypad
	pad_old_data = pad_data;
	joy_readpad();

	// Apply input
	fixed mvspd_x = 1<<12;
	fixed mvspd = 1<<12;
	fixed applied_rx = 0;
	fixed applied_ry = 0;

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

}


