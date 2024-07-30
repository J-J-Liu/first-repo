static void
insert_save_opcode(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                   reg_id_t scratch, int opcode)
{
    scratch = reg_resize_to_opsz(scratch, OPSZ_2);
    MINSERT(ilist, where,
            XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                  OPND_CREATE_INT16(opcode)));
    MINSERT(ilist, where,
            XINST_CREATE_store_2bytes(
                drcontext, OPND_CREATE_MEM16(base, offsetof(ins_ref_t, opcode)),
                opnd_create_reg(scratch)));
}

static void
insert_save_pc(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
               reg_id_t scratch, app_pc pc)
{
    instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)pc, opnd_create_reg(scratch),
                                     ilist, where, NULL, NULL);
    MINSERT(ilist, where,
            XINST_CREATE_store(drcontext,
                               OPND_CREATE_MEMPTR(base, offsetof(ins_ref_t, pc)),
                               opnd_create_reg(scratch)));
}

static void
insert_save_is_cbr(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                   reg_id_t scratch, bool is_cbr)
{
    MINSERT(ilist, where,
            XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                  OPND_CREATE_INT8(is_cbr)));
    MINSERT(ilist, where,
            XINST_CREATE_store_1byte(
                drcontext, OPND_CREATE_MEM8(base, offsetof(ins_ref_t, is_cbr)),
                opnd_create_reg(scratch)));
}

static void
insert_save_target_addr(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                        reg_id_t scratch, app_pc target_addr)
{
    instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)target_addr, opnd_create_reg(scratch),
                                     ilist, where, NULL, NULL);
    MINSERT(ilist, where,
            XINST_CREATE_store(drcontext,
                               OPND_CREATE_MEMPTR(base, offsetof(ins_ref_t, target_addr)),
                               opnd_create_reg(scratch)));
}

static void
insert_save_fall_addr(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                      reg_id_t scratch, app_pc fall_addr)
{
    instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)fall_addr, opnd_create_reg(scratch),
                                     ilist, where, NULL, NULL);
    MINSERT(ilist, where,
            XINST_CREATE_store(drcontext,
                               OPND_CREATE_MEMPTR(base, offsetof(ins_ref_t, fall_addr)),
                               opnd_create_reg(scratch)));
}

static void
insert_save_num_operands(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                         reg_id_t scratch, int num_operands)
{
    MINSERT(ilist, where,
            XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                  OPND_CREATE_INT32(num_operands)));
    MINSERT(ilist, where,
            XINST_CREATE_store_4bytes(
                drcontext, OPND_CREATE_MEM32(base, offsetof(ins_ref_t, num_operands)),
                opnd_create_reg(scratch)));
}

