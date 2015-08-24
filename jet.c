typedef struct jet
{
	vec4 pos;
	fixed rx, ry;
	fixed tilt_y;
	fixed tspd;
} jet_s;

jet_s jet_test = {
	{0x18000, -0x60000, 0x150000, 0x10000},
	0, 0,
	0,
	1<<9,
};
jet_s player = {
	{0, 0, 0, 0x10000},
	0, 0,
	0,
	1<<13,
};

static void jet_draw(jet_s *jet, int is_shadow)
{
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
		mesh_draw(&poly_jet1, 0);
	}

}

static void jet_update(jet_s *jet,
	fixed applied_tspd, fixed applied_rx, fixed applied_ry,
	fixed applied_vx)
{
	mat4_load_identity(&mat_iplr);
	mat4_rotate_x(&mat_iplr, -jet->rx);
	mat4_rotate_y(&mat_iplr, -jet->ry);

	jet->rx += applied_rx;
	jet->ry += applied_ry;
	jet->tilt_y += (((applied_ry>>9)*0x3000)-jet->tilt_y)>>4;
	jet->tspd += (applied_tspd - jet->tspd)>>4;

	fixed mvspd = jet->tspd;
	fixed mvspd_x = applied_vx<<13;

	jet->pos[0] += fixmul(mat_iplr[2][0], mvspd);
	jet->pos[1] += fixmul(mat_iplr[2][1], mvspd);
	jet->pos[2] += fixmul(mat_iplr[2][2], mvspd);

	jet->pos[0] += fixmul(mat_iplr[0][0], mvspd_x);
	jet->pos[1] += fixmul(mat_iplr[0][1], mvspd_x);
	jet->pos[2] += fixmul(mat_iplr[0][2], mvspd_x);

	fixed hfloor = hmap_get(jet->pos[0], jet->pos[2]);
	if(jet->pos[1] >= hfloor)
	{
		// TODO: explode jet
		jet->pos[1] = hfloor;
	}
}

