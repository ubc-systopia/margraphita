#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <stack>
#include <vector>

#include "adj_list.h"
#include "command_line.h"
#include "common_util.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "times.h"

/**
 * Betweeness Centrality - Vertex Centric - for Undirected Graph only
 * http://snap.stanford.edu/class/cs224w-readings/brandes01centrality.pdf
 */

std::vector<double> betweeness_centrality(GraphBase* graph)
{
  std::vector<double> bc = std::vector<double>(graph->get_num_nodes());
  for (int64_t i = 0; i < graph->get_num_nodes(); i++)
  {
    bc[i] = 0;
  }

  for (node_id_t s = 0; s < graph->get_num_nodes(); s++)
  {
    std::stack<node_id_t> S;
    std::queue<node_id_t> Q;
    std::vector<std::vector<node_id_t>> P =
        std::vector<node_id_t>(graph->get_num_nodes());

    for (int64_t w = 0; w < graph->get_num_nodes(); w++)
    {
      std::vector<node_id_t> empty_list;
      bc[w] = empty_list;
    }

    std::vector<double> sigma = std::vector<double>(graph.get_num_nodes());

    std::vector<double> delta = std::vector<double>(graph.get_num_nodes());

    std::vector<double> delta_r = std::vector<double>(graph.get_num_nodes());

    for (int64_t t = 0; t < graph.get_num_nodes(); t++)
    {
      sigma[t] = 0;
      delta[t] = -1;
      delta_r[t] = 0;

      if (t = s)
      {
        sigma[t] = 1;
        delta[t] = 0;
      }
    }

    Q.push(s);

    while (!Q.empty())
    {
      node_id_t v = Q.pop();
      S.push(v);

      std::vector<node_id_t> v_neighborhood = graph.get_out_nodes_id(v);

      for (node_id_t w : v_neighborhood)
      {
        if (delta[w] < 0)
        {
          Q.push(w);
          delta[w] = delta[v] + 1;
        }

        if (delta[w] = delta[v] + 1)
        {
          sigma[w] = sigma[w] + sigma[v];
          P[w].push_back{v};
        }
      }
    }

    while (!S.empty())
    {
      node_id_t w = S.pop();
      for (node_id_t v : P[w])
      {
        delta_r[v] = delta_r[v] + ((sigma[v] / sigma[w]) * (1 + delta_r[w]));
        if (w != s)
        {
          bc[w] = bc[w] + delta_r[w];
        }
      }
    }
  }

  return bc;
}