static void
insert_save_operand(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                    reg_id_t scratch, operand_t *operand, int index)
{
    // Calculate the offset for the operand at the given index
    size_t operand_offset = offsetof(ins_ref_t, operands) + index * sizeof(operand_t);

    // Save operand type
    MINSERT(ilist, where,
            XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                  OPND_CREATE_INT8(operand->type)));
    MINSERT(ilist, where,
            XINST_CREATE_store_1byte(
                drcontext, OPND_CREATE_MEM8(base, operand_offset + offsetof(operand_t, type)),
                opnd_create_reg(scratch)));

    // Save is_source
    MINSERT(ilist, where,
            XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                  OPND_CREATE_INT8(operand->is_source)));
    MINSERT(ilist, where,
            XINST_CREATE_store_1byte(
                drcontext, OPND_CREATE_MEM8(base, operand_offset + offsetof(operand_t, is_source)),
                opnd_create_reg(scratch)));

    // Save is_dest
    MINSERT(ilist, where,
            XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                  OPND_CREATE_INT8(operand->is_dest)));
    MINSERT(ilist, where,
            XINST_CREATE_store_1byte(
                drcontext, OPND_CREATE_MEM8(base, operand_offset + offsetof(operand_t, is_dest)),
                opnd_create_reg(scratch)));

    // Save operand value based on type
    switch (operand->type) {
        case OPERAND_TYPE_REGISTER:
            MINSERT(ilist, where,
                    XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                          OPND_CREATE_INT32(operand->value.reg)));
            MINSERT(ilist, where,
                    XINST_CREATE_store_4bytes(
                        drcontext, OPND_CREATE_MEM32(base, operand_offset + offsetof(operand_t, value.reg)),
                        opnd_create_reg(scratch)));
            break;
        case OPERAND_TYPE_MEMORY:
            instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)operand->value.mem_addr, opnd_create_reg(scratch),
                                             ilist, where, NULL, NULL);
            MINSERT(ilist, where,
                    XINST_CREATE_store(drcontext,
                                       OPND_CREATE_MEMPTR(base, operand_offset + offsetof(operand_t, value.mem_addr)),
                                       opnd_create_reg(scratch)));
            break;
        case OPERAND_TYPE_IMMEDIATE:
            MINSERT(ilist, where,
                    XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                          OPND_CREATE_INT32(operand->value.imm_val)));
            MINSERT(ilist, where,
                    XINST_CREATE_store_4bytes(
                        drcontext, OPND_CREATE_MEM32(base, operand_offset + offsetof(operand_t, value.imm_val)),
                        opnd_create_reg(scratch)));
            break;
    }
}

static void
insert_save_bubble_type(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                        reg_id_t scratch, bubble_type_t bubble_type)
{
    MINSERT(ilist, where,
            XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                  OPND_CREATE_INT8(bubble_type)));
    MINSERT(ilist, where,
            XINST_CREATE_store_1byte(
                drcontext, OPND_CREATE_MEM8(base, offsetof(ins_ref_t, bubble_type)),
                opnd_create_reg(scratch)));
}

// Example function to insert all the save operations for an instruction reference
static void
insert_save_ins_ref(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                    reg_id_t scratch, ins_ref_t *ins_ref)
{
    insert_save_pc(drcontext, ilist, where, base, scratch, ins_ref->pc);
    insert_save_opcode(drcontext, ilist, where, base, scratch, ins_ref->opcode);
    insert_save_is_cbr(drcontext, ilist, where, base, scratch, ins_ref->is_cbr);
    insert_save_target_addr(drcontext, ilist, where, base, scratch, ins_ref->target_addr);
    insert_save_fall_addr(drcontext, ilist, where, base, scratch, ins_ref->fall_addr);
    insert_save_num_operands(drcontext, ilist, where, base, scratch, ins_ref->num_operands);

    for (int i = 0; i < ins_ref->num_operands; i++) {
        insert_save_operand(drcontext, ilist, where, base, scratch, &ins_ref->operands[i], i);
    }

    insert_save_bubble_type(drcontext, ilist, where, base, scratch, ins_ref->bubble_type);
}


