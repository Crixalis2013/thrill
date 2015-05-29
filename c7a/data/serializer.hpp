/*******************************************************************************
 * c7a/data/serializer.hpp
 *
 * Part of Project c7a.
 *
 *
 * This file has no license. Only Chuck Norris can compile it.
 ******************************************************************************/

#pragma once
#ifndef C7A_DATA_SERIALIZER_HEADER
#define C7A_DATA_SERIALIZER_HEADER

#include <string>
#include <cstring>
#include <utility>
#include <cassert>

//TODO(ts) this copies data. That is bad and makes me sad.

namespace c7a {
namespace data {

//! internal representation of data elements
typedef std::string Blob;

//! \namespace namespace to hide the implementations of serializers
namespace serializers {

template <class T>
struct Impl;

//! identity serializer from string to string
template <>
struct Impl<std::string>{
    static std::string Serialize(const std::string& x) {
        return x;
    }

    static std::string Deserialize(const std::string& x) {
        return x;
    }
};

//! serializer for int
template <>
struct Impl<int>{
    static std::string Serialize(const int& x) {
        return std::to_string(x);
    }

    static int Deserialize(const std::string& x) {
        return std::stoi(x);
    }
};

//! serializer for double
template <>
struct Impl<double>{
    static std::string Serialize(const double& x) {
        return std::to_string(x);
    }

    static double Deserialize(const std::string& x) {
        return std::stod(x);
    }
};

//! serializer for (string, int) tuples
template <>
struct Impl<std::pair<std::string, int> >{
    static std::string Serialize(const std::pair<std::string, int>& x) {
        std::size_t len = sizeof(int) + x.first.size();
        char result[len];
        std::memcpy(result, &(x.second), sizeof(int));
        std::memcpy(result + sizeof(int), x.first.c_str(), x.first.size());
        return std::string(result, len);
    }

    static std::pair<std::string, int> Deserialize(const std::string& x) {
        int i;
        std::size_t str_len = x.size() - sizeof(int);
        std::memcpy(&i, x.c_str(), sizeof(int));
        std::string s(x, sizeof(int), str_len);
        return std::pair<std::string, int>(s, i);
    }
};

//! serializer for (int, int) tuples
//struct MyStruct
//{
//    int key;
//    int count;
//
//    // only initializing constructor, no default construction possible.
//    explicit MyStruct(int k, int c) : key(k), count(c)
//    { }
//};
//template <>
//struct Impl<MyStruct>{
//    static std::string Serialize(const MyStruct& s) {
//        std::size_t len = 2 * sizeof(int);
//        char result[len];
//        std::memcpy(result, &(s.key), sizeof(int));
//        std::memcpy(result + sizeof(int), &(s.count), sizeof(int));
//        return std::string(result, len);
//    }
//
//    static MyStruct Deserialize(const std::string& x) {
//        int i, j;
//        std::memcpy(&i, x.c_str(), sizeof(int));
//        std::memcpy(&j, (x+sizeof(int)).c_str(), sizeof(int));
//        MyStruct s(i, j);
//        return s;
//    }
//};

//! binary serializer for any integral type, usable as template.
template <typename Type>
struct GenericImpl {
    static std::string Serialize(const Type& v) {
        return std::string(reinterpret_cast<const char*>(&v), sizeof(v));
    }

    static Type        Deserialize(const std::string& s) {
        assert(s.size() == sizeof(Type));
        return Type(*reinterpret_cast<const Type*>(s.data()));
    }
};

template <>
struct Impl<std::pair<int, int> >: public GenericImpl<std::pair<int, int> >
{ };

} // namespace serializers

//! Serialize the type to std::string
template <class T>
inline std::string Serialize(const T& x) {
    return serializers::Impl<T>::Serialize(x);
}

//! Deserialize the std::string to the given type
template <class T>
inline T Deserialize(const std::string& x) {
    return serializers::Impl<T>::Deserialize(x);
}

} // namespace data
} // namespace c7a

#endif // !C7A_DATA_SERIALIZER_HEADER

/******************************************************************************/
