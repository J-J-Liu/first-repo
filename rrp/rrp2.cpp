#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <algorithm>
#include <random>
#include <deque>

#define MAX_RRPV 3

struct CacheBlock {
    unsigned long tag;
    bool valid;
    int rrpv; // Re-Reference Prediction Value for RRIP
    int frequency; // For LFU
    CacheBlock() : tag(0), valid(false), rrpv(MAX_RRPV), frequency(0) {}
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
        } else if (replacementPolicy == "DRRIP") {
            setDuelingCounters();
        }
    }

    void accessMemory(const std::string& type, unsigned long address) {
        unsigned long blockAddress = address / blockSize;
        unsigned long index = blockAddress % numSets;
        unsigned long tag = blockAddress / numSets;

        for (int i = 0; i < associativity; ++i) {
            if (cache[index][i].valid && cache[index][i].tag == tag) {
                hits++;
                cache[index][i].frequency++;
                cache[index][i].rrpv = 0; // Reset RRPV on hit
                updateReplacement(index, i);
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
    std::vector<std::deque<int>> fifoQueue; // For FIFO replacement policy
    std::vector<std::vector<int>> lfuList; // For LFU replacement policy
    std::unordered_map<int, std::vector<int>> lruList; // For LRU replacement policy
    int hits = 0;
    int misses = 0;
    int psel; // Policy selection counter for DRRIP
    std::vector<int> followerSets; // Sets used to follow the policy selection in DRRIP

    void setDuelingCounters() {
        psel = 0;
        followerSets.resize(numSets);
        std::fill(followerSets.begin(), followerSets.end(), 0);
        // Select a few sets as dedicated sets for BRRIP and SRRIP
        for (int i = 0; i < numSets; i +=  32) {
            followerSets[i] = 1; // BRRIP set
            followerSets[i + 1] = 2; // SRRIP set
        }
    }

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
            } else if (replacementPolicy == "SRRIP" || (replacementPolicy == "DRRIP" && followerSets[index] == 2)) {
                way = findVictimSRRIP(index);
            } else if (replacementPolicy == "BRRIP" || (replacementPolicy == "DRRIP" && followerSets[index] == 1)) {
                way = findVictimBRRIP(index);
            } else if (replacementPolicy == "DRRIP") {
                way = findVictimDRRIP(index);
            } else {
                way = 0; // Default to 0 if no valid replacement policy is set
            }
        }

        cache[index][way].tag = tag;
        cache[index][way].valid = true;
        cache[index][way].frequency = 1;
        cache[index][way].rrpv = (replacementPolicy == "BRRIP" || (replacementPolicy == "DRRIP" && followerSets[index] == 1)) ? MAX_RRPV - 1 : MAX_RRPV;

        updateReplacement(index, way);

        if (replacementPolicy == "FIFO") {
            fifoQueue[index].push_back(way);
        }
    }

    int findVictimSRRIP(int index) {
        while (true) {
            for (int i = 0; i < associativity; ++i) {
                if (cache[index][i].rrpv == MAX_RRPV) {
                    return i;
                }
            }
            for (int i = 0; i < associativity; ++i) {
                cache[index][i].rrpv++;
            }
        }
    }

    int findVictimBRRIP(int index) {
        return findVictimSRRIP(index);
    }

    int findVictimDRRIP(int index) {
        int brripVictim = findVictimBRRIP(index);
        int srripVictim = findVictimSRRIP(index);

        if (psel >= 0) {
            psel--;
            return brripVictim;
        } else {
            psel++;
            return srripVictim;
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
    caches.emplace_back(32768, 64, 8, "SRRIP"); // 32KB, 64B blocks, 8-way, SRRIP
    caches.emplace_back(32768, 64, 8, "BRRIP"); // 32KB, 64B blocks, 8-way, BRRIP
    caches.emplace_back(32768, 64, 8, "DRRIP"); // 32KB, 64B blocks, 8-way, DRRIP

    processMemoryAccesses("cache_input.txt", caches);

    // Print statistics for each cache
    for (size_t i = 0; i < caches.size(); ++i) {
        std::cout << "Cache " << i + 1 << " statistics:" << std::endl;
        caches[i].printStatistics();
        std::cout << std::endl;
    }

    return 0;
}
