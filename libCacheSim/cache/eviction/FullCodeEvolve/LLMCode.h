#include <list>
#include <unordered_map>
#include <stdexcept>

typedef uint64_t obj_id_t;

class CacheManager {
private:
    std::list<obj_id_t> lfu_list; // List to maintain least frequently used items
    std::unordered_map<obj_id_t, std::pair<std::list<obj_id_t>::iterator, int>> map; // Pair of iterator and frequency count
    size_t capacity = 1000000; // Max cache capacity

public:
    // NO CONSTRUCTOR HERE

    bool find(obj_id_t obj_id) {
        auto it = map.find(obj_id);
        if (it == map.end()) {
            return false; // Miss
        }
        // Hit: Update frequency and reposition
        int current_freq = it->second.second;

        // Remove from current position in the list
        lfu_list.erase(it->second.first);
        
        // Increase frequency
        current_freq++;
        it->second.second = current_freq;

        // Insert back to the list based on new frequency
        auto insert_position = find_insert_position(current_freq);
        lfu_list.insert(insert_position, obj_id);
        map[obj_id] = {--insert_position, current_freq}; // Update iterator in the map
        return true;
    }

    void insert(obj_id_t obj_id) {
        if (map.count(obj_id)) {
            find(obj_id); // Update if exists
            return;
        }

        if (map.size() >= capacity) {
            evict(); // Evict if at capacity
        }
        // Insert as new item
        int freq = 1; // New items start with frequency of 1
        lfu_list.push_back(obj_id);
        map[obj_id] = {--lfu_list.end(), freq}; // Store iterator and frequency
    }

    obj_id_t evict() {
        if (lfu_list.empty()) {
            throw std::runtime_error("Cache is empty, nothing to evict.");
        }
        // Remove the least frequently used item
        obj_id_t victim = lfu_list.back();
        lfu_list.pop_back();
        map.erase(victim);
        return victim;
    }

private:
    // Helper function to find the appropriate position to insert based on frequency
    std::list<obj_id_t>::iterator find_insert_position(int freq) {
        for (auto it = lfu_list.begin(); it != lfu_list.end(); ++it) {
            if (map[*it].second > freq) {
                return it; // Found the position where frequency is greater
            }
        }
        return lfu_list.end(); // If not found, insert at the end
    }
};