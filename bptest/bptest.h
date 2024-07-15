#ifndef BRANCH_PREDICTOR_H
#define BRANCH_PREDICTOR_H

#include "dr_api.h"

void branch_predictor_init();
void branch_predictor_exit();
void branch_predictor_instrument_branch(void *drcontext, instrlist_t *bb, instr_t *instr);
void branch_predictor_update(void *drcontext, instr_t *instr, bool taken);

#endif // BRANCH_PREDICTOR_H
