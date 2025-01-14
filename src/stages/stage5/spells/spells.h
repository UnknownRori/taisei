/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "common_tasks.h"

#include "stages/stage5/iku.h"

DECLARE_EXTERN_TASK_WITH_INTERFACE(stage5_midboss_static_bomb, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage5_spell_atmospheric_discharge, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage5_spell_artificial_lightning, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage5_spell_induction, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage5_spell_natural_cathode, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage5_spell_overload, BossAttack);
