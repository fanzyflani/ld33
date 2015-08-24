int jet_count = 3;
jet_s jet_list[3] = {
	{
		{0, 0, 0, 0x10000},
		0, 0,
		0,
		1<<13,

		200, 2,
	},
	{
		{0x18000, -0x60000, 0x150000, 0x10000},
		0, 0,
		0,
		1<<13,

		50, 2,
	},
	{
		{0x18000, -0x60000, 0x190000, 0x10000},
		0, 0,
		0,
		1<<13,

		50, 2,
	},
};

jet_s *player = &jet_list[0];

static void jet_draw(jet_s *jet, int is_shadow)
{
	// Check if in view dist
	int hx = (jet->pos[0]>>18);
	int hz = (jet->pos[2]>>18);
	hx -= hmap_visx;
	hz -= hmap_visz;
	if(hx < 0) hx = -hx;
	if(hz < 0) hz = -hz;

	if(hx > VISRANGE || hz > VISRANGE) return;

	mat4_load_identity(&mat_obj);
	mat4_rotate_z(&mat_obj, jet->tilt_y);
	mat4_rotate_x(&mat_obj, -jet->rx);
	mat4_rotate_y(&mat_obj, -jet->ry);
	mat4_translate_vec4(&mat_obj, &jet->pos);

	if(is_shadow)
	{
		mat4_translate_imm3(&mat_obj,
			0, hmap_get(jet->pos[0], jet->pos[2]) - jet->pos[1], 0);
		mesh_draw(&poly_jet1, MS_SHADOW);
	} else {
		mesh_draw(&poly_jet1, (
			//jet->crashed ? MS_SHADOW :
			jet->hit_flash > 0 ? MS_FLASH :
			0));
	}

}

static void jet_update(jet_s *jet,
	fixed applied_tspd, fixed applied_rx, fixed applied_ry,
	fixed applied_vx)
{
	mat4 jmat;

	if(jet->crashed > 0)
	{
		// TODO: release smoke
		return;
	}

	mat4_load_identity(&jmat);
	mat4_rotate_x(&jmat, -jet->rx);
	mat4_rotate_y(&jmat, -jet->ry);

	if(jet->health <= 0)
	{
		jet->rx += (0x4000 - jet->rx)>>7;
		applied_ry = 1<<7;
		jet->tspd += 1<<7;
	} else {
		jet->rx += applied_rx;
		jet->tspd += (applied_tspd - jet->tspd)>>4;
	}

	jet->ry += applied_ry;
	jet->tilt_y += ((applied_ry*(0x3000>>9))-jet->tilt_y)>>4;

	fixed mvspd = jet->tspd;
	fixed mvspd_x = applied_vx<<13;

	// Apply matrix
	jet->pos[0] += fixmul(jmat[2][0], mvspd);
	jet->pos[1] += fixmul(jmat[2][1], mvspd);
	jet->pos[2] += fixmul(jmat[2][2], mvspd);

	jet->pos[0] += fixmul(jmat[0][0], mvspd_x);
	jet->pos[1] += fixmul(jmat[0][1], mvspd_x);
	jet->pos[2] += fixmul(jmat[0][2], mvspd_x);

	// Update flash
	if(jet->hit_flash > 0)
		jet->hit_flash--;

	// Fire mgun shot
	if(jet->mgun_wait > 0)
		jet->mgun_wait--;

	if(jet->mgun_wait <= 0 && jet->mgun_fire)
	{
		shot_fire(jet->rx, jet->ry, &jet->pos, 1<<17, 1);
		jet->mgun_wait = 50/8;
		jet->mgun_barrel ^= 1;
	}

	// Check against floor
	fixed hfloor = hmap_get(jet->pos[0], jet->pos[2]);
	if(jet->pos[1] >= hfloor)
	{
		jet->crashed = 1;
		jet->health = 0;
		// TODO: play sound
		jet->pos[1] = hfloor;
	}
}

