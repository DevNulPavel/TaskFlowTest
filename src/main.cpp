#include <thread>
#include <utility>
#include <taskflow/taskflow.hpp>  // Cpp-Taskflow is header-only

// https://github.com/cpp-taskflow/cpp-taskflow

void simpleExample(){
    //  task dependency graph
    //
    //          +---+
    //    +---->| B |-----+
    //    |     +---+     |
    //  +---+           +-v-+
    //  | A |           | D |
    //  +---+           +-^-+
    //    |     +---+     |
    //    +---->| C |-----+
    //          +---+
    
    tf::Taskflow tf;
    
    // Закидываем задачи
    auto [A, B, C, D] = tf.emplace([] () { std::cout << "TaskA\n"; },
                                   [] () { std::cout << "TaskB\n"; },
                                   [] () { std::cout << "TaskC\n"; },
                                   [] () { std::cout << "TaskD\n"; });
    
    // Описываем порядок исполнения
    A.precede(B);  // A runs before B
    A.precede(C);  // A runs before C
    B.precede(D);  // B runs before D
    C.precede(D);  // C runs before D
    
    // Просто дожидаемся окончания
    //std::cout << "Before wait\n";
    //tf.wait_for_all();  // block until finish
    //std::cout << "After wait\n";
    
    // Создаем future для получения результата
    std::cout << "Before start\n";
    std::shared_future<void> future = tf.dispatch();
    std::cout << "After get future\n";
    
    future.wait();
    std::cout << "After wait future\n";
}

void dynamicTaskingExample(){
    tf::Taskflow tf;
    
    // Создаем 3 обычные задачи
    tf::Task A = tf.emplace([](){ std::cout << "TaskA\n"; }).name("A");
    tf::Task C = tf.emplace([](){ std::cout << "TaskС\n"; }).name("C");
    tf::Task D = tf.emplace([](){ std::cout << "TaskD\n"; }).name("D");
    
    // Создаем граф подзадач (dynamic tasking)
    tf::Task B = tf.emplace([](tf::SubflowBuilder& subflow) {
        // Создаем подзадачи
        tf::Task B1 = subflow.emplace([](){ std::cout << "TaskB1\n"; }).name("B1");
        tf::Task B2 = subflow.emplace([](){ std::cout << "TaskB2\n"; }).name("B2");
        tf::Task B3 = subflow.emplace([](){ std::cout << "TaskB3\n"; }).name("B3");
        
        // Описываем порядок исполнения
        B1.precede(B3); // B1 перед B3
        B2.precede(B3); // B2 перед B3
    }).name("B");
    
    // Описываем порядок исполнения
    A.precede(B);  // B runs after A
    A.precede(C);  // C runs after A
    B.precede(D);  // D runs after B
    C.precede(D);  // D runs after C
    
    // Создаем future для получения результата без очистки топологии
    std::shared_future<void> future = tf.dispatch();
    future.get();
    
    std::cout << "\nCurrent topology:\n";
    tf.dump_topologies(std::cout);
}

void placeholderExample(){
    tf::Taskflow tf;
    
    // Создаем заглушку для последующей установке кода
    tf::Task A = tf.emplace([](){ std::cout << "TaskA\n"; });
    tf::Task B = tf.placeholder();
    
    // Порядок обработки
    A.precede(B);
    
    // Выставляем обработчик после
    B.work([](){ std::cout << "TaskB\n"; });
    
    // Создаем future для получения результата без очистки топологии
    std::shared_future<void> future = tf.dispatch();
    future.get();
}

void parallelForExample1(){
    std::mutex mutex;
    tf::Taskflow tf;
    
    // Создаем данные и функцию для обработки данных
    std::vector<std::string> v = {"A", "B", "C", "D"};
    auto function = [&] (const std::string& val) {
        std::unique_lock<std::mutex> lock(mutex); // Синхронизован только один вызов <<, поэтому общая блокировка
        std::cout << "Value: " << val << "\n";
    };
    const uint32_t groupsCount = 2;   // Количество задач, которые будут исполняться последовательно, если нужно все параллельно - просто не указывать
    auto [S, T] = tf.parallel_for(v.begin(), v.end(), function, groupsCount);
    
    // Создаем future для получения результата без очистки топологии
    std::shared_future<void> future = tf.dispatch();
    future.get();
}

