/******************************************************************************\
* Distance vector routing protocol with reverse path poisoning.                *
\******************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "routing-simulator.h"

// Message format to send between nodes.
typedef struct {
    cost_t distance_vector[MAX_NODES]; // The distance vector from the sender
} data_t;

// State format
typedef struct state_t {
    cost_t distance_vector[MAX_NODES];          // Current node's distance vector
    cost_t neighbor_costs[MAX_NODES][MAX_NODES]; // Neighbors' distance vectors to destinations
    node_t best_next_hop[MAX_NODES];
} state_t;

// Print distance vector (for debugging)
void print_distance_vector(state_t *state) {
    printf("Node %d: Distance vector:\n", get_current_node());
    for (node_t n = get_first_node(); n <= get_last_node(); n = get_next_node(n)) {
        if (n == get_current_node()) continue;
        printf("  To %d: %d\n", n, state->distance_vector[n]);
    }
}

// Initialize the state
state_t *init_state() {
    printf("Initializing node %d\n", get_current_node());
    state_t *state = (state_t *)calloc(1, sizeof(state_t));

    for (node_t i = get_first_node(); i <= get_last_node(); i = get_next_node(i)) {
        state->distance_vector[i] = COST_INFINITY;
        state->best_next_hop[i] = -1;
        for (node_t j = get_first_node(); j <= get_last_node(); j = get_next_node(j)) {
            state->neighbor_costs[i][j] = COST_INFINITY;
            if (i == j) {
                state->neighbor_costs[i][j] = 0;
            }
        }
    }

    node_t current_node = get_current_node();
    state->distance_vector[current_node] = 0;
    print_distance_vector(state);
    return state;
}

void broadcast_message(state_t *state) {
    // Prepare to send messages to neighbors
    for (node_t n = get_first_node(); n <= get_last_node(); n = get_next_node(n)) {
        // Only consider valid neighbors
        if (get_link_cost(n) < COST_INFINITY && n != get_current_node()) {
            data_t outgoing_data;
            memcpy(outgoing_data.distance_vector, state->distance_vector, sizeof(outgoing_data.distance_vector));

            // Reverse Path Poisoning: Set costs to destinations where 'n' is the next hop to COST_INFINITY
            for (node_t dest = get_first_node(); dest <= get_last_node(); dest = get_next_node(dest)) {
                if (state->best_next_hop[dest] == n) {
                    outgoing_data.distance_vector[dest] = COST_INFINITY;
                } 
            }

            printf("BM: Node %d: Sending message to neighbor %d\n", get_current_node(), n);
            send_message(n, &outgoing_data, sizeof(outgoing_data));
        }
    }
}

// Recalculate the distance vector using Bellman-Ford
int recalculate_distance_vector(state_t *state) {
    int updated = 0;
    node_t current_node = get_current_node();

    for (node_t dest = get_first_node(); dest <= get_last_node(); dest = get_next_node(dest)) {
        if (dest == current_node) continue;

        printf("Lets calculate the best distance to %d from node %d\n", dest, current_node);

        cost_t best_cost = get_link_cost(dest);
        printf("  Initial best cost: %d\n", best_cost);
        node_t best_next_hop = -1;

        for (node_t n = get_first_node(); n <= get_last_node(); n = get_next_node(n)) {
            if (n == current_node) continue;

            printf("  Distance to neighbor%d is %d, cost from neighbor%d to dest%d is %d\n", n, get_link_cost(n), n, dest, state->neighbor_costs[n][dest]);
            cost_t cost_via_n = COST_ADD(get_link_cost(n), state->neighbor_costs[n][dest]);
            if (cost_via_n <= best_cost) {
                best_cost = cost_via_n;
                best_next_hop = n;
            }
        }

        if (best_cost != state->distance_vector[dest]) {
            printf("  Best cost to %d is %d via %d\n", dest, best_cost, best_next_hop);
            state->distance_vector[dest] = best_cost;
            state->best_next_hop[dest] = best_next_hop;
            printf("Current node %d: Next hop to %d is %d\n", current_node, dest, best_next_hop);
            printf("Node %d, next hop is %d\n", current_node, state->best_next_hop[current_node]);
            set_route(dest, best_next_hop, best_cost);
            updated = 1;
        }
    }

    return updated;
}

// Notify a node that a neighboring link has changed cost
void notify_link_change(node_t neighbor, cost_t new_cost) {
    state_t *state = get_state();
    printf("LC: Node %d: Link to neighbor %d changed to cost %d\n", get_current_node(), neighbor, new_cost);
    if (new_cost == COST_INFINITY) {
        printf("\n");

    } 

    state->neighbor_costs[get_current_node()][neighbor] = new_cost;

    if (recalculate_distance_vector(state)) {
        printf("LC: Node %d: Distance vector updated after link cost change.\n", get_current_node());
        print_distance_vector(state);

        broadcast_message(state);
    }
}

// Receive a message sent by a neighboring node
void notify_receive_message(node_t sender, void *message, size_t length) {
    printf("RM: Node %d: Received message from node %d\n", get_current_node(), sender);
    state_t *state = get_state();
    data_t *received_data = (data_t *)message;

    for (node_t dest = get_first_node(); dest <= get_last_node(); dest = get_next_node(dest)) {
        state->neighbor_costs[sender][dest] = received_data->distance_vector[dest];
    }

    if (recalculate_distance_vector(state)) {
        printf("RM: Node %d: Distance vector updated after receiving message.\n", get_current_node());
        print_distance_vector(state);

        broadcast_message(state);
    }
}

