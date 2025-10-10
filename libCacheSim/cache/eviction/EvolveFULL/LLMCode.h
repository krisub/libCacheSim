#include "../../../include/libCacheSim/cache.h"
#include <stdlib.h>
#include <stdbool.h>
#include <bits/stdc++.h>

// Node structure for the doubly linked list
typedef struct Node {
    cache_obj_t *cache_obj;
    struct Node *prev;
    struct Node *next;
} Node;

// Structure for our custom cache
typedef struct {
    Node *head;            // Head of the LRU list
    Node *tail;            // Tail of the LRU list
    size_t cache_size;     // Maximum size of cache
    size_t current_size;   // Current number of items in cache
    hashmap_t *hash_table; // Hash map for quick lookups
} my_cache_t;

// Function to add a node to the front of the doubly linked list
void add_to_front(my_cache_t *my_cache, Node *new_node) {
    new_node->next = my_cache->head;
    new_node->prev = NULL;
    if (my_cache->head != NULL) {
        my_cache->head->prev = new_node;
    }
    my_cache->head = new_node;
    if (my_cache->tail == NULL) {
        my_cache->tail = new_node;
    }
}

// Function to remove a node from the doubly linked list
void remove_node(my_cache_t *my_cache, Node *node) {
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        my_cache->head = node->next; // This was the head
    }
    
    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        my_cache->tail = node->prev; // This was the tail
    }
    free(node); // Free the removed node
}

// 1. Define a struct to hold your cache's private state.
typedef struct {
    my_cache_t *my_cache;
} eviction_params_t;

// 2. Implement your core logic functions.
bool my_find(cache_t *cache, const request_t *req) {
    my_cache_t *my_cache = (my_cache_t *)cache->eviction_params;

    // Check if the object exists
    cache_obj_t *obj = cache_find_base(cache, req, false);
    if (obj) {
        // Find the corresponding node in the linked list
        Node *node = (Node *)hashmap_get(my_cache->hash_table, req->obj_id);
        remove_node(my_cache, node);
        add_to_front(my_cache, node); // Move to front as it is recently used
        return true; // Hit
    }
    
    return false; // Miss
}

cache_obj_t *my_insert(cache_t *cache, const request_t *req) {
    my_cache_t *my_cache = (my_cache_t *)cache->eviction_params;

    // If cache is full, evict the least recently used item
    if (my_cache->current_size >= my_cache->cache_size) {
        Node *lru_node = my_cache->tail;
        cache_obj_t *to_evict = lru_node->cache_obj;
        remove_node(my_cache, lru_node);
        hashmap_remove(my_cache->hash_table, to_evict->obj_id);
        my_cache->current_size--;
        cache_evict_base(cache, to_evict, true); // Evict from base
    }

    // Insert the new object into the cache
    cache_obj_t *new_obj = cache_insert_base(cache, req);

    // Create and insert the new node in the LRU list
    Node *new_node = malloc(sizeof(Node));
    new_node->cache_obj = new_obj;
    add_to_front(my_cache, new_node);
    hashmap_insert(my_cache->hash_table, new_obj->obj_id, new_node);
    my_cache->current_size++;

    return new_obj; // Return the new object info
}

void my_free(cache_t *cache) {
    my_cache_t *my_cache = (my_cache_t *)cache->eviction_params;

    // Free all nodes in the linked list
    Node *current = my_cache->head;
    while (current != NULL) {
        Node *next = current->next;
        free(current);
        current = next;
    }
    
    // Free hash table
    hashmap_free(my_cache->hash_table);
    free(my_cache); // Free my_cache
    cache_struct_free(cache); // Finally, free the base cache struct itself
}

// 3. Implement the mandatory initialization function.
extern "C" {
    cache_t *EvolveFULL_init(common_cache_params_t params, void *init_params) {
        // A. Initialize the base cache structure.
        cache_t *cache = cache_struct_init("EvolveFULL", params, init_params);

        // B. Allocate and initialize your private data structure.
        my_cache_t *my_cache = (my_cache_t *)malloc(sizeof(my_cache_t));
        my_cache->cache_size = params.cache_size; // Assume params has cache_size
        my_cache->current_size = 0;
        my_cache->head = NULL;
        my_cache->tail = NULL;
        my_cache->hash_table = hashmap_create(); // Create a hash map for storage

        // C. Store your private data in the cache struct.
        cache->eviction_params = my_cache;

        // D. Assign your custom functions to the cache's function pointers.
        cache->find = my_find;
        cache->insert = my_insert;
        cache->cache_free = my_free;

        return cache;
    }
}