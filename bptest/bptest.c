#include "branch_predictor.h"
#include <vector>

static int correct_predictions = 0;
static int total_predictions = 0;

// 可以定义多种分支预测器
class BranchPredictor {
public:
    virtual bool predict(instr_t *instr) = 0;
    virtual void update(instr_t *instr, bool taken) = 0;
};

class StaticPredictor : public BranchPredictor {
public:
    bool predict(instr_t *instr) override {
        return true; // 永远预测分支被采取
    }

    void update(instr_t *instr, bool taken) override {
        // 静态预测器不需要更新
    }
};

// 其他预测器的实现...

static std::vector<BranchPredictor*> predictors;

void branch_predictor_init() {
    predictors.push_back(new StaticPredictor());
    // 初始化其他预测器...
}

void branch_predictor_exit() {
    for (auto predictor : predictors) {
        delete predictor;
    }
    predictors.clear();
    dr_fprintf(STDERR, "Correct Predictions: %d\n", correct_predictions);
    dr_fprintf(STDERR, "Total Predictions: %d\n", total_predictions);
    dr_fprintf(STDERR, "Accuracy: %.2f%%\n", (100.0 * correct_predictions / total_predictions));
}

void branch_predictor_instrument_branch(void *drcontext, instrlist_t *bb, instr_t *instr) {
    dr_insert_clean_call(drcontext, bb, instr, (void *)branch_predictor_update, false, 2,
                         OPND_CREATE_INTPTR(instr), OPND_CREATE_INTPTR(instr_is_taken(instr)));
}

void branch_predictor_update(void *drcontext, instr_t *instr, bool taken) {
    for (auto predictor : predictors) {
        bool prediction = predictor->predict(instr);
        if (prediction == taken) {
            correct_predictions++;
        }
        total_predictions++;
        predictor->update(instr, taken);
    }
}
