/*******************************************************************************
 * tests/core/post_hash_table_probing_test.cpp
 *
 * Part of Project c7a.
 *
 * Copyright (C) 2015 Matthias Stumpp <mstumpp@gmail.com>
 *
 * This file has no license. Only Chuck Norris can compile it.
 ******************************************************************************/

#include <c7a/core/reduce_post_probing_table.hpp>
#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>
#include <c7a/net/manager.hpp>

using namespace c7a::data;
using namespace c7a::net;

struct PostTable : public::testing::Test {};

std::pair<int, int> pair(int ele) {
    return std::make_pair(ele, ele);
}

template <typename Key, typename HashFunction = std::hash<Key> >
class CustomKeyHashFunction
        : public c7a::core::PostProbingReduceByHashKey<int> {
public:
    CustomKeyHashFunction(const HashFunction& hash_function = HashFunction())
            : hash_function_(hash_function)
    { }

    template <typename ReducePostProbingTable>
    typename ReducePostProbingTable::index_result
    operator () (Key v, ReducePostProbingTable* ht) const {

        using index_result = typename ReducePostProbingTable::index_result;

        size_t global_index =  v / 2;

        (*ht).NumItems();

        return index_result(global_index);
    }

private:
    HashFunction hash_function_;
};

TEST_F(PostTable, CustomHashFunction) {
    auto key_ex = [](int in) {
                      return in;
                  };

    auto red_fn = [](int in1, int in2) {
                      return in1 + in2;
                  };

    typedef std::function<void (const int&)> EmitterFunction;
    std::vector<EmitterFunction> emitters;
    std::vector<int> writer1;
    emitters.push_back([&writer1](const int value) { writer1.push_back(value); });

    CustomKeyHashFunction<int> cust_hash;
    c7a::core::PostProbingReduceFlushToDefault flush_func;

    c7a::core::ReducePostProbingTable<int, int, int, decltype(key_ex), decltype(red_fn), false,
            c7a::core::PostProbingReduceFlushToDefault, CustomKeyHashFunction<int>>
    table(key_ex, red_fn, emitters, -1, cust_hash, flush_func);

    ASSERT_EQ(0u, writer1.size());
    ASSERT_EQ(0u, table.NumItems());

    for (int i = 0; i < 16; i++) {
        table.Insert(std::move(pair(i)));
    }

    ASSERT_EQ(0u, writer1.size());
    ASSERT_EQ(16u, table.NumItems());

    table.Flush();

    ASSERT_EQ(16u, writer1.size());
    ASSERT_EQ(0u, table.NumItems());
}

TEST_F(PostTable, AddIntegers) {
    auto key_ex = [](int in) {
                      return in;
                  };

    auto red_fn = [](int in1, int in2) {
                      return in1 + in2;
                  };

    typedef std::function<void (const int&)> EmitterFunction;
    std::vector<EmitterFunction> emitters;
    std::vector<int> writer1;
    emitters.push_back([&writer1](const int value) { writer1.push_back(value); });

    c7a::core::ReducePostProbingTable<int, int, int, decltype(key_ex), decltype(red_fn)>
    table(key_ex, red_fn, emitters, -1);

    table.Insert(pair(1));
    table.Insert(pair(2));
    table.Insert(pair(3));

    ASSERT_EQ(3u, table.NumItems());

    table.Insert(pair(2));

    ASSERT_EQ(3u, table.NumItems());
}

TEST_F(PostTable, CreateEmptyTable) {
    auto key_ex = [](int in) {
                      return in;
                  };

    auto red_fn = [](int in1, int in2) {
                      return in1 + in2;
                  };

    typedef std::function<void (const int&)> EmitterFunction;
    std::vector<EmitterFunction> emitters;
    std::vector<int> writer1;
    emitters.push_back([&writer1](const int value) { writer1.push_back(value); });

    c7a::core::ReducePostProbingTable<int, int, int, decltype(key_ex), decltype(red_fn)>
    table(key_ex, red_fn, emitters, -1);

    ASSERT_EQ(0u, table.NumItems());
}

