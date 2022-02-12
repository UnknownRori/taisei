/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"

#include "common_tasks.h"

#define LHC_PERIOD 400

static real lhc_target_height(int turn) {
	return 100.0 + 400.0 * (turn&1);
}

TASK(lhc_laser, { BoxedEllyBaryons baryons; int baryon_idx; real direction; Color color;}) {
	EllyBaryons *baryons = NOT_NULL(ENT_UNBOX(ARGS.baryons));

	Laser *l = TASK_BIND(create_laser(baryons->poss[ARGS.baryon_idx], 200, 300, &ARGS.color, las_linear, NULL, ARGS.direction * VIEWPORT_W * 0.005, 0, 0, 0));
	l->unclearable = true;
	laser_make_static(l);

	INVOKE_SUBTASK(laser_charge, ENT_BOX(l), 200, 30);

	for(;;) {
		l->pos = NOT_NULL(ENT_UNBOX(ARGS.baryons))->poss[ARGS.baryon_idx];
		YIELD;
	}
}

TASK(lhc_baryons_movement, { BoxedEllyBaryons baryons; BoxedBoss boss; }) {
	EllyBaryons *baryons = TASK_BIND(ARGS.baryons);

	for(int t = 0;; t++) {
		real target = lhc_target_height(t/LHC_PERIOD);

		for(int i = 0; i < NUM_BARYONS; i++) {
			real x = VIEWPORT_W * (i > 4 || i < 2);

			if(i == 0 || i == 3) {
				// TODO: define and replace by standard position
				baryons->target_poss[i] = NOT_NULL(ENT_UNBOX(ARGS.boss))->pos + 100 * cdir(M_TAU/NUM_BARYONS * i);
			} else {
				baryons->target_poss[i] = x + I * (target + (100 - 0.4 * (t % LHC_PERIOD)) * (1 - 2 * (i > 3)));
			}
		}

		baryons->center_pos = NOT_NULL(ENT_UNBOX(ARGS.boss))->pos;
		YIELD;
	}
}

TASK(lhc_baryons, { BoxedEllyBaryons baryons; BoxedBoss boss; }) {
	TASK_BIND(ARGS.baryons);

	INVOKE_SUBTASK(lhc_baryons_movement, ARGS.baryons, ARGS.boss);

	for(;;) {
		play_sfx_delayed("laser1", 10, true, 200);

		for(int baryon_idx = 2; baryon_idx < NUM_BARYONS; baryon_idx += 3) {
			Color clr = *RGBA(0.1 + 0.9 * (baryon_idx > 3), 0, 1 - 0.9 * (baryon_idx > 3), 0);
			INVOKE_SUBTASK(lhc_laser, ARGS.baryons, baryon_idx,
				.direction = (1 - 2 * (baryon_idx > 3)),
				.color = clr
			);
		}
		WAIT(LHC_PERIOD);
	}

}

TASK(lhc_secondary_projs, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);

	int interval = difficulty_value(6, 5, 4, 3);
	for(int i = 0;; i++) {
		play_sfx_ex("shot2", 10, false);
		
		PROJECTILE(
			.proto = pp_ball,
			.pos = boss->pos,
			.color = RGBA(0.0, 0.4, 1.0, 0.0),
			.move = move_asymptotic_simple(2 * cdir(2 * i), 3)
		);

		WAIT(interval);
	}
}

DEFINE_EXTERN_TASK(stage6_spell_lhc) {
	Boss *boss = stage6_elly_init_baryons_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);
	
	INVOKE_SUBTASK(lhc_baryons, ARGS.baryons, ENT_BOX(boss));

	INVOKE_SUBTASK(lhc_secondary_projs, ENT_BOX(boss));

	WAIT(230);

	int count = difficulty_value(40, 50, 60, 70);
	int laser_lifetime = difficulty_value(90, 110, 130, 150);

	for(int turn = 0;; turn++) {
		elly_clap(boss, 100);
		WAIT(20);

		cmplx pos = VIEWPORT_W / 2.0 + I * lhc_target_height(turn);

		stage_shake_view(160);
		play_sfx("boom");

		for(int i = 0; i < count; i++) {
			cmplx vel = 3 * rng_dir();

			Laser *l = create_lasercurve2c(pos, laser_lifetime, 300, RGBA(0.5, 0.3, 0.9, 0), las_accel, vel, 0.02 * rng_real() * sign(creal(vel)));
			l->width = 15;

			real speed1 = rng_range(1, 3.5);
			real speed2 = rng_range(1, 3.5);
			
			PROJECTILE(
				.proto = pp_soul,
				.pos = pos,
				.color = RGBA(0.4, 0.0, 1.0, 0.0),
				.move = move_linear(speed1 * rng_dir()),
				.flags = PFLAG_NOSPAWNFLARE,
			);

			PROJECTILE(
				.proto = pp_bigball,
				.pos = pos,
				.color = RGBA(1.0, 0.0, 0.4, 0.0),
				.move = move_linear(speed2 * rng_dir()),
				.flags = PFLAG_NOSPAWNFLARE,
			);

			if(i < 5) {
				PARTICLE(
					.sprite = "stain",
					.pos = pos,
					.color = RGBA(0.3, 0.3, 1.0, 0.0),
					.timeout = 60,
					.draw_rule = pdraw_timeout_scalefade(2, 5, 1, 0),
					.move = move_linear(rng_range(10,12) * cdir(M_TAU / 5 * i)),
					.flags = PFLAG_REQUIREDPARTICLE,
				);
			}
		}
		WAIT(LHC_PERIOD - 20);
	}
}

