#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <algorithm>
#include <deque>
#include <random>

struct CacheBlock {
    unsigned long tag;
    bool valid;
    int rrpv; // Re-reference prediction value for RRIP
    CacheBlock() : tag(0), valid(false), rrpv(0) {}
};

class L1Cache {
public:
    L1Cache(int cacheSize, int blockSize, int associativity, const std::string& replacementPolicy)
        : cacheSize(cacheSize), blockSize(blockSize), associativity(associativity), replacementPolicy(replacementPolicy) {
        numBlocks = cacheSize / blockSize;
        numSets = numBlocks / associativity;
        cache.resize(numSets, std::vector<CacheBlock>(associativity));
        if (replacementPolicy == "DRRIP") {
            bripInsertProbability = 0.1; // Initial BRRIP insertion probability
            bripDynamicAdjust = 32;      // Adjust threshold
        }
    }

    void accessMemory(const std::string& type, unsigned long address) {
        unsigned long blockAddress = address / blockSize;
        unsigned long index = blockAddress % numSets;
        unsigned long tag = blockAddress / numSets;

        for (int i = 0; i < associativity; ++i) {
            if (cache[index][i].valid && cache[index][i].tag == tag) {
                hits++;
                cache[index][i].rrpv = 0;
                return;
            }
        }

        misses++;
        replaceBlock(index, tag);
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
    int hits = 0;
    int misses = 0;

    // For DRRIP
    double bripInsertProbability;
    int bripDynamicAdjust;

    int findVictim(int index) {
        int maxRRPV = 3; // Assuming 2-bit RRPV
        int victim = -1;
        while (victim == -1) {
            for (int i = 0; i < associativity; ++i) {
                if (cache[index][i].rrpv == maxRRPV) {
                    victim = i;
                    break;
                }
            }
            if (victim == -1) {
                for (int i = 0; i < associativity; ++i) {
                    cache[index][i].rrpv++;
                }
            }
        }
        return victim;
    }

    void replaceBlock(int index, unsigned long tag) {
        int victim = findVictim(index);

        if (replacementPolicy == "SRRIP") {
            cache[index][victim].rrpv = 2; // Static RRIP with RRPV = 2
        } else if (replacementPolicy == "BRRIP") {
            if (rand() % 100 < bripInsertProbability * 100) {
                cache[index][victim].rrpv = 2; // Infrequent with RRPV = 2
            } else {
                cache[index][victim].rrpv = 3; // Frequent with RRPV = 3
            }
        } else if (replacementPolicy == "DRRIP") {
            static int srripMisses = 0;
            static int brripMisses = 0;

            if (srripMisses < bripDynamicAdjust) {
                cache[index][victim].rrpv = 2;
                srripMisses++;
            } else if (brripMisses < bripDynamicAdjust) {
                if (rand() % 100 < bripInsertProbability * 100) {
                    cache[index][victim].rrpv = 2;
                } else {
                    cache[index][victim].rrpv = 3;
                }
                brripMisses++;
            }

            if (srripMisses == bripDynamicAdjust && brripMisses == bripDynamicAdjust) {
                if (srripMisses < brripMisses) {
                    srripMisses = 0;
                    brripMisses = 0;
                } else {
                    bripInsertProbability = std::min(1.0, bripInsertProbability + 0.05);
                    srripMisses = 0;
                    brripMisses = 0;
                }
            }
        }

        cache[index][victim].tag = tag;
        cache[index][victim].valid = true;
        cache[index][victim].rrpv = 0; // Reset RRPV to 0 after replacement
    }
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
            cache.accessMemory(type, address);
        }
    }
}

int main() {
    std::vector<L1Cache> caches;

    // Configure multiple L1 caches with different parameters and replacement policies
    caches.emplace_back(32768, 64, 8, "SRRIP");  // 32KB, 64B blocks, 8-way, SRRIP
    caches.emplace_back(32768, 64, 8, "BRRIP");  // 32KB, 64B blocks, 8-way, BRRIP
    caches.emplace_back(32768, 64, 8, "DRRIP");  // 32KB, 64B blocks, 8-way, DRRIP

    processMemoryAccesses("cache_input.txt", caches);

    // Print statistics for each cache
    for (size_t i = 0; i < caches.size(); ++i) {
        std::cout << "Cache " << i + 1 << " statistics (" << caches[i].replacementPolicy << "):" << std::endl;
        caches[i].printStatistics();
        std::cout << std::endl;
    }

    return 0;
}
