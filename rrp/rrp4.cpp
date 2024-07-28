#include <iostream>
#include <fstream>
#include <vector>
import <unordered_map>
#include <string>
#include <sstream>
#include <algorithm>
#include <random>

struct CacheBlock {
    unsigned long tag;
    bool valid;
    int rrpv; // For RRIP
    CacheBlock() : tag(0), valid(false), rrpv(3) {} // Initialize RRPV to the max value (3 for 2-bit)
};

class L1Cache {
public:
    L1Cache(int cacheSize, int blockSize, int associativity, const std::string& replacementPolicy)
        : cacheSize(cacheSize), blockSize(blockSize), associativity(associativity), replacementPolicy(replacementPolicy) {
        numBlocks = cacheSize / blockSize;
        numSets = numBlocks / associativity;
        cache.resize(numSets, std::vector<CacheBlock>(associativity));
        if (replacementPolicy == "FIFO") {
            fifoQueue.resize(numSets);
        } else if (replacementPolicy == "LFU") {
            lfuList.resize(numSets, std::vector<int>(associativity, 0));
        }
    }

    void accessMemory(const std::string& type, unsigned long address) {
        accessMemory(type, address, false);
    }

    void accessMemory(const std::string& type, unsigned long address, bool isBRRIP) {
        unsigned long blockAddress = address / blockSize;
        unsigned long index = blockAddress % numSets;
        unsigned long tag = blockAddress / numSets;

        for (int i = 0; i < associativity; ++i) {
            if (cache[index][i].valid && cache[index][i].tag == tag) {
                hits++;
                updateRRPV(cache[index], i);
                return;
            }
        }

        misses++;
        insertBlock(cache[index], tag, isBRRIP);
    }

    void accessMemoryDRRIP(const std::string& type, unsigned long address, DRRIP& drrip) {
        unsigned long blockAddress = address / blockSize;
        unsigned long index = blockAddress % numSets;
        unsigned long tag = blockAddress / numSets;

        bool hit = false;
        for (int i = 0; i < associativity; ++i) {
            if (cache[index][i].valid && cache[index][i].tag == tag) {
                hits++;
                updateRRPV(cache[index], i);
                hit = true;
                break;
            }
        }

        if (!hit) {
            misses++;
            insertBlock(cache[index], tag, !drrip.shouldUseSRRIP());
        }

        drrip.updateCounters(drrip.shouldUseSRRIP(), hit);
    }

    void printStatistics() const {
        std::cout << "Cache hits: " << hits << std::endl;
        std::cout << "Cache misses: " << misses << std::endl;
    }

private:
    int cacheSize;
    int blockSize;
    int associativity;
    int numBlocks;
    int numSets;
    std::string replacementPolicy;
    std::vector<std::vector<CacheBlock>> cache;
    std::vector<std::deque<int>> fifoQueue; // For FIFO replacement policy
    std::vector<std::vector<int>> lfuList; // For LFU replacement policy
    std::unordered_map<int, std::vector<int>> lruList; // For LRU replacement policy
    int hits = 0;
    int misses = 0;

    void updateReplacement(int index, int way) {
        if (replacementPolicy == "LRU") {
            auto& lru = lruList[index];
            lru.erase(std::remove(lru.begin(), lru.end(), way), lru.end());
            lru.push_back(way);
        } else if (replacementPolicy == "LFU") {
            auto& lfu = lfuList[index];
            lfu[way] = cache[index][way].frequency;
        }
    }

    void replaceBlock(int index, unsigned long tag) {
        int way = -1;
        for (int i = 0; i < associativity; ++i) {
            if (!cache[index][i].valid) {
                way = i;
                break;
            }
        }

        if (way == -1) {
            if (replacementPolicy == "LRU") {
                way = lruList[index][0];
                lruList[index].erase(lruList[index].begin());
            } else if (replacementPolicy == "FIFO") {
                way = fifoQueue[index].front();
                fifoQueue[index].pop_front();
            } else if (replacementPolicy == "Random") {
                way = rand() % associativity;
            } else if (replacementPolicy == "LFU") {
                way = std::distance(lfuList[index].begin(), std::min_element(lfuList[index].begin(), lfuList[index].end()));
            } else if (replacementPolicy == "SRRIP" || replacementPolicy == "BRRIP" || replacementPolicy == "DRRIP") {
                way = findVictim(cache[index]);
                if (way == -1) {
                    incrementRRPVs(cache[index]);
                    way = findVictim(cache[index]);
                }
            } else {
                way = 0; // Default to 0 if no valid replacement policy is set
            }
        }

        cache[index][way].tag = tag;
        cache[index][way].valid = true;
        cache[index][way].frequency = 1;
        cache[index][way].rrpv = (replacementPolicy == "BRRIP") ? 3 : 2; // BRRIP inserts with RRPV=3, otherwise RRPV=2
        updateReplacement(index, way);

        if (replacementPolicy == "FIFO") {
            fifoQueue[index].push_back(way);
        }
    }

