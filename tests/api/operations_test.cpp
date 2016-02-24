/*******************************************************************************
 * tests/api/operations_test.cpp
 *
 * Part of Project Thrill - http://project-thrill.org
 *
 * Copyright (C) 2015 Alexander Noe <aleexnoe@gmail.com>
 * Copyright (C) 2015 Timo Bingmann <tb@panthema.net>
 *
 * All rights reserved. Published under the BSD-2 license in the LICENSE file.
 ******************************************************************************/

#include <thrill/api/allgather.hpp>
#include <thrill/api/cache.hpp>
#include <thrill/api/collapse.hpp>
#include <thrill/api/distribute.hpp>
#include <thrill/api/distribute_from.hpp>
#include <thrill/api/gather.hpp>
#include <thrill/api/generate.hpp>
#include <thrill/api/generate_from_file.hpp>
#include <thrill/api/prefixsum.hpp>
#include <thrill/api/read_lines.hpp>
#include <thrill/api/size.hpp>
#include <thrill/api/sum.hpp>
#include <thrill/api/window.hpp>
#include <thrill/api/write_lines.hpp>
#include <thrill/api/write_lines_many.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <random>
#include <string>
#include <vector>

using namespace thrill; // NOLINT

class Integer
{
public:
    explicit Integer(size_t v)
        : value_(v) { }

    size_t value() const { return value_; }

    static const bool thrill_is_fixed_size = true;
    static const size_t thrill_fixed_size = sizeof(size_t);

    template <typename Archive>
    void ThrillSerialize(Archive& ar) const {
        ar.template PutRaw<size_t>(value_);
    }

    template <typename Archive>
    static Integer ThrillDeserialize(Archive& ar) {
        return Integer(ar.template GetRaw<size_t>());
    }

    friend std::ostream& operator << (std::ostream& os, const Integer& i) {
        return os << i.value_;
    }

protected:
    size_t value_;
};

TEST(Operations, DistributeAndAllGatherElements) {

    std::function<void(Context&)> start_func =
        [](Context& ctx) {

            static const size_t test_size = 1024;

            std::vector<size_t> in_vector;

            // generate data everywhere
            for (size_t i = 0; i < test_size; ++i) {
                in_vector.push_back(i);
            }

            // "randomly" shuffle.
            std::default_random_engine gen(123456);
            std::shuffle(in_vector.begin(), in_vector.end(), gen);

            DIA<size_t> integers = Distribute(ctx, in_vector).Collapse();

            std::vector<size_t> out_vec = integers.AllGather();

            std::sort(out_vec.begin(), out_vec.end());

            ASSERT_EQ(test_size, out_vec.size());
            for (size_t i = 0; i < out_vec.size(); ++i) {
                ASSERT_EQ(i, out_vec[i]);
            }
        };

    api::RunLocalTests(start_func);
}

TEST(Operations, DistributeFromAndAllGatherElements) {

    std::function<void(Context&)> start_func =
        [](Context& ctx) {

            static const size_t test_size = 1024;

            std::vector<size_t> in_vector;

            if (ctx.my_rank() == 0) {
                // generate data only on worker 0.
                for (size_t i = 0; i < test_size; ++i) {
                    in_vector.push_back(i);
                }

                std::random_shuffle(in_vector.begin(), in_vector.end());
            }

            DIA<size_t> integers = DistributeFrom(ctx, in_vector, 0).Collapse();

            std::vector<size_t> out_vec = integers.AllGather();

            std::sort(out_vec.begin(), out_vec.end());

            ASSERT_EQ(test_size, out_vec.size());
            for (size_t i = 0; i < out_vec.size(); ++i) {
                ASSERT_EQ(i, out_vec[i]);
            }
        };

    api::RunLocalTests(start_func);
}

TEST(Operations, DistributeAndGatherElements) {

    std::function<void(Context&)> start_func =
        [](Context& ctx) {

            static const size_t test_size = 1024;

            std::vector<size_t> in_vector;

            // generate data everywhere
            for (size_t i = 0; i < test_size; ++i) {
                in_vector.push_back(i);
            }

            // "randomly" shuffle.
            std::default_random_engine gen(123456);
            std::shuffle(in_vector.begin(), in_vector.end(), gen);

            DIA<size_t> integers = Distribute(ctx, in_vector).Cache();

            std::vector<size_t> out_vec = integers.Gather(0);

            std::sort(out_vec.begin(), out_vec.end());

            if (ctx.my_rank() == 0) {
                ASSERT_EQ(test_size, out_vec.size());
                for (size_t i = 0; i < out_vec.size(); ++i) {
                    ASSERT_EQ(i, out_vec[i]);
                }
            }
            else {
                ASSERT_EQ(0u, out_vec.size());
            }
        };

    api::RunLocalTests(start_func);
}

TEST(Operations, GenerateIntegers) {

    static const size_t test_size = 1000;

    std::function<void(Context&)> start_func =
        [](Context& ctx) {

            auto integers = Generate(
                ctx,
                [](const size_t& index) { return index; },
                test_size);

            std::vector<size_t> out_vec = integers.AllGather();

            ASSERT_EQ(test_size, out_vec.size());

            for (size_t i = 0; i < test_size; ++i) {
                ASSERT_EQ(i, out_vec[i]);
            }
        };

    api::RunLocalTests(start_func);
}

