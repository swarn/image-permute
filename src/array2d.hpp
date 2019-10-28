#ifndef ARRAY2D_HPP
#define ARRAY2D_HPP

#include <vector>


template <typename T>
struct array2d
{
    array2d(int rows, int cols);

    template <typename U>
    array2d(int rows, int cols, std::vector<U> const & values);

    T & operator()(int row, int col);
    T const & operator()(int row, int col) const;
    T & operator[](int idx);
    T const & operator[](int idx) const;

    int rows;
    int cols;
    int size;
    std::vector<T> data;
};

template <typename T>
array2d<T>::array2d(int rows, int cols)
    : rows{rows}, cols{cols}, size{rows * cols}, data(size)
{
}

template <typename T> template <typename U>
array2d<T>::array2d(int rows, int cols, std::vector<U> const & values)
    : rows{rows}, cols{cols}, size{rows * cols}
{
    data.reserve(values.size());
    for (auto const & value: values)
        data.push_back(T(value));
}

template <typename T>
T & array2d<T>::operator()(int row, int col)
{
    return data[row * cols + col];
}

template <typename T>
T const & array2d<T>::operator()(int row, int col) const
{
    return data[row * cols + col];
}

template <typename T>
T & array2d<T>::operator[](int idx)
{
    return data[idx];
}

template <typename T>
T const & array2d<T>::operator[](int idx) const
{
    return data[idx];
}

#endif
