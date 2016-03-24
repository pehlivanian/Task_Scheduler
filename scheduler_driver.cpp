#include "scheduler_driver.hpp"

int basic_task(int task_id, int d)
{
  std::chrono::milliseconds sleepDuration(d);
  std::this_thread::sleep_for(sleepDuration);
  std::cerr << "Task ID: " << task_id << "\n";
  return task_id;
}

enum task_nodes
  {
    task_0,
    task_1,
    task_2,
    task_3,
    task_4,
    task_5,
    task_6,
    task_7,
    N
  };

int main(int argc, char **argv)
{

  using F = std::function<int()>;
  
   Graph deps(N);

   boost::add_edge(task_0, task_1, deps);
   boost::add_edge(task_0, task_2, deps);
   boost::add_edge(task_1, task_3, deps);
   boost::add_edge(task_2, task_3, deps);
   boost::add_edge(task_3, task_4, deps);
   boost::add_edge(task_4, task_5, deps);
   boost::add_edge(task_5, task_6, deps);
   boost::add_edge(task_5, task_7, deps);

   std::vector<F> tasks = 
     {
	std::bind(basic_task, 0, 100),
	std::bind(basic_task, 1, 100),
	std::bind(basic_task, 2, 100),
	std::bind(basic_task, 3, 100),
	std::bind(basic_task, 4, 100),
	std::bind(basic_task, 5, 100),
	std::bind(basic_task, 6, 100),
	std::bind(basic_task, 7, 100)
     };

     {
       auto s = std::make_unique<scheduler<std::function<int()>>>(std::move(deps), std::move(tasks));
     }
   
  return 0;
}
