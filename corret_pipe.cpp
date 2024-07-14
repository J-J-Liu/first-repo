#include "dr_api.h"
#include "drmgr.h"
#include "drutil.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PIPE_NAME "/tmp/my_pipe"

static file_t log_file;
static void *mutex;

static void event_exit(void);
static dr_emit_flags_t event_bb_analysis(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating, void **user_data);
static dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr, bool for_trace, bool translating, void *user_data);
static void record_instruction(void *drcontext, int opcode);
static const char *opnd_disassemble_to_string(void *drcontext, opnd_t opnd);

int fd;

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[])
{
    dr_set_client_name("DynamoRIO Instruction Recorder", "http://dynamorio.org/issues");
    mkfifo(PIPE_NAME, 0666);
    fd = dr_open_file(PIPE_NAME, DR_FILE_WRITE_OVERWRITE | DR_FILE_ALLOW_LARGE);
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
    dr_close_file(fd);
    drmgr_exit();
}

static dr_emit_flags_t event_bb_analysis(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating, void **user_data)
{
    return DR_EMIT_DEFAULT;
}

static dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr, bool for_trace, bool translating, void *user_data)
{
    // 禁用自动谓词化，因为我们希望无条件执行以下插桩代码
    drmgr_disable_auto_predication(drcontext, bb);

    // 如果user_data为空，说明不需要对该基本块进行插桩
    if (user_data == NULL)
        return DR_EMIT_DEFAULT;

    // 在每条应用程序指令前插入清理调用
    if (instr_is_app(instr)) {
        int opcode = instr_get_opcode(instr); // 获取指令操作码
        dr_insert_clean_call(drcontext, bb, instr, (void *)record_instruction,
                             false /* save fpstate */, 2, 
                             OPND_CREATE_INTPTR(drcontext),
                             OPND_CREATE_INT32(opcode));
    }

    return DR_EMIT_DEFAULT;
}

#pragma pack(push, 1)
typedef struct {
    int id;
    int opcode;
    char opcode_name[50];
} MyStruct;
#pragma pack(pop)


static void record_instruction(void *drcontext, int opcode)
{
    dr_fprintf(log_file, "Instruction: %s\n", decode_opcode_name(opcode));
    MyStruct instr_info;
    instr_info.id = 0;
    instr_info.opcode = 2;
    snprintf(instr_info.opcode_name, sizeof(instr_info.opcode_name), "example");
    dr_write_file(fd, &instr_info,  sizeof(MyStruct));
    printf("Written to file\n");
}

static const char *opnd_disassemble_to_string(void *drcontext, opnd_t opnd)
{
    static char buf[256];
    opnd_disassemble_to_buffer(drcontext, opnd, buf, sizeof(buf));
    return buf;
}