static void
instrument_instr(void *drcontext, instrlist_t *ilist, instr_t *where)
{
    /* We need two scratch registers */
    reg_id_t reg_ptr, reg_tmp;
    if (drreg_reserve_register(drcontext, ilist, where, NULL, &reg_ptr) != DRREG_SUCCESS ||
        drreg_reserve_register(drcontext, ilist, where, NULL, &reg_tmp) != DRREG_SUCCESS) {
        DR_ASSERT(false); /* cannot recover */
        return;
    }

    // Load the buffer pointer into reg_ptr
    insert_load_buf_ptr(drcontext, ilist, where, reg_ptr);

    // Save the instruction's program counter (PC)
    insert_save_pc(drcontext, ilist, where, reg_ptr, reg_tmp, instr_get_app_pc(where));

    // Save the instruction's opcode
    insert_save_opcode(drcontext, ilist, where, reg_ptr, reg_tmp, instr_get_opcode(where));

    // Check if the instruction is a conditional branch and save it
    bool is_cbr = instr_is_cbr(where);
    insert_save_is_cbr(drcontext, ilist, where, reg_ptr, reg_tmp, is_cbr);

    // Save the target address if the instruction is a branch
    app_pc target_addr = is_cbr ? instr_get_branch_target_pc(where) : NULL;
    insert_save_target_addr(drcontext, ilist, where, reg_ptr, reg_tmp, target_addr);

    // Save the fall-through address
    app_pc fall_addr = instr_get_app_pc(where) + instr_length(drcontext, where);
    insert_save_fall_addr(drcontext, ilist, where, reg_ptr, reg_tmp, fall_addr);

    // Save the number of operands
    int num_operands = instr_num_srcs(where) + instr_num_dsts(where);
    insert_save_num_operands(drcontext, ilist, where, reg_ptr, reg_tmp, num_operands);

    // Save operand details
    int operand_index = 0;
    for (int i = 0; i < instr_num_srcs(where) && operand_index < 4; i++, operand_index++) {
        operand_t operand;
        operand.type = OPERAND_TYPE_REGISTER; // Default initialization
        operand.is_source = true;
        operand.is_dest = false;

        opnd_t src_opnd = instr_get_src(where, i);
        if (opnd_is_reg(src_opnd)) {
            operand.type = OPERAND_TYPE_REGISTER;
            operand.value.reg = opnd_get_reg(src_opnd);
        } else if (opnd_is_memory_reference(src_opnd)) {
            operand.type = OPERAND_TYPE_MEMORY;
            operand.value.mem_addr = opnd_get_addr(src_opnd);
        } else if (opnd_is_immed(src_opnd)) {
            operand.type = OPERAND_TYPE_IMMEDIATE;
            operand.value.imm_val = opnd_get_immed_int(src_opnd);
        }

        insert_save_operand(drcontext, ilist, where, reg_ptr, reg_tmp, &operand, operand_index);
    }

    for (int i = 0; i < instr_num_dsts(where) && operand_index < 4; i++, operand_index++) {
        operand_t operand;
        operand.type = OPERAND_TYPE_REGISTER; // Default initialization
        operand.is_source = false;
        operand.is_dest = true;

        opnd_t dst_opnd = instr_get_dst(where, i);
        if (opnd_is_reg(dst_opnd)) {
            operand.type = OPERAND_TYPE_REGISTER;
            operand.value.reg = opnd_get_reg(dst_opnd);
        } else if (opnd_is_memory_reference(dst_opnd)) {
            operand.type = OPERAND_TYPE_MEMORY;
            operand.value.mem_addr = opnd_get_addr(dst_opnd);
        } else if (opnd_is_immed(dst_opnd)) {
            operand.type = OPERAND_TYPE_IMMEDIATE;
            operand.value.imm_val = opnd_get_immed_int(dst_opnd);
        }

        insert_save_operand(drcontext, ilist, where, reg_ptr, reg_tmp, &operand, operand_index);
    }

    // Save bubble type, assuming it is always BUBBLE_NONE
    insert_save_bubble_type(drcontext, ilist, where, reg_ptr, reg_tmp, BUBBLE_NONE);

    // Update the buffer pointer to the next ins_ref_t slot
    insert_update_buf_ptr(drcontext, ilist, where, reg_ptr, sizeof(ins_ref_t));

    /* Restore scratch registers */
    if (drreg_unreserve_register(drcontext, ilist, where, reg_ptr) != DRREG_SUCCESS ||
        drreg_unreserve_register(drcontext, ilist, where, reg_tmp) != DRREG_SUCCESS) {
        DR_ASSERT(false);
    }
}















#include <stddef.h>
#include <stdbool.h>
#include "dr_api.h"
#include "drmgr.h"
#include "drutil.h"
#include "drreg.h"
#include "drx.h"
#include "drwrap.h"

// Enums for bubble types and operand types
typedef enum {
    BUBBLE_NONE,         // 不是气泡
    BUBBLE_BAD_PREDICTION, // 错误预测
    BUBBLE_FRONTEND,     // 前端气泡
    BUBBLE_BACKEND       // 后端气泡
} bubble_type_t;

