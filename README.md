# Slices-in-Cpp
This is not a complete library in any way. It exists only to spread out the idea.

Idea ?
-------
From Golang slices. The 'indexing' expression itself be used
to provide the range over which the slice is operating.

How this works in Go ?
-----------------------
Go maintains a lightweight header structure which carries some basic information
like the start of the underlying container, its length, its capacity etc.
This information is easier to pass by copy, as the underlying container
does not get copied.

This makes it possible to work on 2 different sections of the array/underlying
container or rather makes it easier to work with subsections of the container
at different places at the same time.

Why required in C++ ?
-----------------------
Currently, iteration in C++ is supported by 'iterators'.
Iterators abstract the underlying datastructure and provides a unified
interface to walk over the structure.
But with the available set of API's there is more typing involved
with achieving things like slicing of vectors/list/array.

Usage
-----
std::vector<int> vec {1,2,3,4,5,6};
auto vec_s = make_slice(vec); // vec_s now knows about the complete range of vec
vec_s = make_slice(vec, 5); // vec_s knows only about 0 to 4 elements
vec_s = make_slice(vec, 2, 5); // vec_s knows about 2nd to 4th element inclusive

auto new_s = make_slice(vec);
auto copy_new_s = new_s(start, 3);
copy_new_s = new_s(4,end);
