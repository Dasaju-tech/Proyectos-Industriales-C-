#ifndef DIRECTED_GRAPH_H
#define DIRECTED_GRAPH_H

// Fixed-capacity Kahn topological sort. C++98 / Visual C++ 2008.
// Edges point from a dependency to its consumer. No heap allocation.
namespace DirectedGraph
{
    enum { MAX_NODES = 64, MAX_EDGES = 128 };
    struct Edge
    {
        int from;
        int to;
    };

    // Returns false for malformed graphs or cycles. On failure, order is unchanged.
    inline bool TopologicalOrder(int node_count, const Edge* edges,
                                 int edge_count, int* order)
    {
        if (node_count < 0 || node_count > MAX_NODES ||
            edge_count < 0 || edge_count > MAX_EDGES ||
            (edge_count != 0 && edges == 0) ||
            (node_count != 0 && order == 0))
            return false;

        int indegree[MAX_NODES] = { 0 };
        int first[MAX_NODES];
        int next[MAX_EDGES];
        int queue[MAX_NODES];
        int result[MAX_NODES];
        int i;
        for (i = 0; i < node_count; ++i)
            first[i] = -1;
        for (i = 0; i < edge_count; ++i)
        {
            if (edges[i].from < 0 || edges[i].from >= node_count ||
                edges[i].to < 0 || edges[i].to >= node_count)
                return false;
            ++indegree[edges[i].to];
            next[i] = first[edges[i].from];
            first[edges[i].from] = i;
        }
        int head = 0;
        int tail = 0;
        int count = 0;
        for (i = 0; i < node_count; ++i)
            if (indegree[i] == 0)
                queue[tail++] = i;
        while (head < tail)
        {
            const int current = queue[head++];
            result[count++] = current;
            for (i = first[current]; i != -1; i = next[i])
            {
                if (--indegree[edges[i].to] == 0)
                    queue[tail++] = edges[i].to;
            }
        }
        if (count != node_count)
            return false;
        for (i = 0; i < node_count; ++i)
            order[i] = result[i];
        return true;
    }
}

#endif
