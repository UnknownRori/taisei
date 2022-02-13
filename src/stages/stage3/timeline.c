/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "timeline.h"
#include "stage3.h"
#include "wriggle.h"
#include "scuttle.h"
#include "spells/spells.h"
#include "nonspells/nonspells.h"
#include "background_anim.h"

#include "global.h"
#include "stagetext.h"
#include "common_tasks.h"
#include "enemy_classes.h"

static void stage3_dialog_post_boss(void) {
	PlayerMode *pm = global.plr.mode;
	INVOKE_TASK_INDIRECT(Stage3PostBossDialog, pm->dialog->Stage3PostBoss);
}

TASK(swarm_trail_proj, { cmplx pos; cmplx vstart; cmplx vend; real x; real width;}) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_rice,
		.color = RGB(0.4, 0, 0.8),
		.max_viewport_dist = 1000,
	));
	BoxedProjectile phead = ENT_BOX(PROJECTILE(
		.proto = pp_flea,
		.color = RGB(0.8, 0.1, 0.4),
		.max_viewport_dist = 1000,
	));

	real turn_length = 1; // |x/turn_length| should be smaller than pi

	cmplx prevpos = ARGS.pos;
	for(int t = -70;; t++) {
		if(t == 0) {
			play_sfx("redirect");
		}
		cmplx z = t/ARGS.width + I * ARGS.x/turn_length;

		p->pos = ARGS.pos + ARGS.width * z * (ARGS.vstart + (ARGS.vend-ARGS.vstart)/(cexp(-z) + 1));

		p->angle = carg(p->pos-prevpos);

		Projectile *head = ENT_UNBOX(phead);
		if(head) {
			head->pos = p->pos + 8*cdir(p->angle);
		}
		p->move = move_linear(p->pos-prevpos);
		
		prevpos = p->pos;
		YIELD;
	}
}

TASK(swarm_trail_fairy, { cmplx pos; MoveParams move; }) {
	Enemy *e = TASK_BIND(espawn_big_fairy(ARGS.pos, ITEMS(
		.points = 1,
		.power = 2,
	)));

	e->move = ARGS.move;
	
	int shooting_time = 200;
	real width = 30;
	int interval = 10;
	int nrow = 5;

	WAIT(30);
	e->move.retention = 0.9;
	WAIT(10);
	cmplx aim = cnormalize(global.plr.pos - e->pos);
	
	for(int t = 0; t < shooting_time/interval; t++) {
		play_sfx("shot1");

		for(int i = 0; i < nrow - (t&1); i++) {
			real x = ((i+0.5*(t&1))/(real)(nrow-1) - 0.5);

			INVOKE_TASK(swarm_trail_proj, e->pos, ARGS.move.velocity, 3*aim, x, .width = width);
		}
		WAIT(interval);
	}
	play_sfx("redirect");
	e->move = move_asymptotic_halflife(0, -ARGS.move.velocity , 120);
}

TASK(swarm_trail_fairy_spawn, { int count; }) {

	for(int i = 0; i < ARGS.count; i++) {
		cmplx pos = VIEWPORT_W / 2.0;
		cmplx vel = 5*I + 4 * cdir(-M_PI * i / (real)(ARGS.count-1));
		INVOKE_TASK(swarm_trail_fairy, pos, move_linear(vel));

		WAIT(60);
	}
	

}

TASK(flower_swirl, { cmplx pos; cmplx dir; }) {
	Enemy *e = TASK_BIND(espawn_swirl(ARGS.pos, ITEMS(
		.points = 1,
	)));

	real angular_velocity = 0.1;
	real radius = 50;
	cmplx retention = 0.99*cdir(angular_velocity);
	e->move = (MoveParams){
		.velocity = radius * cnormalize(ARGS.dir) * I * cabs(1 - retention),
		.retention = retention,
		.acceleration = ARGS.dir * (1 - retention)
	};

	int interval = 10;
	int petals = 5;
	for(int t = 0;; t++) {
		play_sfx("shot2");
		for(int i = 0; i < petals; i++) {
			cmplx dir = cdir(M_TAU / petals * i);
			PROJECTILE(
				.proto = pp_wave,
				.color = RGB(1, 0.5, 0.6),
				.pos = e->pos - 4 * dir,
				.move = move_asymptotic_halflife(0.1*dir, -2 * dir, 220),
			);
		}

		WAIT(interval);
	}
}

