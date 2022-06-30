#include "xw.h"

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/one_bit_color_map.hpp>
#include <boost/graph/stoer_wagner_min_cut.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/typeof/typeof.hpp>

struct edge_t
{
  unsigned long first;
  unsigned long second;
  edge_t(unsigned f, unsigned s) : first(f), second(s) { }
};

using namespace boost;

typedef adjacency_list<vecS, vecS, undirectedS, no_property,
                       property<edge_weight_t, int>> undirected_graph;
typedef property_map<undirected_graph, edge_weight_t>::type weight_map_type;
typedef property_traits<weight_map_type>::value_type weight_type;
  
unsigned mincut(const vector<pair<unsigned,unsigned>> &graph,
                vector<unsigned> &seg1, vector<unsigned> &seg2,
                const vector<unsigned> &weights = vector<unsigned>())
{
  vector<edge_t> edges;
  unsigned biggest = 0;
  for (auto &p : graph) {
    edges.push_back(edge_t(p.first,p.second));
    biggest = max(biggest,max(p.first,p.second));
  }
  const vector<unsigned> *w = &weights;
  vector<unsigned> w2;
  if (weights.empty()) {
    w2.resize(graph.size(),1);
    w = &w2;
  }
  // construct the graph object. 8 is the number of vertices, which
  // are numbered from 0 through 7, and 16 is the number of edges.
  undirected_graph g(edges.data(),edges.data() + edges.size(),w->data(),
                     biggest + 1,edges.size());
                     
  // define a property map, `parities`, that will store a boolean
  // value for each vertex.  Vertices that have the same parity after
  // `stoer_wagner_min_cut` runs are on the same side of the min-cut.
  BOOST_AUTO(parities, make_one_bit_color_map(num_vertices(g),
                                              get(vertex_index,g)));

  // run the Stoer-Wagner algorithm to obtain the min-cut
  // weight. `parities` is also filled in.
  int ans = stoer_wagner_min_cut(g,get(edge_weight, g),parity_map(parities));

  seg1.clear();
  seg2.clear();

  for (size_t i = 0 ; i < num_vertices(g) ; ++i)
    if (get(parities,i)) seg1.push_back(i);
    else seg2.push_back(i);

  return ans;
}

template<class T>
unsigned position(const vector<T> &vec, const T &item)
{
  auto itr = lower_bound(vec.begin(),vec.end(),item);
  assert(*itr == item);
  return itr - vec.begin();
}

void puzzle::mincut() const
{
  vector<unsigned> lengths;
  vector<const word *> word_from_id;
  for (auto &w : *this) {
    if (w.id >= lengths.size()) {
      lengths.resize(w.id + 1);
      word_from_id.resize(w.id + 1);
    }
    lengths[w.id] = w.size();
    word_from_id[w.id] = &w;
  }
  vector<pair<unsigned,unsigned>> edges;
  vector<unsigned> weights;
  for (auto &w : *this)
    for (auto sqp : w)
      for (auto other_id : sqp->word_ids)
        if (w.id < other_id) {
          edges.push_back({ w.id, other_id });
          weights.push_back(100000 / (lengths[w.id] * lengths[other_id]));    
        }
  
  vector<unsigned> s1, s2;
  unsigned sc = ::mincut(edges,s1,s2);//,weights);
  vector<string> seg1, seg2;
  for (unsigned i : s1) seg1.push_back(word_from_id[i]->desc());
  for (unsigned i : s2) seg2.push_back(word_from_id[i]->desc());
  cout << "score " << sc << endl
       << "seg1 " << seg1 << endl
       << "seg2 " << seg2 << endl;
  exit(0);
}
