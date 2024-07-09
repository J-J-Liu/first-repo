#include "dr_api.h"

// 声明record_branch函数
void record_branch(app_pc current_pc, app_pc next_pc, int instr_len, bool taken);

static void event_app_instruction(void *drcontext, instrlist_t *ilist, instr_t *where, void *user_data) {
    // 获取当前指令的PC值
    app_pc current_pc = instr_get_app_pc(where);
    // 获取指令长度
    int instr_len = instr_length(drcontext, where);
    
    // 检查当前指令是否是跳转指令
    if (instr_is_cbr(where) || instr_is_ubr(where)) {
        // 获取下一个即将执行的指令的PC值
        app_pc next_pc = instr_get_next_app_pc(where);
        
        // 创建一个调用record_branch的代码片段
        dr_insert_clean_call(drcontext, ilist, where, (void *)record_branch, false, 4,
                             OPND_CREATE_INTPTR(current_pc),
                             OPND_CREATE_INTPTR(next_pc),
                             OPND_CREATE_INT(instr_len),
                             OPND_CREATE_INT(instr_is_cbr(where)));
    }
}

// record_branch函数的实现
void record_branch(app_pc current_pc, app_pc next_pc, int instr_len, bool taken) {
    // 检查跳转是否被采取
    bool branch_taken = taken;
    if (branch_taken) {
        dr_fprintf(STDERR, "Branch taken at PC: %p to %p with length %d\n", current_pc, next_pc, instr_len);
    } else {
        dr_fprintf(STDERR, "Branch not taken at PC: %p with length %d\n", current_pc, instr_len);
    }
}

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[]) {
    dr_register_bb_event(event_app_instruction);
}