typedef enum {
    OPERAND_TYPE_REGISTER,
    OPERAND_TYPE_MEMORY,
    OPERAND_TYPE_IMMEDIATE
} operand_type_t;

// Operand structure
typedef struct _operand_t {
    operand_type_t type;     // 操作数类型：寄存器、内存、立即数
    bool is_source;          // 是否是源操作数
    bool is_dest;            // 是否是目标操作数
    union {
        int reg;             // 如果是寄存器，存储寄存器编号或名称
        void *mem_addr;      // 如果是内存，存储内存地址
        int imm_val;         // 如果是立即数，存储立即数值
    } value;
} operand_t;

// Instruction reference structure
typedef struct _ins_ref_t {
    app_pc pc;                // 指令地址
    int opcode;               // 操作码
    bool is_cbr;              // 是否是条件跳转指令
    app_pc target_addr;       // 跳转的目标地址
    app_pc fall_addr;         // 默认执行下一条指令的地址
    int num_operands;         // 操作数个数
    operand_t operands[4];    // 操作数特性（假设最多4个操作数，可以根据需要调整）
    bubble_type_t bubble_type; // 气泡类型，全部是BUBBLE_NONE
} ins_ref_t;

// Function to insert save opcode
static void insert_save_opcode(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                               reg_id_t scratch, int opcode) {
    scratch = reg_resize_to_opsz(scratch, OPSZ_2);
    MINSERT(ilist, where,
            XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                  OPND_CREATE_INT16(opcode)));
    MINSERT(ilist, where,
            XINST_CREATE_store_2bytes(
                drcontext, OPND_CREATE_MEM16(base, offsetof(ins_ref_t, opcode)),
                opnd_create_reg(scratch)));
}

// Function to insert save PC
static void insert_save_pc(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                           reg_id_t scratch, app_pc pc) {
    instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)pc, opnd_create_reg(scratch),
                                     ilist, where, NULL, NULL);
    MINSERT(ilist, where,
            XINST_CREATE_store(drcontext,
                               OPND_CREATE_MEMPTR(base, offsetof(ins_ref_t, pc)),
                               opnd_create_reg(scratch)));
}

// Function to insert save conditional branch flag
static void insert_save_cbr(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                            reg_id_t scratch, bool is_cbr) {
    MINSERT(ilist, where,
            XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                  OPND_CREATE_INT8(is_cbr ? 1 : 0)));
    MINSERT(ilist, where,
            XINST_CREATE_store_1byte(
                drcontext, OPND_CREATE_MEM8(base, offsetof(ins_ref_t, is_cbr)),
                opnd_create_reg(scratch)));
}

// Function to insert save target address
static void insert_save_target_addr(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                                    reg_id_t scratch, app_pc target_addr) {
    instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)target_addr, opnd_create_reg(scratch),
                                     ilist, where, NULL, NULL);
    MINSERT(ilist, where,
            XINST_CREATE_store(drcontext,
                               OPND_CREATE_MEMPTR(base, offsetof(ins_ref_t, target_addr)),
                               opnd_create_reg(scratch)));
}

// Function to insert save fall-through address
static void insert_save_fall_addr(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                                  reg_id_t scratch, app_pc fall_addr) {
    instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)fall_addr, opnd_create_reg(scratch),
                                     ilist, where, NULL, NULL);
    MINSERT(ilist, where,
            XINST_CREATE_store(drcontext,
                               OPND_CREATE_MEMPTR(base, offsetof(ins_ref_t, fall_addr)),
                               opnd_create_reg(scratch)));
}

