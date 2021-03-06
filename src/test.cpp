#include <libunittest/all.hpp>
#include "transwarp.h"
#include "../examples/basic_with_three_tasks.h"
#include "../examples/statistical_key_facts.h"


using transwarp::make_task;
using transwarp::make_final_task;


COLLECTION(test_transwarp) {

void make_test_one_task(std::size_t threads) {
    const int value = 42;
    auto f1 = [value]{ return value; };
    std::shared_ptr<transwarp::ifinal_task<int>> task;
    if (threads > 0) {
        task = make_final_task(transwarp::parallel{threads}, f1);
    } else {
        task = make_final_task(transwarp::sequenced{}, f1);
    }
    ASSERT_EQUAL(0u, task->get_node().id);
    ASSERT_EQUAL(0u, task->get_node().level);
    ASSERT_EQUAL(0u, task->get_node().parents.size());
    ASSERT_EQUAL("task0", task->get_node().name);
    const auto graph = task->get_graph();
    ASSERT_EQUAL(0u, graph.size());
    task->schedule();
    auto future = task->get_future();
    ASSERT_EQUAL(42, future.get());
}

TEST(one_task) {
    make_test_one_task(0);
    make_test_one_task(1);
    make_test_one_task(2);
    make_test_one_task(3);
    make_test_one_task(4);
}

void make_test_three_tasks(std::size_t threads) {
    int value = 42;

    auto f1 = [&value]{ return value; };
    auto task1 = make_task("t1", f1);

    auto f2 = [](int v) { return v + 2; };
    auto task2 = make_task("\nt2\t", f2, task1);

    auto f3 = [](int v, int w) { return v + w + 3; }; 
    std::shared_ptr<transwarp::ifinal_task<int>> task3;
    if (threads > 0) {
        task3 = make_final_task("t3 ", transwarp::parallel{threads}, f3, task1, task2);
    } else {
        task3 = make_final_task("t3 ", transwarp::sequenced{}, f3, task1, task2);
    }

    ASSERT_EQUAL(0u, task1->get_node().id);
    ASSERT_EQUAL(0u, task1->get_node().level);
    ASSERT_EQUAL(0u, task1->get_node().parents.size());
    ASSERT_EQUAL("t1", task1->get_node().name);

    ASSERT_EQUAL(1u, task2->get_node().id);
    ASSERT_EQUAL(1u, task2->get_node().level);
    ASSERT_EQUAL(1u, task2->get_node().parents.size());
    ASSERT_EQUAL("\nt2\t", task2->get_node().name);

    ASSERT_EQUAL(2u, task3->get_node().id);
    ASSERT_EQUAL(2u, task3->get_node().level);
    ASSERT_EQUAL(2u, task3->get_node().parents.size());
    ASSERT_EQUAL("t3 ", task3->get_node().name);

    task3->schedule();
    ASSERT_EQUAL(89, task3->get_future().get());
    ASSERT_EQUAL(42, task1->get_future().get());

    ++value;

    task3->schedule();
    ASSERT_EQUAL(91, task3->get_future().get());
    ASSERT_EQUAL(43, task1->get_future().get());

    const auto graph = task3->get_graph();
    ASSERT_EQUAL(3u, graph.size());
    const auto dot_graph = transwarp::make_dot(graph);

    const std::string exp_dot_graph = "digraph {\n"
"\"t1\n"
"id 0 level 0 parents 0\" -> \"t2\n"
"id 1 level 1 parents 1\"\n"
"\"t1\n"
"id 0 level 0 parents 0\" -> \"t3\n"
"id 2 level 2 parents 2\"\n"
"\"t2\n"
"id 1 level 1 parents 1\" -> \"t3\n"
"id 2 level 2 parents 2\"\n"
"}\n";

    ASSERT_EQUAL(exp_dot_graph, dot_graph);
}

TEST(three_tasks) {
    make_test_three_tasks(0);
    make_test_three_tasks(1);
    make_test_three_tasks(2);
    make_test_three_tasks(3);
    make_test_three_tasks(4);
}

void make_test_bunch_of_tasks(std::size_t threads) {
    auto f0 = []{ return 42; };
    auto f1 = [](int a){ return 3 * a; };
    auto f2 = [](int a, int b){ return a + b; };
    auto f3 = [](int a, int b, int c){ return a + 2*b + c; };

    auto task0 = make_task(f0);
    auto task1 = make_task(f0);
    auto task2 = make_task(f1, task1);
    auto task3 = make_task(f2, task2, task0);
    auto task5 = make_task(f2, task3, task2);
    auto task6 = make_task(f3, task1, task2, task5);
    auto task7 = make_task(f2, task5, task6);
    auto task8 = make_task(f2, task6, task7);
    auto task9 = make_task(f1, task7);
    auto task10 = make_task(f1, task9);
    auto task11 = make_task(f3, task10, task7, task8);
    auto task12 = make_task(f2, task11, task6);
    std::shared_ptr<transwarp::ifinal_task<int>> task13;
    if (threads > 0) {
        task13 = make_final_task(transwarp::parallel{threads}, f3, task10, task11, task12);
    } else {
        task13 = make_final_task(transwarp::sequenced{}, f3, task10, task11, task12);
    }

    const auto task0_result = 42;
    const auto task3_result = 168;
    const auto task11_result = 11172;
    const auto exp_result = 42042;

    task13->schedule();
    ASSERT_EQUAL(exp_result, task13->get_future().get());
    ASSERT_EQUAL(task0_result, task0->get_future().get());
    ASSERT_EQUAL(task3_result, task3->get_future().get());
    ASSERT_EQUAL(task11_result, task11->get_future().get());

    for (auto i=0; i<100; ++i) {
        task13->schedule();
        ASSERT_EQUAL(task0_result, task0->get_future().get());
        ASSERT_EQUAL(task3_result, task3->get_future().get());
        ASSERT_EQUAL(task11_result, task11->get_future().get());
        ASSERT_EQUAL(exp_result, task13->get_future().get());
    }
}

TEST(bunch_of_tasks) {
    make_test_bunch_of_tasks(0);
    make_test_bunch_of_tasks(1);
    make_test_bunch_of_tasks(2);
    make_test_bunch_of_tasks(3);
    make_test_bunch_of_tasks(4);
}

TEST(transwarp_error) {
    const std::string msg = "text";
    try {
        throw transwarp::transwarp_error(msg);
    } catch (const std::runtime_error& e) {
        ASSERT_EQUAL(msg, e.what());
    }
}

TEST(task_canceled) {
    const std::string msg = "cool is canceled";
    const transwarp::node node{1, 2, "cool", {}};
    try {
        throw transwarp::task_canceled(node);
    } catch (const transwarp::transwarp_error& e) {
        ASSERT_EQUAL(msg, e.what());
    }
}

TEST(make_dot_graph_with_empty_graph) {
    const std::vector<transwarp::edge> graph;
    const auto dot_graph = transwarp::make_dot(graph);
    const std::string exp_dot_graph = "digraph {\n}\n";
    ASSERT_EQUAL(exp_dot_graph, dot_graph);
}

TEST(make_dot_graph_with_three_nodes) {
    const transwarp::node node2{1, 1, "node2", {}};
    const transwarp::node node3{2, 1, "node3", {}};
    const transwarp::node node1{0, 0, "node1", {&node2, &node3}};
    std::vector<transwarp::edge> graph;
    graph.push_back({&node1, &node2});
    graph.push_back({&node1, &node3});
    const auto dot_graph = transwarp::make_dot(graph);
    const std::string exp_dot_graph = "digraph {\n"
"\"node2\n"
"id 1 level 1 parents 0\" -> \"node1\n"
"id 0 level 0 parents 2\"\n"
"\"node3\n"
"id 2 level 1 parents 0\" -> \"node1\n"
"id 0 level 0 parents 2\"\n"
"}\n";

    ASSERT_EQUAL(exp_dot_graph, dot_graph);
}

TEST(get_functor) {
    auto f1 = [] { return 42; };
    auto task1 = make_task(f1);
    ASSERT_TRUE(f1 == task1->get_functor());
}

template<typename Task>
constexpr std::size_t n_parents() {
    using result_t = decltype(std::declval<Task>().get_parents());
    return std::tuple_size<typename std::decay<result_t>::type>::value;
}

TEST(get_parents) {
    auto f1 = [] { return 42; };
    auto task1 = make_task(f1);
    static_assert(n_parents<decltype(*task1)>() == 0, "");
    auto f2 = [] { return 13; };
    auto task2 = make_task(f2);
    static_assert(n_parents<decltype(*task2)>() == 0, "");
    auto f3 = [](int v, int w) { return v + w; };
    auto task3 = make_task(f3, task1, task2);
    static_assert(n_parents<decltype(*task3)>() == 2, "");
    auto tasks = task3->get_parents();
    ASSERT_EQUAL(task1.get(), std::get<0>(tasks).get());
    ASSERT_EQUAL(task2.get(), std::get<1>(tasks).get());
}

TEST(get_node) {
    auto f1 = [] { return 42; };
    auto task1 = make_task(f1);
    auto f2 = [] { return 13; };
    auto task2 = make_task(f2);
    auto f3 = [](int v, int w) { return v + w; };
    auto task3 = make_final_task(transwarp::sequenced{}, f3, task1, task2);

    // task3
    ASSERT_EQUAL(2, task3->get_node().id);
    ASSERT_EQUAL(1, task3->get_node().level);
    ASSERT_EQUAL("task2", task3->get_node().name);
    ASSERT_EQUAL(2u, task3->get_node().parents.size());
    ASSERT_EQUAL(&task1->get_node(), task3->get_node().parents[0]);
    ASSERT_EQUAL(&task2->get_node(), task3->get_node().parents[1]);

    // task1
    ASSERT_EQUAL(0, task1->get_node().id);
    ASSERT_EQUAL(0, task1->get_node().level);
    ASSERT_EQUAL("task0", task1->get_node().name);
    ASSERT_EQUAL(0u, task1->get_node().parents.size());

    // task2
    ASSERT_EQUAL(1, task2->get_node().id);
    ASSERT_EQUAL(0, task2->get_node().level);
    ASSERT_EQUAL("task1", task2->get_node().name);
    ASSERT_EQUAL(0u, task2->get_node().parents.size());
}

void make_test_task_with_exception_thrown(std::size_t threads) {
    auto f1 = [] {
        throw std::logic_error("from f1");
        return 42;
    };
    auto f2 = [] (int x) {
        throw std::logic_error("from f2");
        return x + 13;
    };
    auto f3 = [] (int x) {
        throw std::logic_error("from f3");
        return x + 1;
    };
    auto task1 = make_task(f1);
    auto task2 = make_task(f2, task1);
    std::shared_ptr<transwarp::ifinal_task<int>> task3;
    if (threads > 0) {
        task3 = make_final_task(transwarp::parallel{threads}, f3, task2);
    } else {
        task3 = make_final_task(transwarp::sequenced{}, f3, task2);
    }
    task3->schedule();
    try {
        task3->get_future().get();
        ASSERT_TRUE(false);
    } catch (const std::logic_error& e) {
        ASSERT_EQUAL(std::string("from f1"), e.what());
    }
}

TEST(task_with_exception_thrown) {
    make_test_task_with_exception_thrown(0);
    make_test_task_with_exception_thrown(1);
    make_test_task_with_exception_thrown(2);
    make_test_task_with_exception_thrown(3);
    make_test_task_with_exception_thrown(4);
}

TEST(cancel_with_schedule_called_before_in_parallel_and_uncancel) {
    auto f0 = [] { return 42; };
    auto f1 = [] (int x) { return x + 13; };
    auto task1 = make_task(f0);
    auto task2 = make_final_task(transwarp::parallel{2}, f1, task1);
    task2->set_pause(true);
    task2->schedule();
    task2->set_cancel(true);
    task2->set_pause(false);
    ASSERT_THROW(transwarp::task_canceled, [task2] { task2->get_future().get(); });
    task2->set_cancel(false);
    task2->schedule();
    ASSERT_EQUAL(55, task2->get_future().get());
}

TEST(cancel_with_schedule_called_after) {
    auto f0 = [] { return 42; };
    auto f1 = [] (int x) { return x + 13; };
    auto task1 = make_task(f0);
    auto task2 = make_final_task(transwarp::sequenced{}, f1, task1);
    task2->set_cancel(true);
    task2->schedule();
    ASSERT_FALSE(task2->get_future().valid());
}

TEST(itask) {
    std::shared_ptr<transwarp::ifinal_task<int>> final;
    {
        auto f0 = [] { return 42; };
        auto f1 = [] (int x) { return x + 13; };
        auto task1 = make_task(f0);
        auto task2 = make_final_task(transwarp::parallel{2}, f1, task1);
        final = task2;
    }
    final->schedule();
    ASSERT_EQUAL(55, final->get_future().get());
}

TEST(set_pause_without_thread_pool) {
    auto f0 = [] { return 42; };
    auto f1 = [] (int x) { return x + 13; };
    auto task1 = make_task(f0);
    auto task2 = make_final_task(transwarp::sequenced{}, f1, task1);
    task2->set_pause(true);

    std::thread([task2] {
        task2->schedule();
    }).detach();

    while (!task2->get_future().valid()) {}

    auto future = task2->get_future();
    ASSERT_TRUE(std::future_status::timeout == future.wait_for(std::chrono::microseconds(50)));

    task2->set_pause(false);
    ASSERT_EQUAL(55, future.get());
}

TEST(set_pause_with_thread_pool) {
    auto f0 = [] { return 42; };
    auto f1 = [] (int x) { return x + 13; };
    auto task1 = make_task(f0);
    auto task2 = make_final_task(transwarp::parallel{2}, f1, task1);
    task2->set_pause(true);
    task2->schedule();

    auto future = task2->get_future();
    ASSERT_TRUE(std::future_status::timeout == future.wait_for(std::chrono::microseconds(50)));

    task2->set_pause(false);
    ASSERT_EQUAL(55, future.get());
}

TEST(sequenced) {
    transwarp::sequenced seq;
    ASSERT_TRUE(typeid(seq).hash_code() > 0);
}

TEST(parallel) {
    transwarp::parallel par{2};
    ASSERT_EQUAL(2, par.n_threads());
    ASSERT_THROW(transwarp::transwarp_error, []{ transwarp::parallel{0}; });
}

COLLECTION(test_examples) {

TEST(basic_with_three_tasks) {
    std::ostringstream os;
    examples::basic_with_three_tasks(os);
    const std::string expected = "result = 55.3\nresult = 58.8\n";
    ASSERT_EQUAL(expected, os.str());
}

void make_test_statistical_keys_facts(bool parallel) {
    std::ostringstream os;
    examples::statistical_key_facts(os, 10000, parallel);
    ASSERT_GREATER(os.str().size(), 0);
}

TEST(statistical_key_facts) {
    make_test_statistical_keys_facts(false);
    make_test_statistical_keys_facts(true);
}

} // test_examples
} // test_transwarp