TEST_F(PostTable, FlushIntegers) {
    auto key_ex = [](int in) {
                      return in;
                  };
    auto red_fn = [](int in1, int in2) {
                      return in1 + in2;
                  };

    typedef std::function<void (const int&)> EmitterFunction;
    std::vector<EmitterFunction> emitters;
    std::vector<int> writer1;
    emitters.push_back([&writer1](const int value) { writer1.push_back(value); });

    c7a::core::ReducePostProbingTable<int, int, int, decltype(key_ex), decltype(red_fn)>
    table(key_ex, red_fn, emitters, -1);

    table.Insert(pair(1));
    table.Insert(pair(2));
    table.Insert(pair(3));

    ASSERT_EQ(3u, table.NumItems());

    table.Flush();

    ASSERT_EQ(3u, writer1.size());
    ASSERT_EQ(0u, table.NumItems());

    table.Insert(pair(1));

    ASSERT_EQ(1u, table.NumItems());
}

TEST_F(PostTable, FlushIntegersInSequence) {
    auto key_ex = [](int in) {
                      return in;
                  };
    auto red_fn = [](int in1, int in2) {
                      return in1 + in2;
                  };

    typedef std::function<void (const int&)> EmitterFunction;
    std::vector<EmitterFunction> emitters;
    std::vector<int> writer1;
    emitters.push_back([&writer1](const int value) { writer1.push_back(value); });

    c7a::core::ReducePostProbingTable<int, int, int, decltype(key_ex), decltype(red_fn)>
    table(key_ex, red_fn, emitters, -1);

    table.Insert(pair(1));
    table.Insert(pair(2));
    table.Insert(pair(3));

    ASSERT_EQ(3u, table.NumItems());

    table.Flush();

    ASSERT_EQ(3u, writer1.size());
    ASSERT_EQ(0u, table.NumItems());

    table.Insert(pair(1));

    ASSERT_EQ(1u, table.NumItems());
}

TEST_F(PostTable, MultipleEmitters) {
    std::vector<int> vec1;

    auto key_ex = [](int in) {
                      return in;
                  };

    auto red_fn = [](int in1, int in2) {
                      return in1 + in2;
                  };

    typedef std::function<void (const int&)> EmitterFunction;
    std::vector<EmitterFunction> emitters;
    std::vector<int> writer1;
    std::vector<int> writer2;
    emitters.push_back([&writer1](const int value) { writer1.push_back(value); });
    emitters.push_back([&writer2](const int value) { writer2.push_back(value); });

    c7a::core::ReducePostProbingTable<int, int, int, decltype(key_ex), decltype(red_fn)>
    table(key_ex, red_fn, emitters, -1);

    table.Insert(pair(1));
    table.Insert(pair(2));
    table.Insert(pair(3));

    ASSERT_EQ(3u, table.NumItems());

    table.Flush();

    ASSERT_EQ(0u, table.NumItems());
    ASSERT_EQ(3u, writer1.size());
    ASSERT_EQ(3u, writer2.size());

    table.Insert(pair(1));

    ASSERT_EQ(1u, table.NumItems());
}

TEST_F(PostTable, ComplexType) {
    using StringPair = std::pair<std::string, int>;

    auto key_ex = [](StringPair in) {
                      return in.first;
                  };

    auto red_fn = [](StringPair in1, StringPair in2) {
                      return std::make_pair(in1.first, in1.second + in2.second);
                  };

    typedef std::function<void (const StringPair&)> EmitterFunction;
    std::vector<EmitterFunction> emitters;
    std::vector<StringPair> writer1;
    emitters.push_back([&writer1](const StringPair value) { writer1.push_back(value); });

    c7a::core::ReducePostProbingTable<StringPair, std::string, StringPair, decltype(key_ex), decltype(red_fn)>
    table(key_ex, red_fn, emitters, "");

    table.Insert(std::make_pair("hallo", std::make_pair("hallo", 1)));
    table.Insert(std::make_pair("hello", std::make_pair("hello", 2)));
    table.Insert(std::make_pair("bonjour", std::make_pair("bonjour", 3)));

    ASSERT_EQ(3u, table.NumItems());

    table.Insert(std::make_pair("hello", std::make_pair("hello", 5)));

    ASSERT_EQ(3u, table.NumItems());

    table.Insert(std::make_pair("baguette", std::make_pair("baguette", 42)));

    ASSERT_EQ(4u, table.NumItems());
}

// TODO(ms): add one test with a for loop inserting 10000 items. -> trigger

/******************************************************************************/