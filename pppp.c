class StaticPredictor : public BranchPredictor {
public:
    StaticPredictor() : BranchPredictor("StaticPredictor") {}

    bool predict(instr_t *instr) override {
        return true; // 永远预测分支被采取
    }

    void update(instr_t *instr, bool taken) override {
        // 静态预测器不需要更新
    }
};

// 新增的分支预测器类
class TwoBitPredictor : public BranchPredictor {
public:
    TwoBitPredictor() : BranchPredictor("TwoBitPredictor") {
        state = 0b11; // 初始状态：强taken
    }

    bool predict(instr_t *instr) override {
        // 预测为 taken 当 state 的高位为1
        return state & 0b10;
    }

    void update(instr_t *instr, bool taken) override {
        // 更新两位状态机
        if (taken) {
            if (state != 0b11) state++;
        } else {
            if (state != 0b00) state--;
        }
    }

private:
    unsigned char state; // 两位状态机状态
};

class BackwardJumpPredictor : public BranchPredictor {
public:
    BackwardJumpPredictor() : BranchPredictor("BackwardJumpPredictor") {}

    bool predict(instr_t *instr) override {
        // 检查分支目标地址是否小于当前指令地址
        return instr_get_branch_target_pc(instr) < instr_get_app_pc(instr);
    }

    void update(instr_t *instr, bool taken) override {
        // 后向跳转预测不需要更新
    }
};


#include <unordered_map>

class LocalHistoryPredictor : public BranchPredictor {
public:
    LocalHistoryPredictor() : BranchPredictor("LocalHistoryPredictor") {}

    bool predict(instr_t *instr) override {
        unsigned history = get_local_history(instr);
        return local_pattern_table[history] & 0b10; // 预测为 taken 当状态的高位为1
    }

    void update(instr_t *instr, bool taken) override {
        unsigned history = get_local_history(instr);
        // 更新两位状态机
        if (taken) {
            if (local_pattern_table[history] != 0b11) local_pattern_table[history]++;
        } else {
            if (local_pattern_table[history] != 0b00) local_pattern_table[history]--;
        }
        update_local_history(instr, taken);
    }

private:
    std::unordered_map<instr_t*, unsigned> local_histories; // 局部分支历史
    std::unordered_map<unsigned, unsigned char> local_pattern_table; // 局部模式表，两位状态机

    unsigned get_local_history(instr_t *instr) {
        if (local_histories.find(instr) == local_histories.end()) {
            local_histories[instr] = 0;
        }
        return local_histories[instr];
    }

    void update_local_history(instr_t *instr, bool taken) {
        local_histories[instr] = ((local_histories[instr] << 1) | taken) & 0b1111; // 仅保留最近的4次历史
    }
};


#include <unordered_map>
#include <vector>

class TAGEPredictor : public BranchPredictor {
public:
    TAGEPredictor() : BranchPredictor("TAGEPredictor") {
        // 初始化每个表的历史长度和大小
        history_lengths = {4, 8, 16, 32}; // 可以根据需要调整历史长度
        table_size = 128; // 每个模式历史表的大小，可以根据需要调整
        for (unsigned len : history_lengths) {
            tables.push_back(std::vector<Entry>(table_size));
        }
    }

    bool predict(instr_t *instr) override {
        unsigned pc = get_pc(instr);
        unsigned index = 0;
        int table_index = -1;

        // 查找匹配的条目
        for (int i = history_lengths.size() - 1; i >= 0; --i) {
            index = get_index(pc, local_histories[pc], i);
            if (tables[i][index].valid && tables[i][index].tag == get_tag(pc, i)) {
                table_index = i;
                break;
            }
        }

        // 如果找到了匹配的条目，则返回其预测值，否则使用默认预测
        if (table_index != -1) {
            return tables[table_index][index].counter & 0b10;
        } else {
            return bimodal_predictor[pc & (table_size - 1)] & 0b10; // 默认使用简单的bimodal预测
        }
    }

    void update(instr_t *instr, bool taken) override {
        unsigned pc = get_pc(instr);
        unsigned index = 0;
        int table_index = -1;

        // 查找匹配的条目
        for (int i = history_lengths.size() - 1; i >= 0; --i) {
            index = get_index(pc, local_histories[pc], i);
            if (tables[i][index].valid && tables[i][index].tag == get_tag(pc, i)) {
                table_index = i;
                break;
            }
        }

        // 更新找到的匹配条目
        if (table_index != -1) {
            if (taken) {
                if (tables[table_index][index].counter != 0b11) tables[table_index][index].counter++;
            } else {
                if (tables[table_index][index].counter != 0b00) tables[table_index][index].counter--;
            }
        }

        // 如果没有找到匹配的条目，使用默认的bimodal预测更新
        else {
            index = pc & (table_size - 1);
            if (taken) {
                if (bimodal_predictor[index] != 0b11) bimodal_predictor[index]++;
            } else {
                if (bimodal_predictor[index] != 0b00) bimodal_predictor[index]--;
            }
        }

        // 更新局部分支历史
        update_local_history(pc, taken);

        // 插入新条目到TAGE表中
        for (int i = 0; i < history_lengths.size(); ++i) {
            index = get_index(pc, local_histories[pc], i);
            if (!tables[i][index].valid) {
                tables[i][index].valid = true;
                tables[i][index].tag = get_tag(pc, i);
                tables[i][index].counter = taken ? 0b10 : 0b01;
                break;
            }
        }
    }

private:
    struct Entry {
        bool valid = false;
        unsigned tag = 0;
        unsigned char counter = 0b01; // 两位饱和计数器，初始值为弱不 taken
    };

    std::vector<unsigned> history_lengths; // 历史长度
    unsigned table_size; // 表的大小
    std::unordered_map<unsigned, unsigned> local_histories; // 局部分支历史
    std::vector<std::vector<Entry>> tables; // 模式历史表
    std::vector<unsigned char> bimodal_predictor = std::vector<unsigned char>(table_size, 0b01); // 简单的bimodal预测器

    unsigned get_pc(instr_t *instr) {
        // 获取指令的PC值，这里假设instr有一个get_pc方法
        return instr->get_pc();
    }

    unsigned get_index(unsigned pc, unsigned history, int table_index) {
        // 计算索引，可以根据需要使用不同的哈希函数
        return (pc ^ history) % table_size;
    }

    unsigned get_tag(unsigned pc, int table_index) {
        // 计算标签，可以根据需要使用不同的哈希函数
        return pc ^ (table_index * 31);
    }

    void update_local_history(unsigned pc, bool taken) {
        if (local_histories.find(pc) == local_histories.end()) {
            local_histories[pc] = 0;
        }
        local_histories[pc] = ((local_histories[pc] << 1) | taken) & ((1 << history_lengths.back()) - 1); // 保留最长历史长度的位数
    }
};