TEST(Operations, MapResultsCorrectChangingType) {

    std::function<void(Context&)> start_func =
        [](Context& ctx) {

            auto integers = Generate(
                ctx,
                [](const size_t& index) -> size_t {
                    return index + 1;
                },
                16);

            std::function<double(size_t)> double_elements =
                [](size_t in) {
                    return 2.0 * static_cast<double>(in);
                };

            auto doubled = integers.Map(double_elements);

            std::vector<double> out_vec = doubled.AllGather();

            size_t i = 1;
            for (double& element : out_vec) {
                ASSERT_DOUBLE_EQ(element, (2.0 * static_cast<double>(i++)));
            }

            ASSERT_EQ(16u, out_vec.size());
            static_assert(std::is_same<decltype(doubled)::ValueType, double>::value, "DIA must be double");
            static_assert(std::is_same<decltype(doubled)::StackInput, size_t>::value, "Node must be size_t");
        };

    api::RunLocalTests(start_func);
}

TEST(Operations, FlatMapResultsCorrectChangingType) {

    std::function<void(Context&)> start_func =
        [](Context& ctx) {

            auto integers = Generate(
                ctx,
                [](const size_t& index) -> size_t {
                    return index;
                },
                16);

            auto flatmap_double =
                [](size_t in, auto emit) {
                    emit(static_cast<double>(2 * in));
                    emit(static_cast<double>(2 * (in + 16)));
                };

            auto doubled = integers.FlatMap<double>(flatmap_double);

            std::vector<double> out_vec = doubled.AllGather();

            ASSERT_EQ(32u, out_vec.size());

            for (size_t i = 0; i != out_vec.size() / 2; ++i) {
                ASSERT_DOUBLE_EQ(out_vec[2 * i + 0],
                                 2.0 * static_cast<double>(i));

                ASSERT_DOUBLE_EQ(out_vec[2 * i + 1],
                                 2.0 * static_cast<double>(i + 16));
            }

            static_assert(
                std::is_same<decltype(doubled)::ValueType, double>::value,
                "DIA must be double");

            static_assert(
                std::is_same<decltype(doubled)::StackInput, size_t>::value,
                "Node must be size_t");
        };

    api::RunLocalTests(start_func);
}

TEST(Operations, PrefixSumCorrectResults) {

    std::function<void(Context&)> start_func =
        [](Context& ctx) {

            auto integers = Generate(
                ctx,
                [](const size_t& input) {
                    return input + 1;
                },
                16);

            auto prefixsums = integers.PrefixSum();

            std::vector<size_t> out_vec = prefixsums.AllGather();

            size_t ctr = 0;
            for (size_t i = 0; i < out_vec.size(); i++) {
                ctr += i + 1;
                ASSERT_EQ(out_vec[i], ctr);
            }

            ASSERT_EQ((size_t)16, out_vec.size());
        };

    api::RunLocalTests(start_func);
}

TEST(Operations, PrefixSumFacultyCorrectResults) {

    std::function<void(Context&)> start_func =
        [](Context& ctx) {

            auto integers = Generate(
                ctx,
                [](const size_t& input) {
                    return input + 1;
                },
                10);

            auto prefixsums = integers.PrefixSum(
                [](size_t in1, size_t in2) {
                    return in1 * in2;
                }, 1);

            std::vector<size_t> out_vec = prefixsums.AllGather();

            size_t ctr = 1;
            for (size_t i = 0; i < out_vec.size(); i++) {
                ctr *= i + 1;
                ASSERT_EQ(out_vec[i], ctr);
            }

            ASSERT_EQ(10u, out_vec.size());
        };

    api::RunLocalTests(start_func);
}

TEST(Operations, GenerateAndSumHaveEqualAmount1) {

    std::default_random_engine generator(std::random_device { } ());
    std::uniform_int_distribution<int> distribution(1000, 10000);

    size_t generate_size = distribution(generator);

    std::function<void(Context&)> start_func =
        [generate_size](Context& ctx) {

            auto input = GenerateFromFile(
                ctx,
                "inputs/test1",
                [](const std::string& line) {
                    return std::stoi(line);
                },
                generate_size);

            auto ones = input.Map([](int) {
                                      return 1;
                                  });

            auto add_function = [](int in1, int in2) {
                                    return in1 + in2;
                                };

            ASSERT_EQ((int)generate_size + 42, ones.Sum(add_function, 42));
        };

    api::RunLocalTests(start_func);
}

TEST(Operations, GenerateAndSumHaveEqualAmount2) {

    std::function<void(Context&)> start_func =
        [](Context& ctx) {

            // TODO(ms): Replace this with some test-specific rendered file
            auto input = ReadLines(ctx, "inputs/test1")
                         .Map([](const std::string& line) {
                                  return std::stoi(line);
                              });

            auto ones = input.Map([](int in) {
                                      return in;
                                  });

            auto add_function = [](int in1, int in2) {
                                    return in1 + in2;
                                };

            ASSERT_EQ(136, ones.Sum(add_function));
            ASSERT_EQ(16u, ones.Size());
        };

    api::RunLocalTests(start_func);
}