TASK(flower_swirl_spawn, { cmplx pos; cmplx dir; int count; int interval; cmplx spread; }) {
	for(int i = 0; i < ARGS.count; i++) {
		cmplx pos = ARGS.pos + creal(ARGS.spread) * rng_sreal();
		pos += I * cimag(ARGS.spread) * rng_sreal();

		INVOKE_TASK(flower_swirl, pos, ARGS.dir);
		WAIT(ARGS.interval);
	}
}

TASK(horde_fairy, { cmplx pos; }) {
	Enemy *e;
	if(rng_chance(0.5)) {
		e = TASK_BIND(espawn_fairy_blue(ARGS.pos, ITEMS(
			.points = 1,
		)));
	} else {
		e = TASK_BIND(espawn_fairy_red(ARGS.pos, ITEMS(
			.power = 1,
		)));
	}

	e->move = move_linear(1.5 * I);
	int interval = 40;

	for(int t = 0;; t++, WAIT(interval)) {
		play_sfx("shot1");
		cmplx diff = global.plr.pos - e->pos;

		if(cabs(diff) < 40) {
			continue;
		}

		cmplx aim = cnormalize(diff);
		PROJECTILE(
			.proto = pp_plainball,
			.pos = e->pos,
			.color = RGB(0.4, 0, 1),
			.move = move_asymptotic_halflife(0, 2 * aim, 25),
		);
		PROJECTILE(
			.proto = pp_flea,
			.pos = e->pos,
			.color = RGB(0.1, 0.4, 0.8),
			.move = move_asymptotic_halflife(0, 2 * aim, 20),
		);
	}
}

TASK(horde_fairy_spawn, { int count; int interval; }) {
	for(int t = 0; t < ARGS.count; t++) {
		INVOKE_TASK(horde_fairy, .pos = VIEWPORT_W * rng_real());
		WAIT(ARGS.interval);
	}
}

TASK(circle_twist_fairy_lances, { BoxedEnemy enemy; }) {
	Enemy *e = TASK_BIND(ARGS.enemy);

	cmplx offset = rng_dir();

	int lance_count = 300;
	int lance_segs = 20;
	int lance_dirs = 100;
	int fib1 = 1;
	int fib2 = 1;
	for(int i = 0; i < lance_count; i++) {
		play_sfx("shot3");
		int tmp = fib1;
		fib1 = (fib1 + fib2) % lance_dirs;
		fib2 = tmp;
		
		cmplx dir = offset * cdir(M_TAU / lance_dirs * fib1);
		for(int j = 0; j < lance_segs; j++) {
			int s = 1 - 2 * (j & 1);
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos + 20 * dir,
				.color = RGB(0.3, 0.4, 1),
					.move = move_asymptotic_halflife((0.5+4.5*j/lance_segs)*dir, 5*dir*cdir(0.05 * s), 50+10*I),
			);
		}

		WAIT(2);
	}
}


TASK(circle_twist_fairy, { cmplx pos; cmplx target_pos; }) {
	Enemy *e = TASK_BIND(espawn_super_fairy(ARGS.pos, ITEMS(
		.power = 5,
		.points = 5,
		.bomb_fragment = 1,
	)));

	e->move=move_towards(ARGS.target_pos, 0.01);
	WAIT(50);

	int circle_count = 10;
	int count = 50;
	int interval = 10;

	INVOKE_SUBTASK_DELAYED(300, circle_twist_fairy_lances, ENT_BOX(e));

	for(int i = 0; i < circle_count; i++) {
		int s = 1-2*(i&1);
		play_sfx("shot_special1");
		for(int t = 0; t < count; t++) {
			play_sfx_loop("shot1_loop");
			cmplx dir = -I * cdir(s * M_TAU / count * t);
			PROJECTILE(
				.proto = pp_ball,
				.pos = e->pos,
				.color = RGB(0.4, 0, 1),
				.move = {(2 + 0.005 * t) * dir, -0.01 * dir, 1.0 * cdir(-0.01*s)}
			);
			YIELD;
		}
		WAIT(interval);
	}

	e->move = move_asymptotic_halflife(0, 2 * I, 20);

}	

// bosses

TASK_WITH_INTERFACE(midboss_intro, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	boss->move = move_towards(VIEWPORT_W/2 + 100.0*I, 0.04);
}

TASK_WITH_INTERFACE(midboss_outro, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	for(int t = 0;; t++) {
		boss->pos += t*t/900.0 * cdir(3*M_PI/2 + 0.5 * sin(t / 20.0));
		YIELD;
	}
}

