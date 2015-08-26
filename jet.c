#define JET_MAX 100
int jet_count = 0;
jet_s jet_list[JET_MAX];
int jet_enemy_max = 0;
int jet_enemy_rem = 0;
int jet_level = 0;
uint32_t jet_seeds[JET_MAX];

static int jet_add(fixed x, fixed y, fixed z, int health, int team, jet_ai_e ai_mode)
{
	int idx = jet_count++;

	jet_s *jet = &jet_list[idx];
	memset(jet, 0, sizeof(jet_s));

	jet->pos[0] = x;
	jet->pos[1] = y;
	jet->pos[2] = z;
	jet->pos[3] = 0x10000;
	jet->health = health;
	jet->team = team;
	jet->tspd = 1<<13;
	jet->ai_mode = ai_mode;

	if(jet->team == 2)
	{
		jet_enemy_max++;
		jet_enemy_rem++;
	}

	return idx;
}

static void jet_draw(jet_s *jet, int is_shadow)
{
	// Check if live
	if(jet->crashed >= 64) return;

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
	if(jet->crashed >= 1)
		mat4_scale(&mat_obj, (64-(jet->crashed-1))<<(16-6));
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

static void jet_update(jet_s *jet)
{
	mat4 jmat;

	if(jet->crashed > 0)
	{
		if(jet->crashed < 64)
			jet->crashed++;

		// TODO: release smoke
		return;
	}

	// Apply input
	fixed applied_rx = 0;
	fixed applied_ry = 0;
	fixed applied_vx = 0;
	fixed applied_tspd = 1<<13;

	if(jet->ai_mode == JAI_PLAYER)
	{
		/*
		// TEST: Kill player on SELECT
		if((pad_data & PAD_SELECT) != 0 && jet->health > 0)
			jet->health = 0;
		*/

		if((pad_data & PAD_X) != 0)
			applied_tspd = 1<<15;
		
		if((pad_data & PAD_LEFT) != 0)
			applied_ry -= 1;
		if((pad_data & PAD_RIGHT) != 0)
			applied_ry += 1;
		if((pad_data & PAD_UP) != 0)
			applied_rx -= 1;
		if((pad_data & PAD_DOWN) != 0)
			applied_rx += 1;
		if((pad_data & PAD_L1) != 0)
			applied_vx -= 1;
		if((pad_data & PAD_R1) != 0)
			applied_vx += 1;
		
		jet->mgun_fire = ((pad_data & PAD_S) != 0);

		applied_rx <<= 9;
		applied_ry <<= 9;

	} else switch(jet->ai_mode) {
		case JAI_HUNT_MAIN: {
			fixed dx = player->pos[0] - jet->pos[0];
			fixed dy = player->pos[1] - jet->pos[1];
			fixed dz = player->pos[2] - jet->pos[2];

			// because I'm too much of a cheapskate to implement proper boids
			fixed radj = fixrand1s()>>(16-8);

			// Calculate y-dot to work out best ry
			const fixed rotsy = 1<<8;
			fixed ys = fixsin(jet->ry + radj);
			fixed yc = fixcos(jet->ry + radj);
			fixed ysl = fixsin(jet->ry + radj - rotsy);
			fixed ycl = fixcos(jet->ry + radj - rotsy);
			fixed ysr = fixsin(jet->ry + radj + rotsy);
			fixed ycr = fixcos(jet->ry + radj + rotsy);
			fixed dot = fixmulf(dz, yc) + fixmulf(dx, ys);
			fixed dotl = fixmulf(dz, ycl) + fixmulf(dx, ysl);
			fixed dotr = fixmulf(dz, ycr) + fixmulf(dx, ysr);

			// Go towards player
			if(dotl > dot && dotl > dotr)
			{
				applied_ry = -rotsy;
			} else if(dotr > dot) {
				applied_ry =  rotsy;
			}

			// Make sure we don't crash into the ground
			// Go towards player - DO NOT FLY UPSIDE DOWN
			const fixed rotsx = 1<<8;
			fixed hdy = hmap_get(jet->pos[0], jet->pos[2]) - jet->pos[1];
			fixed xs = fixsin(jet->rx);
			fixed xc = fixcos(jet->rx);
			if(hdy < 0x40000 && jet->rx > -0x3000) 
			{
				applied_rx = -rotsx;
			} else {
				// Calculate x-dot to work out best rx
				fixed xsn = fixsin(jet->rx - rotsx);
				fixed xsp = fixsin(jet->rx + rotsx);
				fixed dotx = fixmulf(dy, xs);
				fixed dotxn = fixmulf(dy, xsn);
				fixed dotxp = fixmulf(dy, xsp);

			   	if(dotxn > dotx && dotxn > dotxp && jet->rx > -0x3000) {
					applied_rx = -rotsx;
				} else if(dotxp > dotx && jet->rx < 0x3000) {
					applied_rx =  rotsx;
				}
			}

			// Work out if we should fire
			vec4 dv = {dx, dy, dz, 0x00000};
			vec4 av = {fixmulf(ys, xc), xs, fixmulf(yc, xc), 0x00000};
			vec4_normalize_3(&dv);
			fixed dotfire = vec4_dot_3(&dv, &av);
			jet->mgun_fire = (dotfire >= 0xA800);

			// TODO: more stuff
		} break;

		case JAI_LTURN7:
			applied_ry = -1<<7;
			break;

		case JAI_RTURN7:
			applied_ry = 1<<7;
			break;

		default:
			break;
	}

	mat4_load_identity(&jmat);
	mat4_rotate_x(&jmat, -jet->rx);
	mat4_rotate_y(&jmat, -jet->ry);

	if(jet->health <= 0)
	{
		jet->rx += (0x4000 - jet->rx)>>7;
		applied_ry = 1<<7;
		jet->tspd += 1<<7;
		applied_vx = 0;
		jet->mgun_fire = 0;
	} else {
		jet->rx += applied_rx;
		jet->tspd += (applied_tspd - jet->tspd)>>4;
	}

	jet->ry += applied_ry;
	jet->tilt_y += ((applied_ry*(0x3000>>9))-jet->tilt_y)>>4;

	fixed mvspd = jet->tspd;
	fixed mvspd_x = applied_vx<<14;

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
		shot_fire(jet->rx, jet->ry, &jet->pos, 1<<17, jet->team);
		jet->mgun_wait = 50/8;
		jet->mgun_barrel ^= 1;
	}

	// Check against floor
	fixed hfloor = hmap_get(jet->pos[0], jet->pos[2]);
	if(jet->pos[1] >= hfloor)
	{
		jet->crashed = 1;
		if(jet->team == 2)
			jet_enemy_rem--;

		if(jet->health > 0 || jet == player)
		{
			// Play crash sound
			SPU_n_ADSR(16) = 0x9FC083FF;
			SPU_n_START(16) = s3mplayer.psx_spu_offset[16];
			SPU_n_PITCH(16) = 0x0800;
			SPU_n_MVOL_L(16) = (jet == player ? 0x3FFF : 0x1FFF);
			SPU_n_MVOL_R(16) = (jet == player ? 0x3FFF : 0x1FFF);
			SPU_KON = (1<<16);
		}

		jet->health = 0;
		jet->pos[1] = hfloor;

	}
}