// Function to insert save operands
static void insert_save_operands(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                                 reg_id_t scratch, int num_operands, operand_t *operands) {
    // Save the number of operands
    MINSERT(ilist, where,
            XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                  OPND_CREATE_INT8(num_operands)));
    MINSERT(ilist, where,
            XINST_CREATE_store_1byte(
                drcontext, OPND_CREATE_MEM8(base, offsetof(ins_ref_t, num_operands)),
                opnd_create_reg(scratch)));

    // Save each operand
    for (int i = 0; i < num_operands; i++) {
        // Save operand type
        MINSERT(ilist, where,
                XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                      OPND_CREATE_INT8(operands[i].type)));
        MINSERT(ilist, where,
                XINST_CREATE_store_1byte(
                    drcontext, OPND_CREATE_MEM8(base, offsetof(ins_ref_t, operands) + i * sizeof(operand_t) + offsetof(operand_t, type)),
                    opnd_create_reg(scratch)));

        // Save is_source
        MINSERT(ilist, where,
                XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                      OPND_CREATE_INT8(operands[i].is_source ? 1 : 0)));
        MINSERT(ilist, where,
                XINST_CREATE_store_1byte(
                    drcontext, OPND_CREATE_MEM8(base, offsetof(ins_ref_t, operands) + i * sizeof(operand_t) + offsetof(operand_t, is_source)),
                    opnd_create_reg(scratch)));

        // Save is_dest
        MINSERT(ilist, where,
                XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                      OPND_CREATE_INT8(operands[i].is_dest ? 1 : 0)));
        MINSERT(ilist, where,
                XINST_CREATE_store_1byte(
                    drcontext, OPND_CREATE_MEM8(base, offsetof(ins_ref_t, operands) + i * sizeof(operand_t) + offsetof(operand_t, is_dest)),
                    opnd_create_reg(scratch)));

        // Save operand value
        switch (operands[i].type) {
        case OPERAND_TYPE_REGISTER:
            MINSERT(ilist, where,
                    XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                          OPND_CREATE_INT32(operands[i].value.reg)));
            MINSERT(ilist, where,
                    XINST_CREATE_store_4bytes(
                        drcontext, OPND_CREATE_MEM32(base, offsetof(ins_ref_t, operands) + i * sizeof(operand_t) + offsetof(operand_t, value.reg)),
                        opnd_create_reg(scratch)));
            break;
        case OPERAND_TYPE_MEMORY:
            instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)operands[i].value.mem_addr, opnd_create_reg(scratch),
                                             ilist, where, NULL, NULL);
            MINSERT(ilist, where,
                    XINST_CREATE_store(drcontext,
                                       OPND_CREATE_MEMPTR(base, offsetof(ins_ref_t, operands) + i * sizeof(operand_t) + offsetof(operand_t, value.mem_addr)),
                                       opnd_create_reg(scratch)));
            break;
        case OPERAND_TYPE_IMMEDIATE:
            MINSERT(ilist, where,
                    XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                          OPND_CREATE_INT32(operands[i].value.imm_val)));
            MINSERT(ilist, where,
                    XINST_CREATE_store_4bytes(
                        drcontext, OPND_CREATE_MEM32(base, offsetof(ins_ref_t, operands) + i * sizeof(operand_t) + offsetof(operand_t, value.imm_val)),
                        opnd_create_reg(scratch)));
            break;
        }
    }
}

// Function to insert save bubble type
static void insert_save_bubble_type(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                                    reg_id_t scratch, bubble_type_t bubble_type) {
    MINSERT(ilist, where,
            XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                  OPND_CREATE_INT8(bubble_type)));
    MINSERT(ilist, where,
            XINST_CREATE_store_1byte(
                drcontext, OPND_CREATE_MEM8(base, offsetof(ins_ref_t, bubble_type)),
                opnd_create_reg(scratch)));
}

