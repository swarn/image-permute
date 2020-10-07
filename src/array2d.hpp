#ifndef ARRAY2D_HPP
#define ARRAY2D_HPP

#include <vector>

// A simple wrapper to present a vector<T> as a row-major 2D array.
template <typename T>
struct array2d
{
    using size_type = typename std::vector<T>::size_type;

    // Construct the array by default-inserting `rows * cols` instances of T.
    array2d(size_type rows, size_type cols)
        : rows{rows}, cols{cols}, data(rows * cols) { }

    // Construct the array by copying frrom an `array2d<U>` where type `U` is
    // convertable to `T`.
    template <typename U>
    explicit array2d(array2d<U> const & other)
        : rows{other.rows}
        , cols{other.cols}
        , data(other.data.begin(), other.data.end()) { }

    T & operator()(size_type row, size_type col)
    {
        return data[row * cols + col];
    }

    T const & operator()(size_type row, size_type col) const
    {
        return data[row * cols + col];
    }

    T & operator[](size_type idx)
    {
        return data[idx];
    }

    T const & operator[](size_type idx) const
    {
        return data[idx];
    }

    size_type size() const
    {
        return data.size();
    }

    size_type const rows;
    size_type const cols;
    std::vector<T> data;
};


#endif
