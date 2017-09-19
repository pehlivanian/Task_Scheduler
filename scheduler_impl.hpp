
#ifndef __SCHEDULER_IMPL_HPP__
#define __SCHEDULER_IMPL_HPP__

#include <numeric>
#include <iostream>
#include <ostream>
#include <iterator>

template<typename F>
void
scheduler<F>::set_futures()
{
  int num_tasks = tasks.size();

  prom.resize(num_tasks);
  fut.resize(num_tasks);

  // Get the futures
  for(size_t i=0;
      i<num_tasks;
      ++i)
    {
      try 
        {
          fut[i] = prom[i].get_future();
        }
      catch(std::future_error &e)
        {; /* future retreived */ }
    }
}

template<typename F>
void
scheduler<F>::set_sources()
{
  int num_tasks = tasks.size();

  sources.resize(num_tasks);

  size_t cnt=0;
  std::generate(sources.begin(),sources.end(),[&cnt,this]()
                {
                  return get_sources(cnt++);
                });

}

template<typename F>
void
scheduler<F>::set_initial_node(bool checkDAG)
{
  auto numInits = std::count(run_levels.begin(), run_levels.end(), 0);

  if ((checkDAG) && (numInits == 0))
    {
      std::cerr << "No initial node specified.\n";
      exit(1);
    }
  else if ((checkDAG) && (numInits > 1))
    {
      std::cerr << "More than one initial node.\n";
      exit(1);
    }
  else {
    auto initIt = std::find_if(run_levels.begin(), run_levels.end(), [](int i)
                               {
                                 return (i==0); // no parent nodes
                               });
    initial_node = (run_levels.end() != initIt) ? *initIt : NULL;
  }
}
    
template<typename F>
void
scheduler<F>::init_()
{
  // Set the futures
  set_futures();

  // Set run levels
  set_run_levels();
      
  // Predetermine in_edges for faster traversal
  set_sources();

  // Set initial_node
  set_initial_node(false);

  // Render final graph in graphviz.dot
  // write_graphviz(std::cout, g);
      
}
  
template<typename F>
std::vector<int> 
scheduler<F>::ELS_delta(const Graph& gr)
{
  auto graph_size = scheduler_utils::graph_size;

  std::vector<Vertex> v(graph_size(gr),1);
  std::vector<int> d(graph_size(gr));
  v[0] = 0;
  std::partial_sum(v.begin(), v.end(), v.begin());
  for_each(v.begin(), v.end(), [&d,gr](Vertex i)
           {
             auto oe = out_edges(i,gr);
             auto ie = in_edges(i,gr);
             d[i] = std::distance(oe.first,oe.second) - 
                    std::distance(ie.first,ie.second);
           });
  return d;

}

template<typename F>
bool
scheduler<F>::is_DAG(const Graph& g)
{
  Vertexcont toporder;
  try // Still the fastest way I've found; O(mn)
    {
      topological_sort(g, std::front_inserter(toporder));
      return true;
    }
  catch(std::exception &e)
    {
      return false;
    }
}

template<typename F>
void 
scheduler<F>::set_run_levels()
{
      
  run_levels = std::vector<int>(tasks.size(), 0);
  Vertexcont toporder;
  
  try
    {
      topological_sort(g, std::front_inserter(toporder));
    }
  catch(std::exception &e)
    {
      std::cerr << e.what() << "\n";
      std::cerr << "You have defined a pipeline - not supported...\n";
      exit(1);
    }

  vContIt i = toporder.begin();
      
  for(;
      i != toporder.end();
      ++i)
    {
      if (in_degree(*i,g) > 0)
        {
          inIt j, j_end;
          int maxdist = 0;
          for(boost::tie(j,j_end) = in_edges(*i,g);
              j != j_end;
              ++j)
            {
              maxdist = (std::max)(run_levels[source(*j,g)], maxdist);
              run_levels[*i] = maxdist+1;
            }
        }
    }

    run_order = std::vector<int>(run_levels.size());
    std::iota(run_order.begin(), run_order.end(), 0);

    auto run_order_sorter = [this](int l, int r){ return this->run_levels[l] < this->run_levels[r];};
    std::sort(run_order.begin(), run_order.end(), run_order_sorter);

}

template<typename F>
void
scheduler<F>::add_dep(int i, int j)
{
  boost::add_edge(i,j,g);

  set_futures();
  set_run_levels();
  set_sources();
  set_initial_node(false);
}

template<typename F>
std::list<Vertex>
scheduler<F>::get_sources(const Vertex& v)
{
  std::list<Vertex> r;
  Vertex v1;
  inIt j, j_end;
  boost::tie(j,j_end) = in_edges(v, g);
  for(;j != j_end;++j)
    {
      v1 = source(*j, g);
      r.push_back(v1);
    }

  return r;
}
 
template<typename F>
auto
scheduler<F>::task_thread(fun_type&& f, int i)
{
  auto j_beg = sources[i].begin(), j_end = sources[i].end();

  for(;
      j_beg != j_end;
      ++j_beg)
    {
      ret_type val = fut[*j_beg].get();
    }

  return std::thread([this](fun_type f, int i)
                     {
                       prom[i].set_value(f());
                     },f,i);
}

template<typename F>
void
scheduler<F>::set_tasks() noexcept
{
  size_t num_tasks = tasks.size();
  th.resize(num_tasks);


  for(int i : run_order)
    {
      th[i] = task_thread(std::move(tasks[i]), i);
     }
}

template<typename F>
void
scheduler<F>::join_tasks() noexcept
{
    // for_each(run_order.begin(), run_order.end(), [this](auto i){if (this->th[i].joinable()) { this->th[i].join(); }});
    for_each(th.begin(), th.end(), [](auto&& t){ if (t.joinable()) { t.join(); }});
}

template<typename F>
scheduler<F>::~scheduler<F>()
{
  set_tasks();
  join_tasks();
}

#endif
