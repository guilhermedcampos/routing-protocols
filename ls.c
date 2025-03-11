/******************************************************************************\
* Link state routing protocol.                                                 *
\******************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "routing-simulator.h"

typedef struct link_state_t {
  cost_t link_cost[MAX_NODES];
  int version;
} link_state_t;

// Message format to send between nodes.
typedef struct data_t {
  link_state_t ls[MAX_NODES];
} data_t;

// State format.
typedef struct state_t {
  link_state_t link_states[MAX_NODES];
} state_t;

// Handler for the node to allocate and initialize its state.
state_t *init_state() {
    state_t *state = (state_t *)calloc(1, sizeof(state_t));

    for (node_t n = get_first_node(); n <= get_last_node(); n = get_next_node(n)) {
        state->link_states[n].version = 0;

        for (node_t dest = get_first_node(); dest <= get_last_node(); dest = get_next_node(dest)) {

            if (n == get_current_node()) {  // If local node
                state->link_states[n].link_cost[dest] = get_link_cost(dest);
            }
            else if (n == dest) { // If node to itself
                state->link_states[n].link_cost[dest] = 0;
            }
            else {
                state->link_states[n].link_cost[dest] = COST_INFINITY;
            }
        }
    }
    return state;
}


void broadcast_message(state_t *state) {
    data_t outgoing_data;
    for (node_t n = get_first_node(); n <= get_last_node(); n = get_next_node(n)) {
        if (get_link_cost(n) < COST_INFINITY && n != get_current_node()) {
            memcpy(outgoing_data.ls, state->link_states, sizeof(outgoing_data.ls));
            printf("BM: Node %d: Sending message to neighbor %d\n", get_current_node(), n);
            send_message(n, &outgoing_data, sizeof(outgoing_data));
        }
    }
}

void run_dijkstra(state_t *state) {
    node_t current_node = get_current_node();
    cost_t dist[MAX_NODES];
    int visited[MAX_NODES];
    node_t pred[MAX_NODES];

    // Initialize distances, predecessors, and visited flags
    for (node_t n = get_first_node(); n <= get_last_node(); n = get_next_node(n)) {
        dist[n] = state->link_states[current_node].link_cost[n];
        visited[n] = 0;
        pred[n] = current_node;
    }
    dist[current_node] = 0;

    // Main loop to process all nodes
    for (node_t n = get_first_node(); n <= get_last_node(); n = get_next_node(n)) {
        node_t u = -1;
        cost_t min_cost = COST_INFINITY;

        // Find the closest unvisited node
        for (node_t candidate = get_first_node(); candidate <= get_last_node(); candidate = get_next_node(candidate)) {
            if (!visited[candidate] && dist[candidate] < min_cost) {
                u = candidate;
                min_cost = dist[candidate];
            }
        }

        if (u == -1) break; // No reachable unvisited nodes remain
        visited[u] = 1;

        // Update distances for neighbors of the selected node
        for (node_t neighbor = get_first_node(); neighbor <= get_last_node(); neighbor = get_next_node(neighbor)) {
            if (!visited[neighbor] && state->link_states[u].link_cost[neighbor] < COST_INFINITY) {
                cost_t alt = COST_ADD(dist[u], state->link_states[u].link_cost[neighbor]);
                if (alt < dist[neighbor]) {
                    dist[neighbor] = alt;
                    pred[neighbor] = u;
                }
            }
        }
    }

    for (node_t n = get_first_node(); n <= get_last_node(); n = get_next_node(n)) {
        if (n == current_node) continue;

        if (dist[n] == COST_INFINITY) {
            // Remove route for unreachable node
            set_route(n, -1, COST_INFINITY);
            printf("Setting route from %d to %d as unreachable\n", current_node, n);
        } else {
            node_t next_hop = n;
            node_t current = n;

            while (pred[current] != current_node && pred[current] != -1) {
                next_hop = pred[current];
                current = pred[current];
            }

            if (pred[current] != -1 && get_link_cost(next_hop) < COST_INFINITY) {
                set_route(n, next_hop, dist[n]);
                printf("Setting route from %d to %d via %d with cost %d\n", current_node, n, next_hop, dist[n]);
            } else {
                set_route(n, -1, COST_INFINITY);
                printf("Setting route from %d to %d as unreachable\n", current_node, n);
            }
        }
    }

}

// Notify a node that a neighboring link has changed cost.
void notify_link_change(node_t neighbor, cost_t new_cost) {
    printf("LC: Node %d: Link to neighbor %d changed to cost %d\n", get_current_node(), neighbor, new_cost);
    state_t *state = get_state();
    node_t current_node = get_current_node();

    state->link_states[current_node].link_cost[neighbor] = new_cost;
    state->link_states[current_node].version++;
    printf("LC: Node %d: Updated link state version to %d\n", current_node, state->link_states[current_node].version);

    run_dijkstra(state);
    broadcast_message(state);
}

// Receive a message sent by a neighboring node.
void notify_receive_message(node_t sender, void *message, size_t length) {
    printf("RM: Node %d: Received message from node %d\n", get_current_node(), sender);
    state_t *state = get_state();
    data_t *received_data = (data_t *)message;
    int updated = 0;

    for (node_t n = get_first_node(); n <= get_last_node(); n = get_next_node(n)) {
        //print versions
        printf("\n");
        printf("Node %d version(RECEIVED): %d\n", n, received_data->ls[n].version);
        printf("Node %d version(STATE, should be smaller): %d\n", n, state->link_states[n].version);
        if (received_data->ls[n].version > state->link_states[n].version) {
            printf("More recent version received from node %d\n", sender);
            state->link_states[n].version = received_data->ls[n].version;
            state->link_states[n] = received_data->ls[n];
            
            for (node_t dest = get_first_node(); dest <= get_last_node(); dest = get_next_node(dest)) {
                state->link_states[n].link_cost[dest] = received_data->ls[n].link_cost[dest];
            }
            updated = 1;
        }
    }

    if (updated) {
        // Run Dijkstra's algorithm to update routes
        run_dijkstra(state);
        printf("Running Dijkstra's algorithm\n");
        broadcast_message(state);
    }   
}