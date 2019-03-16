/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_rwops_rwops_trace_h
#define IGUARD_rwops_rwops_trace_h

#include "taisei.h"

#include <SDL.h>

SDL_RWops *SDL_RWWrapTrace(SDL_RWops *src, bool autoclose);

#endif // IGUARD_rwops_rwops_trace_h
