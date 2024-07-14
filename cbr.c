#include <stddef.h>
#include "dr_api.h"
#include "drmgr.h"
#include "drreg.h"
#include "utils.h"

typedef struct _ins_ref_t {
    app_pc pc;
    int opcode;
    bool is_conditional_branch;
    opnd_t src[4];
    opnd_t dst[4];
} ins_ref_t;

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
insert_save_conditional_branch(void *drcontext, instrlist_t *ilist, instr_t *where,
                               reg_id_t base, reg_id_t scratch, bool is_conditional_branch)
{
    instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)is_conditional_branch, opnd_create_reg(scratch),
                                     ilist, where, NULL, NULL);
    MINSERT(ilist, where,
            XINST_CREATE_store(drcontext,
                               OPND_CREATE_MEMPTR(base, offsetof(ins_ref_t, is_conditional_branch)),
                               opnd_create_reg(scratch)));
}

static void
insert_save_src_opnd(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                     reg_id_t scratch, int index, opnd_t src_opnd)
{
    instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)src_opnd, opnd_create_reg(scratch),
                                     ilist, where, NULL, NULL);
    MINSERT(ilist, where,
            XINST_CREATE_store(drcontext,
                               OPND_CREATE_MEMPTR(base, offsetof(ins_ref_t, src[index])),
                               opnd_create_reg(scratch)));
}

static void
insert_save_dst_opnd(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                     reg_id_t scratch, int index, opnd_t dst_opnd)
{
    instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)dst_opnd, opnd_create_reg(scratch),
                                     ilist, where, NULL, NULL);
    MINSERT(ilist, where,
            XINST_CREATE_store(drcontext,
                               OPND_CREATE_MEMPTR(base, offsetof(ins_ref_t, dst[index])),
                               opnd_create_reg(scratch)));
}

/* insert inline code to add an instruction entry into the buffer */
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

    insert_load_buf_ptr(drcontext, ilist, where, reg_ptr);
    insert_save_pc(drcontext, ilist, where, reg_ptr, reg_tmp, instr_get_app_pc(where));
    insert_save_opcode(drcontext, ilist, where, reg_ptr, reg_tmp, instr_get_opcode(where));
    
    bool is_conditional_branch = instr_is_cbr(where); // 判断是否为条件分支
    insert_save_conditional_branch(drcontext, ilist, where, reg_ptr, reg_tmp, is_conditional_branch);

    int num_srcs = instr_num_srcs(where);
    for (int i = 0; i < num_srcs; i++) {
        insert_save_src_opnd(drcontext, ilist, where, reg_ptr, reg_tmp, i, instr_get_src(where, i));
    }

    int num_dsts = instr_num_dsts(where);
    for (int i = 0; i < num_dsts; i++) {
        insert_save_dst_opnd(drcontext, ilist, where, reg_ptr, reg_tmp, i, instr_get_dst(where, i));
    }

    insert_update_buf_ptr(drcontext, ilist, where, reg_ptr, sizeof(ins_ref_t));

    /* Restore scratch registers */
    if (drreg_unreserve_register(drcontext, ilist, where, reg_ptr) != DRREG_SUCCESS ||
        drreg_unreserve_register(drcontext, ilist, where, reg_tmp) != DRREG_SUCCESS)
        DR_ASSERT(false);
}