void parallelForExample2(){
    std::mutex mutex;
    tf::Taskflow tf;
    
    // Создаем данные и функцию для обработки данных
    std::vector<std::string> v = {"A", "B", "C", "D"};
    auto function = [&] (size_t index) {
        std::unique_lock<std::mutex> lock(mutex); // Синхронизован только один вызов <<, поэтому общая блокировка
        std::cout << "Value(" << index << "): " << v[index] << "\n";
    };
    const uint32_t begin = 0;
    const uint32_t end = 4;
    const uint32_t step = 1;
    const uint32_t groupsCount = 2;   // Количество задач, которые будут исполняться последовательно, если нужно все параллельно - просто не указывать
    auto [S, T] = tf.parallel_for(begin, end, step, function, groupsCount);
    
    // Создаем future для получения результата без очистки топологии
    std::shared_future<void> future = tf.dispatch();
    future.get();
}

void reduceExample(){
    tf::Taskflow tf;
    
    std::vector<int32_t> v = {1, 2, 3, 4};
    int32_t sum = 0;
    auto [S, T] = tf.reduce(v.begin(), v.end(), sum, std::plus<int32_t>());

    // Создаем future для получения результата без очистки топологии
    std::shared_future<void> future = tf.dispatch();
    future.get();
    
    std::cout << "Result = " << sum << "\n";
}

void transformReduceExample(){
    tf::Taskflow tf;
    
    std::vector<std::pair<int, int>> v = { {1, 5}, {6, 4}, {-6, 4} };
    int min = std::numeric_limits<int>::max();
    auto [S, T] = tf.transform_reduce(v.begin(), v.end(), min,
                                      [] (int l, int r) { return std::min(l, r); },
                                      [] (const std::pair<int, int>& p) { return std::min(p.first, p.second); } );
    
    // Создаем future для получения результата без очистки топологии
    std::shared_future<void> future = tf.dispatch();
    future.get();
}

void precedeExample(){
    tf::Taskflow tf;
    
    // Создаем обычные задачи
    tf::Task A = tf.emplace([](){ std::cout << "TaskA\n"; }).name("A");
    tf::Task B = tf.emplace([](){ std::cout << "TaskB\n"; }).name("B");
    tf::Task C = tf.emplace([](){ std::cout << "TaskС\n"; }).name("C");
    tf::Task D = tf.emplace([](){ std::cout << "TaskD\n"; }).name("D");
    tf::Task E = tf.emplace([](){ std::cout << "TaskE\n"; }).name("E");
    
    // А запускается перед B, C, D, E
    // B, C, D, E - работают параллельно
    A.precede(B, C, D, E);
    
    // Создаем future для получения результата без очистки топологии
    std::shared_future<void> future = tf.dispatch();
    future.get();
}

void gatherExample(){
    tf::Taskflow tf;
    
    // Создаем обычные задачи
    tf::Task A = tf.emplace([](){ std::cout << "TaskA\n"; }).name("A");
    tf::Task B = tf.emplace([](){ std::cout << "TaskB\n"; }).name("B");
    tf::Task C = tf.emplace([](){ std::cout << "TaskС\n"; }).name("C");
    tf::Task D = tf.emplace([](){ std::cout << "TaskD\n"; }).name("D");
    tf::Task E = tf.emplace([](){ std::cout << "TaskE\n"; }).name("E");
    
    // A запускается после B, C, D, E
    // B, C, D, E - работают параллельно
    A.gather(B, C, D, E);
    
    // Создаем future для получения результата без очистки топологии
    std::shared_future<void> future = tf.dispatch();
    future.get();
    
    tf.wait_for_all();
}

int main(int argc, char *argv[]) {
    //simpleExample();
    //dynamicTaskingExample();
    //placeholderExample();
    //parallelForExample1();
    //parallelForExample2();
    //reduceExample();
    //transformReduceExample();
    //precedeExample();
    gatherExample();
}
