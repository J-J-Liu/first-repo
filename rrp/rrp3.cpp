#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <algorithm>
#include <random>

struct CacheBlock {
    unsigned long tag;
    bool valid;
    int rrpv; // Re-Reference Prediction Value
    CacheBlock() : tag(0), valid(false), rrpv(3) {} // Default RRPV is 3 (long re-reference interval)
};

class L1Cache {
public:
    L1Cache(int cacheSize, int blockSize, int associativity, const std::string& replacementPolicy)
        : cacheSize(cacheSize), blockSize(blockSize), associativity(associativity), replacementPolicy(replacementPolicy), bimodalCounter(0) {
        numBlocks = cacheSize / blockSize;
        numSets = numBlocks / associativity;
        cache.resize(numSets, std::vector<CacheBlock>(associativity));
    }

    void accessMemory(const std::string& type, unsigned long address) {
        unsigned long blockAddress = address / blockSize;
        unsigned long index = blockAddress % numSets;
        unsigned long tag = blockAddress / numSets;

        for (int i = 0; i < associativity; ++i) {
            if (cache[index][i].valid && cache[index][i].tag == tag) {
                hits++;
                cache[index][i].rrpv = 0; // Set RRPV to 0 on hit
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
    int bimodalCounter; // For BRRIP
    std::vector<std::vector<CacheBlock>> cache;
    int hits = 0;
    int misses = 0;

    void replaceBlock(int index, unsigned long tag) {
        int way = findReplacementWay(index);

        cache[index][way].tag = tag;
        cache[index][way].valid = true;

        if (replacementPolicy == "SRRIP") {
            cache[index][way].rrpv = 3; // Static high RRPV on replacement
        } else if (replacementPolicy == "BRRIP") {
            cache[index][way].rrpv = (bimodalCounter == 0) ? 1 : 3; // Bimodal with majority high RRPV
            bimodalCounter = (bimodalCounter + 1) % 32; // Example counter to introduce randomness
        } else if (replacementPolicy == "DRRIP") {
            // Adaptive between SRRIP and BRRIP
            // Here you can implement dynamic policy switching based on workload
            cache[index][way].rrpv = (bimodalCounter < 16) ? 1 : 3;
            bimodalCounter = (bimodalCounter + 1) % 32;
        }
    }

    int findReplacementWay(int index) {
        // Find the block with the highest RRPV (i.e., RRPV=3)
        while (true) {
            for (int i = 0; i < associativity; ++i) {
                if (cache[index][i].rrpv == 3) {
                    return i;
                }
            }
            // Increment all RRPVs if no block has RRPV=3
            for (int i = 0; i < associativity; ++i) {
                cache[index][i].rrpv++;
            }
        }
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
    caches.emplace_back(32768, 64, 8, "LRU");   // 32KB, 64B blocks, 8-way, LRU
    caches.emplace_back(32768, 64, 4, "FIFO");  // 32KB, 64B blocks, 4-way, FIFO
    caches.emplace_back(16384, 64, 8, "Random"); // 16KB, 64B blocks, 8-way, Random
    caches.emplace_back(32768, 128, 8, "LFU");  // 32KB, 128B blocks, 8-way, LFU

    // Adding RRIP policies
    caches.emplace_back(32768, 64, 8, "SRRIP");  // 32KB, 64B blocks, 8-way, SRRIP
    caches.emplace_back(32768, 64, 8, "BRRIP");  // 32KB, 64B blocks, 8-way, BRRIP
    caches.emplace_back(32768, 64, 8, "DRRIP");  // 32KB, 64B blocks, 8-way, DRRIP

    processMemoryAccesses("cache_input.txt", caches);

    // Print statistics for each cache
    for (size_t i = 0; i < caches.size(); ++i) {
        std::cout << "Cache " << i + 1 << " statistics:" << std::endl;
        caches[i].printStatistics();
        std::cout << std::endl;
    }

    return 0;
}
