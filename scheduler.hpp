#ifndef __SCHEDULER_HPP__
#define __SCHEDULER_HPP__

#include <iostream>
#include <ostream>
#include <vector>
#include <iterator>
#include <functional>
#include <algorithm>
#include <mutex>
#include <thread>
#include <future>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/graphviz.hpp>

using namespace boost;

using Graph = adjacency_list<vecS, vecS, bidirectionalS>;
using Edge = graph_traits<Graph>::edge_descriptor;
using Vertex = graph_traits<Graph>::vertex_descriptor;
using vIt = graph_traits<Graph>::vertex_iterator;
using Vertexcont = std::list<Vertex>;
using vContIt = Vertexcont::const_iterator;
using outIt = graph_traits<Graph>::out_edge_iterator;
using inIt = graph_traits<Graph>::in_edge_iterator;

enum class node_types { sink_ = 0, source_ = 1 };


//! Simple std::thread-based scheduler. Tasks are specified
//! as an std::vector of std::function, or callable types,
//! while the dependencies are encoded by a Graph object 
//! from the Boost Graph Library.

//! F       : template type, a callable object, possible a std::function<> type
//!           The arguments are assumed to be bound (by std::bind or a lambda)
//!           by the caller, so that F has arguments of type void.

//! CTOR Inputs:
//! tasks : a vector of callable tasks, arbitrary return type, void input
//! deps : A Graph object, consisting of vertices {0,...,N}
//! connected by one or more directed edges. 
//! The dependency graph, specifically the directed edges, may be set
//! on the boost graph object, or with the scheduler<F> member function 
//! add_deps(int, int)
//!
//! The dependency graph must be a directed acyclic graph otherwise the 
//! constructor will exit.
//!
//! If the task vector and dependency graph are all well-specified, the
//! algorithm will find the initial node, initiate that task, after which
//! all other tasks follow.

template<typename F>
class scheduler
{
public:
  using ret_type = typename std::result_of<F()>::type;
  using fun_type = F;
  using prom_type = std::promise<ret_type>;
  using fut_type = std::shared_future<ret_type>;

  ~scheduler();
  scheduler() = default;
  scheduler(const Graph &deps_, const std::vector<fun_type> &tasks_) :
    g(deps_),
    tasks(tasks_) { init_();}
  scheduler(Graph&& deps_, std::vector<fun_type>&& tasks_) :
    g(std::move(deps_)),
    tasks(std::move(tasks_)) { init_(); }
  scheduler(std::vector<fun_type> &tasks_) :
    g(Graph(tasks_.size())),
    tasks(tasks_) { }
  scheduler(std::vector<fun_type> &&tasks_) :
    g(Graph(tasks_.size())),
    tasks(tasks_) { }
  
  // Future components only moveable
  scheduler(const scheduler&) = delete;
  scheduler& operator=(const scheduler&) = delete;
  
  void add_dep(int,int);
        
private:
  void init_();
  void set_futures();
  void set_sources();
  void set_initial_node(bool checkDAG=true);
  void set_run_levels();
  void set_tasks() noexcept;
  void join_tasks() noexcept;
  bool is_DAG(const Graph&);
  std::vector<int> ELS_delta(const Graph&);
  size_t graph_size(const Graph& g);

  std::list<Vertex> get_sources(const Vertex&);
  auto task_thread(fun_type&&, int);

  Graph g;
  std::vector<fun_type> tasks;
  std::vector<prom_type> prom;
  std::vector<fut_type> fut;
  std::vector<std::thread> th;
  std::vector<std::list<Vertex>> sources;
  std::vector<int> run_levels;
  std::vector<int> delta;
  Vertex initial_node;
    
};

class scheduler_utils
{
public:
  static std::iterator_traits<vIt>::difference_type graph_size(const Graph& g)
  {
    // Graph object can be overallocated so we count vertices
    return std::distance(vertices(g).first, vertices(g).second);
  }
  
  template<typename It, typename F>
  static void for_each_index(It first, It last,
                 typename std::iterator_traits<It>::difference_type i_val,
                 F&& f)
  {
    for(;first!=last;++first,++i_val)
      f(i_val,*first);
  }
};

#include "scheduler_impl.hpp"

#endif