// Main instrumentation function
static void instrument_instr(void *drcontext, instrlist_t *ilist, instr_t *where) {
    /* We need two scratch registers */
    reg_id_t reg_ptr, reg_tmp;
    if (drreg_reserve_register(drcontext, ilist, where, NULL, &reg_ptr) !=
            DRREG_SUCCESS ||
        drreg_reserve_register(drcontext, ilist, where, NULL, &reg_tmp) !=
            DRREG_SUCCESS) {
        DR_ASSERT(false); /* cannot recover */
        return;
    }

    insert_load_buf_ptr(drcontext, ilist, where, reg_ptr);

    // Get instruction details
    app_pc pc = instr_get_app_pc(where);
    int opcode = instr_get_opcode(where);
    bool is_cbr = instr_is_cbr(where);
    app_pc target_addr = is_cbr ? instr_get_branch_target_pc(where) : NULL;
    app_pc fall_addr = pc + instr_length(drcontext, where);

    // Retrieve operand details
    int num_operands = instr_num_srcs(where) + instr_num_dsts(where);
    operand_t operands[4]; // Ensure that this matches the max size defined in ins_ref_t
    for (int i = 0; i < instr_num_srcs(where); i++) {
        operands[i].type = instr_operand_is_memory(where, i) ? OPERAND_TYPE_MEMORY :
                           instr_operand_is_immed(where, i) ? OPERAND_TYPE_IMMEDIATE :
                           OPERAND_TYPE_REGISTER;
        operands[i].is_source = true;
        operands[i].is_dest = false;
        if (operands[i].type == OPERAND_TYPE_REGISTER) {
            operands[i].value.reg = opnd_get_reg(instr_get_src(where, i));
        } else if (operands[i].type == OPERAND_TYPE_MEMORY) {
            operands[i].value.mem_addr = opnd_get_disp(instr_get_src(where, i));
        } else if (operands[i].type == OPERAND_TYPE_IMMEDIATE) {
            operands[i].value.imm_val = opnd_get_immed_int(instr_get_src(where, i));
        }
    }
    for (int i = 0; i < instr_num_dsts(where); i++) {
        operands[instr_num_srcs(where) + i].type = instr_operand_is_memory(where, instr_num_srcs(where) + i) ? OPERAND_TYPE_MEMORY :
                                                   instr_operand_is_immed(where, instr_num_srcs(where) + i) ? OPERAND_TYPE_IMMEDIATE :
                                                   OPERAND_TYPE_REGISTER;
        operands[instr_num_srcs(where) + i].is_source = false;
        operands[instr_num_srcs(where) + i].is_dest = true;
        if (operands[instr_num_srcs(where) + i].type == OPERAND_TYPE_REGISTER) {
            operands[instr_num_srcs(where) + i].value.reg = opnd_get_reg(instr_get_dst(where, i));
        } else if (operands[instr_num_srcs(where) + i].type == OPERAND_TYPE_MEMORY) {
            operands[instr_num_srcs(where) + i].value.mem_addr = opnd_get_disp(instr_get_dst(where, i));
        } else if (operands[instr_num_srcs(where) + i].type == OPERAND_TYPE_IMMEDIATE) {
            operands[instr_num_srcs(where) + i].value.imm_val = opnd_get_immed_int(instr_get_dst(where, i));
        }
    }

    // Save instruction details
    insert_save_pc(drcontext, ilist, where, reg_ptr, reg_tmp, pc);
    insert_save_opcode(drcontext, ilist, where, reg_ptr, reg_tmp, opcode);
    insert_save_cbr(drcontext, ilist, where, reg_ptr, reg_tmp, is_cbr);
    insert_save_target_addr(drcontext, ilist, where, reg_ptr, reg_tmp, target_addr);
    insert_save_fall_addr(drcontext, ilist, where, reg_ptr, reg_tmp, fall_addr);
    insert_save_operands(drcontext, ilist, where, reg_ptr, reg_tmp, num_operands, operands);
    insert_save_bubble_type(drcontext, ilist, where, reg_ptr, reg_tmp, BUBBLE_NONE); // or whatever logic you have for bubble_type

    insert_update_buf_ptr(drcontext, ilist, where, reg_ptr, sizeof(ins_ref_t));

    /* Restore scratch registers */
    if (drreg_unreserve_register(drcontext, ilist, where, reg_ptr) != DRREG_SUCCESS ||
        drreg_unreserve_register(drcontext, ilist, where, reg_tmp) != DRREG_SUCCESS)
        DR_ASSERT(false);
}