TEST(Operations, WindowCorrectResults) {

    static const bool debug = false;
    static const size_t test_size = 144;
    static const size_t window_size = 10;

    std::function<void(Context&)> start_func =
        [](Context& ctx) {

            sLOG << ctx.num_hosts();

            auto integers = Generate(
                ctx,
                [](const size_t& input) { return input * input; },
                test_size);

            auto window = integers.Window(
                window_size, [](size_t rank,
                                const common::RingBuffer<size_t>& window) {

                    // check received window
                    die_unequal(window_size, window.size());

                    for (size_t i = 0; i < window.size(); ++i) {
                        sLOG << rank + i << window[i];
                        die_unequal((rank + i) * (rank + i), window[i]);
                    }

                    // return rank to check completeness
                    return Integer(rank);
                });

            // check rank completeness

            std::vector<Integer> out_vec = window.AllGather();

            if (ctx.my_rank() == 0)
                sLOG << common::Join(" - ", out_vec);

            for (size_t i = 0; i < out_vec.size(); i++) {
                ASSERT_EQ(i, out_vec[i].value());
            }

            ASSERT_EQ(test_size - window_size + 1, out_vec.size());
        };

    api::RunLocalTests(start_func);
}

TEST(Operations, FilterResultsCorrectly) {

    std::function<void(Context&)> start_func =
        [](Context& ctx) {

            auto integers = Generate(
                ctx,
                [](const size_t& index) {
                    return (size_t)index + 1;
                },
                16);

            std::function<bool(size_t)> even = [](size_t in) {
                                                   return (in % 2 == 0);
                                               };

            auto doubled = integers.Filter(even);

            std::vector<size_t> out_vec = doubled.AllGather();

            size_t i = 1;

            for (size_t element : out_vec) {
                ASSERT_EQ(element, (i++ *2));
            }

            ASSERT_EQ(8u, out_vec.size());
        };

    api::RunLocalTests(start_func);
}

TEST(Operations, DIACasting) {

    std::function<void(Context&)> start_func =
        [](Context& ctx) {

            auto even = [](size_t in) {
                            return (in % 2 == 0);
                        };

            auto integers = Generate(
                ctx,
                [](const size_t& index) {
                    return index + 1;
                },
                16);

            DIA<size_t> doubled = integers.Filter(even).Collapse();

            std::vector<size_t> out_vec = doubled.AllGather();

            size_t i = 1;

            for (size_t element : out_vec) {
                ASSERT_EQ(element, (i++ *2));
            }

            ASSERT_EQ(8u, out_vec.size());
        };

    api::RunLocalTests(start_func);
}

TEST(Operations, ForLoop) {

    std::function<void(Context&)> start_func =
        [](Context& ctx) {

            auto integers = Generate(
                ctx,
                [](const size_t& index) -> size_t {
                    return index;
                },
                16);

            auto flatmap_duplicate = [](size_t in, auto emit) {
                                         emit(in);
                                         emit(in);
                                     };

            auto map_multiply = [](size_t in) {
                                    return 2 * in;
                                };

            DIA<size_t> squares = integers.Collapse();

            // run loop four times, inflating DIA of 16 items -> 256
            for (size_t i = 0; i < 4; ++i) {
                auto pairs = squares.FlatMap(flatmap_duplicate);
                auto multiplied = pairs.Map(map_multiply);
                squares = multiplied.Collapse();
            }

            std::vector<size_t> out_vec = squares.AllGather();

            ASSERT_EQ(256u, out_vec.size());
            for (size_t i = 0; i != 256; ++i) {
                ASSERT_EQ(out_vec[i], 16 * (i / 16));
            }
            ASSERT_EQ(256u, squares.Size());
        };

    api::RunLocalTests(start_func);
}

TEST(Operations, WhileLoop) {

    std::function<void(Context&)> start_func =
        [](Context& ctx) {

            auto integers = Generate(
                ctx,
                [](const size_t& index) -> size_t {
                    return index;
                },
                16);

            auto flatmap_duplicate = [](size_t in, auto emit) {
                                         emit(in);
                                         emit(in);
                                     };

            auto map_multiply = [](size_t in) {
                                    return 2 * in;
                                };

            DIA<size_t> squares = integers.Collapse();
            size_t sum = 0;

            // run loop four times, inflating DIA of 16 items -> 256
            while (sum < 256) {
                auto pairs = squares.FlatMap(flatmap_duplicate);
                auto multiplied = pairs.Map(map_multiply);
                squares = multiplied.Collapse();
                sum = squares.Size();
            }

            std::vector<size_t> out_vec = squares.AllGather();

            ASSERT_EQ(256u, out_vec.size());
            for (size_t i = 0; i != 256; ++i) {
                ASSERT_EQ(out_vec[i], 16 * (i / 16));
            }
            ASSERT_EQ(256u, squares.Size());
        };

    api::RunLocalTests(start_func);
}

/******************************************************************************/
