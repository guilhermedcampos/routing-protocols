# Introduction

This project is a network routing simulation for the Computer Networks course that implements four different routing algorithms to determine the shortest paths between nodes in a network. The goal is to understand how different routing approaches handle dynamic network conditions and message exchanges between routers.

# Getting Started

## Prerequisites

Before running the project, ensure you have the following installed:

- **C Compiler (GCC/Clang)** – Required to compile the C files.
- **Make** – To build the project using the provided `Makefile`.
- **GraphViz** – To visualize the network topology.

## Repo Structure

The project includes four distinct routing protocol implementations, each protocol is implemented in a separate C file:

- **Distance-Vector (DV)** – (`dv.c`) Uses the Bellman-Ford algorithm to compute shortest paths by exchanging distance vectors with neighbors.
- **Distance-Vector with Reverse Path Poisoning (DVRPP)** – (`dvrpp.c`) Enhances the DV protocol by preventing certain routing loops using reverse path poisoning.
- **Path-Vector (PV)** – (`pv.c`) Extends distance-vector routing by maintaining full path information to prevent loops.
- **Link-State (LS)** – (`ls.c`) Uses Dijkstra’s algorithm to compute shortest paths after building a global view of the network topology.

Other files:
- **dot-to-pdf.sh** – Script to convert `.dot` files to PDFs.
- **routing-simulator.cpp** – The core simulator file.
- **topologies/** – Directory containing:
  - **`.net` files**: Network topology input files.
  - **Generated PDFs**: Visualization outputs for each algorithm.

# Run and Build

## Building the Project

To compile the project, run:

```sh
make
```

## Running the Simulation


### Generating a Step-by-Step Visualization

For instance, to run a specific routing algorithm with a given topology file, use:

```sh
./{routing-algorithm}-simulator topologies/{topology-type}.net --final-dot output.dot
```

This will generate a DOT file (output.dot) with the final network state.

If you want to generate a DOT file that shows each step of the simulation:

```sh
./{routing-algorithm}-simulator topologies/{topology-type}.net --steps-dot steps.dot
```

### Converting DOT Files to PDF

Once you have the .dot file, convert it to a PDF using:

```sh
./dot-to-pdf.sh output.dot
```

or, for the step-by-step visualization:

```sh
./dot-to-pdf.sh steps.dot
```

This will create a PDF file that visually represents the network topology and routing updates.

### Example Usage

Running the Path-Vector (PV) routing algorithm on a linear topology with 3 nodes:

```sh
./pv-simulator topologies/linear-3.net --final-dot output.dot
./dot-to-pdf.sh output.dot
```

## Topology File Format

Network topology files (`.net`) define the network structure using the format:

<time> <first-node> <second-node> <cost>

### Example: `linear-3.net`

0 0 1 1 
0 1 2 1

This is a linear topology with three nodes (0-1-2), where each link has a cost of 1, forming a simple chain.
Setting link cost to 255 disables a link.




