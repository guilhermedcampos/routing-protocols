/******************************************************************************\
* Path vector routing protocol.                                                *
\******************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "routing-simulator.h"

// Message format to send between nodes.
typedef struct message_t {
    cost_t data[MAX_NODES]; // The distance vector from the sender
    node_t path[MAX_NODES][MAX_NODES]; // The path vector from the sender
} message_t;

// State format.
typedef struct state_t {
    cost_t neighbor_costs[MAX_NODES][MAX_NODES]; // Cost to each destination via each neighbor
    node_t paths[MAX_NODES][MAX_NODES][MAX_NODES]; // Paths to edach destination via each neighbor
} state_t;

void broadcast_message(state_t *state) {
    message_t message;
    for (node_t n = get_first_node(); n <= get_last_node(); n = get_next_node(n)) {
        if (get_link_cost(n) < COST_INFINITY && n != get_current_node()) {
            for (node_t dest = get_first_node(); dest <= get_last_node(); dest = get_next_node(dest)) {
                message.data[dest] = state->neighbor_costs[get_current_node()][dest];
                memcpy(message.path[dest], state->paths[get_current_node()][dest], sizeof(message.path[dest]));
            }
            printf("BM: Node %d: Sending message to neighbor %d\n", get_current_node(), n);
            send_message(n, &message, sizeof(message));
        }
    }
}
// Helper function to check if a path contains a cycle
int contains_cycle(node_t *path, node_t node) {
    for (int i = 0; i < MAX_NODES; i++) {
        if (path[i] == -1) break; // End of path
        if (path[i] == node) return 1; // Node is already in the path
    }
    return 0;
}

// Initialize the state
state_t *init_state() {
    printf("Initializing node %d\n", get_current_node());
    state_t *state = (state_t *)calloc(1, sizeof(state_t));

    for (node_t n = get_first_node(); n <= get_last_node(); n = get_next_node(n)) {
        for (node_t dest = get_first_node(); dest <= get_last_node(); dest = get_next_node(dest)) {
            if (n == dest) {
                state->neighbor_costs[n][dest] = 0;
            } else {
                state->neighbor_costs[n][dest] = COST_INFINITY;
            }
            for (node_t next = get_first_node(); next <= get_last_node(); next = get_next_node(next)) {
                state->paths[n][dest][next] = -1;
            }
        }
    }
    return state;
}


// Recalculate the distance vector using Bellman-Ford
int recalculate_distance_vector(state_t *state) {
    int updated = 0;
    node_t current_node = get_current_node();

    // copy current state->paths
    node_t paths_copy[MAX_NODES][MAX_NODES][MAX_NODES];
    memcpy(paths_copy, state->paths, sizeof(paths_copy));

    for (node_t dest = get_first_node(); dest <= get_last_node(); dest = get_next_node(dest)) {
        if (dest == current_node) continue;

        cost_t best_cost = COST_INFINITY;
        node_t best_next_hop = -1;
        node_t best_path[MAX_NODES];
        memset(best_path, -1, sizeof(best_path)); // Reset the best path

        for (node_t neighbor = get_first_node(); neighbor <= get_last_node(); neighbor = get_next_node(neighbor)) {
            if (neighbor == current_node || get_link_cost(neighbor) >= COST_INFINITY) continue;

            cost_t cost_via_neighbor = COST_ADD(get_link_cost(neighbor), state->neighbor_costs[neighbor][dest]);

            // Skip paths that create cycles
            if (contains_cycle(state->paths[neighbor][dest], current_node)) {
                //printf("  Skipping path through %d to %d due to cycle\n", neighbor, dest);
                continue;
            }

            // Consider this path only if it offers a better cost
            if (cost_via_neighbor < best_cost) {
                best_cost = cost_via_neighbor;
                best_next_hop = neighbor;

                // Construct the new path
                memset(best_path, -1, sizeof(best_path)); // Reset path
                best_path[0] = current_node;
                int path_index = 1;
                for (int i = 0; i < MAX_NODES && state->paths[neighbor][dest][i] != -1; i++) {
                    best_path[path_index++] = state->paths[neighbor][dest][i];
                }
            }
        }

        // Update the state if the best cost or path changed
        if (best_cost != state->neighbor_costs[current_node][dest] ||
            memcmp(state->paths[current_node][dest], best_path, sizeof(best_path)) != 0) {
            printf("  Updating path to %d: cost = %d, next hop = %d\n", dest, best_cost, best_next_hop);
            state->neighbor_costs[current_node][dest] = best_cost;
            memcpy(state->paths[current_node][dest], best_path, sizeof(best_path));
            set_route(dest, best_next_hop, best_cost);
            updated = 1;

            // Print the new path
            printf("  Path to %d is: ", dest);
            for (int i = 0; i < MAX_NODES; i++) {
                if (best_path[i] == -1) break;
                printf("%d ", best_path[i]);
            }
            printf("\n");
        }
    }

    // if state->paths didnt change, return 0  
    if (memcmp(paths_copy, state->paths, sizeof(paths_copy)) == 0) {
        return 0;
    }

    return updated;
}

// Invalidate routes that use the specified neighbor
void invalidate_route(node_t current_node, node_t neighbor) {
    state_t *state = get_state();
    for (node_t dest = get_first_node(); dest <= get_last_node(); dest = get_next_node(dest)) {
        // Check if the path to the destination uses the neighbor
        if (state->paths[current_node][dest][0] == neighbor) {
            state->neighbor_costs[current_node][dest] = COST_INFINITY;
            memset(state->paths[current_node][dest], -1, sizeof(state->paths[current_node][dest]));
            set_route(dest, -1, COST_INFINITY);
            printf("Invalidating path to %d via %d\n", dest, neighbor);
        }
    }
}

// Notify a node that a neighboring link has changed cost
void notify_link_change(node_t neighbor, cost_t new_cost) {
    state_t *state = get_state();
    node_t current_node = get_current_node();
    printf("LC: Node %d: Link to neighbor %d changed to cost %d\n", current_node, neighbor, new_cost);

    // Update the link cost
    state->neighbor_costs[current_node][neighbor] = new_cost;

    // Invalidate paths if the link is removed
    if (new_cost == COST_INFINITY) {
        invalidate_route(current_node, neighbor);
    }

    // Recalculate the distance vector
    if (recalculate_distance_vector(state)) {
        // Broadcast the updated path vector to neighbors
        broadcast_message(state);
    }
}

// Receive a message sent by a neighboring node
void notify_receive_message(node_t sender, void *message, size_t length) {
    state_t *state = get_state();
    message_t *received_message = (message_t *)message;

    // Update route costs
    for (node_t n = get_first_node(); n <= get_last_node(); n = get_next_node(n)) {
        state->neighbor_costs[sender][n] = received_message->data[n];
    }

    // Update paths from sender to every destination
    for (node_t n = get_first_node(); n <= get_last_node(); n = get_next_node(n)) {
        for (node_t dest = get_first_node(); dest <= get_last_node(); dest = get_next_node(dest)) {
            state->paths[sender][n][dest] = received_message->path[n][dest];
        }
    }

    // Recalculate the distance vector
    if (recalculate_distance_vector(state)) {
        // Broadcast the updated path vector to neighbors
        broadcast_message(state);
    }
}
