#include "dr_api.h"
#include "drmgr.h"
#include "drutil.h"
#include <string.h>

static file_t log_file;
static void *mutex;

static void event_exit(void);
static dr_emit_flags_t event_bb_analysis(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating, void **user_data);
static dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr, bool for_trace, bool translating, void *user_data);
static void record_instruction(void *drcontext, instr_t *instr);

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[])
{
    dr_set_client_name("DynamoRIO Instruction Recorder", "http://dynamorio.org/issues");

    if (!drmgr_init()) {
        DR_ASSERT(false);
        return;
    }

    log_file = dr_open_file("opcode_operands.log", DR_FILE_WRITE_OVERWRITE | DR_FILE_ALLOW_LARGE);
    DR_ASSERT(log_file != INVALID_FILE);
    mutex = dr_mutex_create();

    dr_register_exit_event(event_exit);
    drmgr_register_bb_instrumentation_event(event_bb_analysis, event_app_instruction, NULL);

    dr_log(NULL, DR_LOG_ALL, 1, "Client 'record_opcode_operands' initializing\n");
}

static void event_exit(void)
{
    dr_mutex_destroy(mutex);
    dr_close_file(log_file);
    drmgr_exit();
}

static dr_emit_flags_t event_bb_analysis(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating, void **user_data)
{
    return DR_EMIT_DEFAULT;
}

static dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr, bool for_trace, bool translating, void *user_data)
{
    if (instr_is_app(instr)) {
        dr_insert_clean_call(drcontext, bb, instr, (void *)record_instruction, false, 1, OPND_CREATE_INTPTR(instr));
    }
    return DR_EMIT_DEFAULT;
}

static void record_instruction(void *drcontext, instr_t *instr)
{
    dr_mutex_lock(mutex);

    byte buf[256];
    int len = instr_disassemble_to_buffer(drcontext, instr, buf, sizeof(buf));
    DR_ASSERT(len > 0);

    // 获取操作码
    int opcode = instr_get_opcode(instr);
    dr_fprintf(log_file, "Opcode: %d\n", opcode);

    // 记录反汇编的指令
    dr_fprintf(log_file, "Instruction: %s\n", buf);

    // 获取并记录源操作数
    int num_srcs = instr_num_srcs(instr);
    dr_fprintf(log_file, "  Sources:\n");
    for (int i = 0; i < num_srcs; i++) {
        opnd_t src = instr_get_src(instr, i);
        dr_fprintf(log_file, "    %s\n", opnd_disassemble_to_string(drcontext, src));
    }

    // 获取并记录目的操作数
    int num_dsts = instr_num_dsts(instr);
    dr_fprintf(log_file, "  Destinations:\n");
    for (int i = 0; i < num_dsts; i++) {
        opnd_t dst = instr_get_dst(instr, i);
        dr_fprintf(log_file, "    %s\n", opnd_disassemble_to_string(drcontext, dst));
    }

    dr_fprintf(log_file, "\n");

    dr_mutex_unlock(mutex);
}

static const char *opnd_disassemble_to_string(void *drcontext, opnd_t opnd)
{
    static char buf[256];
    opnd_disassemble_to_buffer(drcontext, opnd, buf, sizeof(buf));
    return buf;
}
