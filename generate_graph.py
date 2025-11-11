import random
import networkx as nx
import matplotlib.pyplot as plt

def generate_graph(filename, N, M, image_name="graph.png"):
    # Create an undirected graph
    G = nx.Graph()

    # Vertex IDs from 0 to N-1
    ids = list(range(N))

    # Add nodes with attributes
    for city_id in ids:
        name = f"Cidade{city_id}"
        city_type = random.choice([0, 1])  # 0 = regional, 1 = capital
        G.add_node(city_id, name=name, type=city_type)

    # Generate edges (no duplicates, no self-loops)
    edges = set()
    while len(edges) < M:
        u, v = random.sample(ids, 2)
        edge = tuple(sorted((u, v)))
        edges.add(edge)

    # Add edges with random weights
    for u, v in edges:
        weight = random.randint(1, 10)
        G.add_edge(u, v, weight=weight)

    # Save to text file
    with open(filename, "w", encoding="utf-8") as f:
        f.write(f"{N} {M}\n")
        for city_id in ids:
            data = G.nodes[city_id]
            f.write(f"{city_id} {data['name']} {data['type']}\n")
        for u, v, data in G.edges(data=True):
            f.write(f"{u} {v} {data['weight']}\n")

    print(f"✅ Graph saved to {filename} ({N} vertices, {M} edges)")

    # Draw the graph
    plt.figure(figsize=(10, 8))
    pos = nx.spring_layout(G, seed=42)  # reproducible layout

    # Node colors: red for capitals, blue for regional
    colors = ["red" if G.nodes[n]["type"] == 1 else "skyblue" for n in G.nodes]
    nx.draw_networkx_nodes(G, pos, node_color=colors, node_size=300)
    nx.draw_networkx_edges(G, pos, width=1, alpha=0.7)
    nx.draw_networkx_labels(G, pos, {n: G.nodes[n]["name"] for n in G.nodes}, font_size=8)

    plt.title(f"Random Graph ({N} vertices, {M} edges)")
    plt.axis("off")
    plt.tight_layout()
    plt.savefig(image_name, dpi=300)
    plt.close()

    print(f"🖼️ Graph image saved to {image_name}")

if __name__ == "__main__":
    # Example usage
    generate_graph("grafo_random.txt", N=5, M=9, image_name="grafo_visual.png")