TASK(spawn_midboss) {
	STAGE_BOOKMARK_DELAYED(120, midboss);

	Boss *boss = global.boss = stage3_spawn_scuttle(VIEWPORT_W/2 - 200.0*I);
	boss_add_attack_task(boss, AT_Move, "Introduction", 1, 0, TASK_INDIRECT(BossAttack, midboss_intro), NULL);
	boss_add_attack_task(boss, AT_Normal, "Lethal Bite", 11, 20000, TASK_INDIRECT(BossAttack, stage3_midboss_nonspell_1), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.mid.deadly_dance, false);
	boss_add_attack_task(boss, AT_Move, "Runaway", 2, 1, TASK_INDIRECT(BossAttack, midboss_outro), NULL);
	boss->zoomcolor = *RGB(0.4, 0.1, 0.4);

	boss_engage(boss);
}

TASK(boss_appear, { BoxedBoss boss; }) {
	Boss *boss = TASK_BIND(ARGS.boss);
	boss->move = move_towards(VIEWPORT_W/2.0 + 100.0*I, 0.05);
}

TASK(spawn_boss) {
	STAGE_BOOKMARK_DELAYED(120, boss);

	Boss *boss = global.boss = stage3_spawn_wriggle(VIEWPORT_W/2 - 200.0*I);

	PlayerMode *pm = global.plr.mode;
	Stage3PreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(Stage3PreBossDialog, pm->dialog->Stage3PreBoss, &e);
	INVOKE_TASK_WHEN(&e->boss_appears, boss_appear, ENT_BOX(boss));
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stage3boss");
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	boss_add_attack_task(boss, AT_Normal, "", 11, 35000, TASK_INDIRECT(BossAttack, stage3_boss_nonspell_1), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.boss.moonlight_rocket, false);
	boss_add_attack_task(boss, AT_Normal, "", 40, 35000, TASK_INDIRECT(BossAttack, stage3_boss_nonspell_2), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.boss.wriggle_night_ignite, false);
	boss_add_attack_task(boss, AT_Normal, "", 40, 35000, TASK_INDIRECT(BossAttack, stage3_boss_nonspell_3), NULL);
	boss_add_attack_from_info(boss, &stage3_spells.boss.firefly_storm, false);
	boss_add_attack_from_info(boss, &stage3_spells.extra.light_singularity, false);

	boss_engage(boss);
}

DEFINE_EXTERN_TASK(stage3_timeline) {
	stage_start_bgm("stage3");
	stage_set_voltage_thresholds(50, 125, 300, 600);

	INVOKE_TASK(flower_swirl_spawn,
		.pos = VIEWPORT_W + 10 + 200 * I,
		.dir = -3 - 1 * I,
		.count = 10,
		.interval = 30,
		.spread = 10 + 200*I
	);
	INVOKE_TASK_DELAYED(100, flower_swirl_spawn,
		.pos = -10 + 200 * I,
		.dir = 3 - 1 * I,
		.count = 8,
		.interval = 30,
		.spread = 10 + 150*I
	);
	
	INVOKE_TASK_DELAYED(400, swarm_trail_fairy_spawn, 5);

	STAGE_BOOKMARK_DELAYED(800, horde);
	INVOKE_TASK_DELAYED(800, horde_fairy_spawn,
		.count = 30,
		.interval = 20
	);
	
	STAGE_BOOKMARK_DELAYED(1300, circle-twist);
	INVOKE_TASK_DELAYED(1300, circle_twist_fairy,
		.pos = 0,
		.target_pos = VIEWPORT_W/2.0 + I*VIEWPORT_H/3.0,
	);

	


	STAGE_BOOKMARK_DELAYED(2500, pre-midboss);

	INVOKE_TASK_DELAYED(2700, spawn_midboss);

	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);

	WAIT(150);

	STAGE_BOOKMARK(post-midboss);




	STAGE_BOOKMARK_DELAYED(2800, pre-boss);

	WAIT(3000);

	stage_unlock_bgm("stage3boss");

	INVOKE_TASK(spawn_boss);

	while(!global.boss) YIELD;
	WAIT_EVENT(&global.boss->events.defeated);

	WAIT(240);
	stage3_dialog_post_boss();
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	WAIT(5);
	stage_finish(GAMEOVER_SCORESCREEN);
}
