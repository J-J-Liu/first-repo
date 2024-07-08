#include "dr_api.h"
#include "drmgr.h"
#include "drutil.h"
#include <string.h>

static file_t log_file;
static void *mutex;
static uint64 predict_success = 0;
static uint64 predict_failure = 0;

// 使用简单的上次结果预测算法
static bool last_result = true;

static void event_exit(void);
static dr_emit_flags_t event_bb_analysis(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating, void **user_data);
static dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr, bool for_trace, bool translating, void *user_data);
static void record_branch_prediction(void *drcontext, instr_t *instr, bool taken);

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[])
{
    dr_set_client_name("DynamoRIO Branch Prediction", "http://dynamorio.org/issues");

    if (!drmgr_init() || !drutil_init()) {
        DR_ASSERT(false);
        return;
    }

    log_file = dr_open_file("branch_prediction.log", DR_FILE_WRITE_OVERWRITE | DR_FILE_ALLOW_LARGE);
    DR_ASSERT(log_file != INVALID_FILE);
    mutex = dr_mutex_create();

    dr_register_exit_event(event_exit);
    drmgr_register_bb_instrumentation_event(event_bb_analysis, event_app_instruction, NULL);

    dr_log(NULL, DR_LOG_ALL, 1, "Client 'branch_prediction' initializing\n");
}

static void event_exit(void)
{
    dr_mutex_lock(mutex);
    dr_fprintf(log_file, "Prediction Success: %llu\n", predict_success);
    dr_fprintf(log_file, "Prediction Failure: %llu\n", predict_failure);
    dr_mutex_unlock(mutex);

    dr_mutex_destroy(mutex);
    dr_close_file(log_file);
    drutil_exit();
    drmgr_exit();
}

static dr_emit_flags_t event_bb_analysis(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating, void **user_data)
{
    return DR_EMIT_DEFAULT;
}

static dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr, bool for_trace, bool translating, void *user_data)
{
    if (instr_is_app(instr) && instr_is_cbr(instr)) {
        dr_insert_clean_call(drcontext, bb, instr, (void *)record_branch_prediction, false, 2,
                             OPND_CREATE_INTPTR(instr), OPND_CREATE_INTPTR(instr_is_cti_short(instr)));
    }
    return DR_EMIT_DEFAULT;
}

static void record_branch_prediction(void *drcontext, instr_t *instr, bool taken)
{
    dr_mutex_lock(mutex);

    // 预测上次分支结果
    bool prediction = last_result;
    last_result = taken;

    if (prediction == taken) {
        predict_success++;
    } else {
        predict_failure++;
    }

    dr_mutex_unlock(mutex);
}
