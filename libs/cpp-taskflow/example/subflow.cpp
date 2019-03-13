// This example demonstrates how to use cpp-taskflow to create
// dynamic workload during execution.
//
// We first create four tasks A, B, C, and D. During the execution
// of B, it uses flow builder to creates another three tasks
// B1, B2, and B3, and adds dependencies from B1 and B2 to B3.
//
// We use dispatch and get to wait until the graph finished.
// Do so is difference from "wait_for_all" which will clean up the
// finished graphs. After the graph finished, we dump the topology
// for inspection.
//
// Usage: ./subflow detach|join
//

#include <taskflow/taskflow.hpp>  

const auto usage = "usage: ./subflow detach|join";

int main(int argc, char* argv[]) {

  if(argc != 2) {
    std::cerr << usage << std::endl;
    std::exit(EXIT_FAILURE);
  }

  std::string_view opt(argv[1]);

  if(opt != "detach" && opt != "join") {
    std::cerr << usage << std::endl;
    std::exit(EXIT_FAILURE);
  }

  auto detached = (opt == "detach") ? true : false;

  // Create a taskflow graph with three regular tasks and one subflow task.
  tf::Taskflow tf(std::thread::hardware_concurrency());

  auto [A, B, C, D] = tf.emplace(
    // Task A
    [] () { std::cout << "TaskA\n"; },              
    // Task B
    [cap=std::vector<int>{1,2,3,4,5,6,7,8}, detached] (auto& subflow) {
      std::cout << "TaskB is spawning B1, B2, and B3 ...\n";

      auto B1 = subflow.emplace([&]() { 
        printf("  Subtask B1: reduce sum = %d\n", 
                std::accumulate(cap.begin(), cap.end(), 0, std::plus<int>()));
      }).name("B1");        
      
      auto B2 = subflow.emplace([&]() { 
        printf("  Subtask B2: reduce multiply = %d\n", 
                std::accumulate(cap.begin(), cap.end(), 1, std::multiplies<int>()));
      }).name("B2");        
                                                              
      auto B3 = subflow.emplace([&]() { 
        printf("  Subtask B3: reduce minus = %d\n", 
                std::accumulate(cap.begin(), cap.end(), 0, std::minus<int>()));
      }).name("B3");        
                                                              
      B1.precede(B3);
      B2.precede(B3);

      // detach or join the subflow (by default the subflow join at B)
      if(detached) subflow.detach();
    },
    // Task C
    [] () { std::cout << "TaskC\n"; },               
    // Task D
    [] () { std::cout << "TaskD\n"; }                
  );                                                 
                                         
  A.name("A");
  B.name("B");
  C.name("C");
  D.name("D");
              
  A.precede(B);  // B runs after A 
  A.precede(C);  // C runs after A 
  B.precede(D);  // D runs after B 
  C.precede(D);  // D runs after C  

  tf.dispatch().get();  // block until finished

  // examine the graph
  std::cout << '\n' << tf.dump_topologies();

  return 0;
}