    void incrementRRPVs(std::vector<CacheBlock>& set) {
        for (auto& block : set) {
            if (block.valid && block.rrpv < 3) {
                block.rrpv++;
            }
        }
    }

    int findVictim(const std::vector<CacheBlock>& set) {
        int victim = -1;
        for (int i = 0; i < set.size(); ++i) {
            if (!set[i].valid) {
                return i;
            }
            if (set[i].rrpv == 3) {
                victim = i;
            }
        }
        return victim;
    }

    void insertBlock(std::vector<CacheBlock>& set, unsigned long tag, bool isBRRIP) {
        int victim = findVictim(set);
        if (victim == -1) {
            incrementRRPVs(set);
            victim = findVictim(set);
        }

        set[victim].tag = tag;
        set[victim].valid = true;
        set[victim].rrpv = isBRRIP ? 3 : 2; // BRRIP inserts with RRPV=3, otherwise RRPV=2
    }

    void updateRRPV(std::vector<CacheBlock>& set, int way) {
        set[way].rrpv = 0;
    }
};

class DRRIP {
public:
    DRRIP(int numSets) : psel(0) {
        srrip_sets = std::vector<int>(numSets, 0);
        brrip_sets = std::vector<int>(numSets, 0);
    }

    void updateCounters(bool isSRRIP, bool hit) {
        if (hit) {
            if (isSRRIP) {
                srrip_sets[setIndex]++;
            } else {
                brrip_sets[setIndex]++;
            }
        }

        if (srrip_sets[setIndex] > brrip_sets[setIndex]) {
            psel--;
        } else {
            psel++;
        }
    }

    bool shouldUseSRRIP() const {
        return psel <= 0;
    }

private:
    int psel;
    std::vector<int> srrip_sets;
    std::vector<int> brrip_sets;
    int setIndex; // 当前的缓存集合索引
};

void processMemoryAccesses(const std::string& filename, std::vector<L1Cache>& caches) {
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        unsigned long address;
        int size;
        if (!(iss >> type >> std::hex >> address >> size)) { break; }

        for (auto& cache : caches) {
            if (cache.getReplacementPolicy() == "DRRIP") {
                DRRIP drrip(cache.getNumSets());
                cache.accessMemoryDRRIP(type, address, drrip);
            } else {
                cache.accessMemory(type, address);
            }
        }
    }
}

int main() {
    std::vector<L1Cache> caches;

    // 配置多个L1缓存，并测试不同的替换策略
    caches.emplace_back(32768, 64, 8, "LRU");   // 32KB, 64B blocks, 8-way, LRU
    caches.emplace_back(32768, 64, 8, "FIFO");  // 32KB, 64B blocks, 8-way, FIFO
    caches.emplace_back(16384, 64, 8, "Random"); // 16KB, 64B blocks, 8-way, Random
    caches.emplace_back(32768, 64, 8, "SRRIP");  // 32KB, 64B blocks, 8-way, SRRIP
    caches.emplace_back(32768, 64, 8, "BRRIP");  // 32KB, 64B blocks, 8-way, BRRIP
    caches.emplace_back(32768, 64, 8, "DRRIP");  // 32KB, 64B blocks, 8-way, DRRIP

    processMemoryAccesses("cache_input.txt", caches);

    // 打印每个缓存的统计信息
    for (size_t i = 0; i < caches.size(); ++i) {
        std::cout << "Cache " << i + 1 << " statistics:" << std::endl;
        caches[i].printStatistics();
        std::cout << std::endl;
    }

    return 0;
}
