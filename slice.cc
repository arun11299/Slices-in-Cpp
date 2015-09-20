#include <iostream>
#include <type_traits>
#include <vector>
#include <list>
#include <string>
#include <sstream>
#include <exception>
#include <cassert>
#include <limits>
#include <utility>

/*
 * Class OutOfRangeException
 * Exception thrown when indexed out of bounds.
 * This exception must be only thrown in debug builds.
 */

// its a big nope
static const size_t NOPE = std::numeric_limits<size_t>::max();

class OutOfRangeException: public std::runtime_error
{
public:
    OutOfRangeException(size_t container_size, 
                        size_t indexed_size): std::runtime_error("Out of range")
                                            , container_size_(container_size)
                                            , indexed_size_(indexed_size)
    {}

    virtual const char* what() const throw() {
        oss_.clear();
        oss_ << std::runtime_error::what() << "\n"
        << "Container size = " << container_size_ << "\n"
        << "Indexed size = " << indexed_size_;

        return oss_.str().c_str();
    }

    virtual ~OutOfRangeException() {}
private:
    size_t container_size_ = 0;
    size_t indexed_size_ = 0;
    static std::ostringstream oss_;
};

std::ostringstream OutOfRangeException::oss_;



enum Position {
    beg = 0,
    end,
    all,
};

//Fwd decl slice
template <typename> class slice;

//NOTE: slice in no way controls the life time of the
// underlying container vector or list
// The lifetime is controlled by external agents, you!

template <typename Container>
slice<Container>
make_slice(Container& cont, size_t end_pos)
{
    return slice<Container>(cont, end_pos);
}

template <typename Container>
slice<Container>
make_slice(Container& cont, size_t start_pos, size_t end_pos)
{
    return slice<Container>(cont, start_pos, end_pos);
}


template <typename Container>
size_t len(const slice<Container>& slice) 
{
    return slice.len();
}


/*
 * Cliqued detail namespace hiding the implementation stuff
 * which are container based.
 */
namespace detail {

    //ATTN: The second template parameter to this base class
    // is really unfortunate.
    // I cannot use Derived::container_type as during compilation
    // 'Derived' is still an incomplete type and this leads
    // to circular reference and hence cannot be resolved.
    // auto return is a solution but that is creepy
    // and will be distasteful in this case.

    template <class Derived, class Container>
    class BaseSlice {
    public:
        virtual slice<Container> operator() (size_t start_pos, size_t end_pos);
        virtual slice<Container> operator() (Position pos, int64_t end_pos);
        virtual slice<Container> operator() (int64_t start_pos, Position pos);
        virtual slice<Container> operator() (Position pos);

        virtual size_t len() const noexcept;
    };

    template <class Derived, class Container>
    auto BaseSlice<Derived, Container>::operator() (size_t start_pos, size_t end_pos)
    -> slice<Container>
    {
        auto *derived = static_cast<Derived*>(this);
        //TODO: throw out of range
#ifndef NDEBUG
        size_t pindex = NOPE;
        if (start_pos > derived->len()) pindex = start_pos;
        if (end_pos < start_pos) pindex = end_pos;
        if (end_pos > derived->epos_) pindex = end_pos;

        if (pindex != NOPE) throw OutOfRangeException(derived->cont_.size(),
                                                         pindex);
#endif

        return make_slice(derived->cont_,
                          derived->spos_ + start_pos,
                          derived->spos_ + end_pos);
    }

    template <class Derived, class Container>
    auto BaseSlice<Derived, Container>::operator() (Position pos, int64_t end_pos)
    -> slice<Container>
    {
        assert(pos != all);
        assert(pos != end);

        auto *derived = static_cast<Derived*>(this);

        //TODO: throw out of range
        return make_slice(derived->cont_,
                          derived->spos_,
                          derived->spos_ + end_pos);
    }

    template <class Derived, class Container>
    auto BaseSlice<Derived, Container>::operator() (int64_t start_pos, Position pos)
    -> slice<Container>
    {
        assert(pos != all);
        assert(pos != beg);

        auto *derived = static_cast<Derived*>(this);

        //TODO: throw out of range
        return make_slice(derived->cont_,
                          derived->spos_ + start_pos,
                          derived->epos_);
    }

    template <class Derived, class Container>
    auto BaseSlice<Derived, Container>::operator() (Position pos)
    -> slice<Container>
    {
        assert(pos == all);

        auto *derived = static_cast<Derived*>(this);

        return make_slice(derived->cont_,
                          derived->spos_,
                          derived->epos_);
    }

    template <class Derived, class Container>
    size_t BaseSlice<Derived, Container>::len() const noexcept
    {
        return static_cast<const Derived*>(this)->len();
    }

    /*
     * Class ContiguousContSlice
     */
    template <typename Container>
    class ContiguousContSlice: public BaseSlice<ContiguousContSlice<Container>, Container>
    {
    public:
        using container_type = Container;
        using elem_type = typename container_type::value_type;
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;
        using size_type = typename container_type::size_type;
        using self_type = ContiguousContSlice<Container>;
        using reference = typename Container::reference;
        using const_reference = typename Container::const_reference;
        friend class BaseSlice<self_type, container_type>;

    public:
        ContiguousContSlice(container_type& cont, size_t end_pos);

        ContiguousContSlice(container_type& cont, size_t start_pos,
                    size_t end_pos);

        self_type operator=(const self_type& other) {
            cont_ = other.cont_;
            spos_ = other.spos_;
            epos_ = other.epos_;

            return *this;
        }

    public:
        iterator begin() noexcept;
        const_iterator begin() const noexcept;
        const_iterator cbegin() const noexcept;

        iterator end() noexcept;
        const_iterator end() const noexcept;
        const_iterator cend() const noexcept;

        reference operator[] (size_type index);
        const_reference operator[] (size_type index) const;

    public:
        size_t len() const noexcept { return epos_ - spos_; }

    private:
        // These will always be index to the actual
        // container
        size_t spos_ = 0;
        size_t epos_ = 0;
        container_type& cont_;
    };

    template <typename Cont>
    ContiguousContSlice<Cont>::ContiguousContSlice(
            ContiguousContSlice<Cont>::container_type& cont,
                                       size_t end_pos)
    : cont_(cont)
    , epos_(end_pos)
    { }


    template <typename Cont>
    ContiguousContSlice<Cont>::ContiguousContSlice(
            ContiguousContSlice<Cont>::container_type& cont,
                                       size_t start_pos,
                                       size_t end_pos)
    : cont_(cont)
    , spos_(start_pos)
    , epos_(end_pos)
    { }


    template <typename Cont>
    auto ContiguousContSlice<Cont>::begin() noexcept 
                                -> typename Cont::iterator
    {
        return cont_.begin() + spos_;
    }

    template <typename Cont>
    auto ContiguousContSlice<Cont>::begin() const noexcept 
                                -> typename Cont::const_iterator
    {
        return cont_.begin() + spos_;
    }

    template <typename Cont>
    auto ContiguousContSlice<Cont>::cbegin() const noexcept 
                                -> typename Cont::const_iterator
    {
        return begin();
    }

    template <typename Cont>
    auto ContiguousContSlice<Cont>::end() noexcept 
                                -> typename Cont::iterator
    {
        return cont_.begin() + epos_;
    }

    template <typename Cont>
    auto ContiguousContSlice<Cont>::end() const noexcept 
                                -> typename Cont::const_iterator
    {
        return cont_.begin() + epos_;
    }

    template <typename Cont>
    auto ContiguousContSlice<Cont>::cend() const noexcept 
                                -> typename Cont::const_iterator
    {
        return end();
    }

    template <typename Cont>
    auto ContiguousContSlice<Cont>::operator[] (typename Cont::size_type index) 
                                -> typename Cont::reference
    {
        assert(index < len());
        return cont_[spos_ + index];
    }

    template <typename Cont>
    auto ContiguousContSlice<Cont>::operator[] (typename Cont::size_type index) const
                                -> typename Cont::const_reference
    {
        assert(index < len());
        return cont_[spos_ + index];
    }

    /*
     * Class NodeContSlice
     */
    //TODO: thinking not to do... :P
    template <typename Container>
    class NodeContSlice: public BaseSlice<NodeContSlice<Container>, Container>{
    };

};

/*
any (s == t[i : i + len(s)] for i in range(len(t) - len(s)))

C++11 algo has added this any_of stuff..
Looks like we can have string matching just like
above in c++ too....fingers crossed
*/


template <typename Container>
class slice : public Container
{
public: // Useful typedefs
    using ElemType = typename Container::value_type;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using self_type = slice<Container>;
    using reference = typename Container::reference;
    using const_reference = typename Container::const_reference;
    using size_type = typename Container::size_type;

public: //Ctors

    // Constructor flavour 1
    slice(Container& cont, size_t end_pos);

    // Constructor flavour 2
    slice(Container& cont, size_t start_pos, size_t end_pos);

public:
    iterator begin() noexcept              { return impl_.begin(); }
    const_iterator begin()  const noexcept { return impl_.begin(); }
    const_iterator cbegin() const noexcept { return begin();       }

    iterator end() noexcept              { return impl_.end(); }
    const_iterator end()  const noexcept { return impl_.end(); }
    const_iterator cend() const noexcept { return end();       }

public:
    size_t len() const noexcept                      { return impl_.len(); }

    self_type operator() (Position a, int64_t b)     { return impl_(a, b); }
    self_type operator() (int64_t a, Position b)     { return impl_(a, b); }
    self_type operator() (int64_t a, int64_t b)      { return impl_(a, b); }
    self_type operator() (Position a)                { return impl_(a); }


    reference operator[] (size_type index)           { return impl_[index]; }
    const_reference operator[] (size_type index) const { return impl_[index]; }


private: 

private:
    using SliceType = typename std::conditional<
                          std::is_same<Container, std::vector<ElemType>>::value,
                          detail::ContiguousContSlice<std::vector<ElemType>>,
                          typename std::conditional<
                              std::is_same<Container, std::string>::value,
                              detail::ContiguousContSlice<std::string>,
                              detail::NodeContSlice<std::list<ElemType>>
                          >::type
                        >::type;

    SliceType impl_;
};

template <typename Container>
slice<Container>::slice(
                  Container& cont,
                  size_t end_pos): impl_(cont, end_pos)
{
    if (end_pos > cont.size()) 
        throw OutOfRangeException(cont.size(), end_pos);
}

template <typename Container>
slice<Container>::slice(
                      Container& cont,
                      size_t start_pos,
                      size_t end_pos): impl_(cont, start_pos, end_pos)
{
    if (start_pos > cont.size() - 1) {
        throw OutOfRangeException(cont.size(), start_pos);
    } else if (
        end_pos > cont.size() ||
        end_pos < start_pos) {
        throw OutOfRangeException(cont.size(), end_pos);
    }
}

template <typename T>
void print_slice(const slice<T>& slice) {
    for (auto& v: slice) { std::cout << v << " "; }
    std::cout << std::endl;
}

template <typename T>
void print_vec(const std::vector<T>& v) {
    for (auto& e: v) { std::cout << e << " "; }
    std::cout << std::endl;
}


// NOTE: Its super cheap to copy slice, no need
// to worry about const reference and all that shit.
// pure value semantics ? But underlying container
// is shared :( so passing by value is NOT EXACTLY
// passing by value in the sense that the underlying
// container is still modifiable
template <typename C>
bool binary_search(slice<C> slice, typename C::value_type val)
{
    int mid = len(slice)/2 ; // Trying using auto here, it will give you a error
                             // because auto will get deduced to size_t 
    if (slice[mid] == val) return true;
    if (mid == 0) return false;

    if (val > slice[mid]) return binary_search(slice(mid+1, end), val);
    else return binary_search(slice(beg,mid), val);
}


void binary_search_ex() {
    std::vector<int> vec{1,2,3,4,5,6,7,8,9,10};
    print_vec(vec);
    auto vslice = make_slice(vec, vec.size());
    if (binary_search(vslice, 6)) std::cout << "Found" << std::endl;
    else std::cout << "Not found" << std::endl;

    std::string str("!ABCDEFGH");
    auto sslice = make_slice(str, str.size());
    if (binary_search(sslice, '!')) std::cout << "Found" << std::endl;
    else std::cout << "Not Found" << std::endl;
}



int main() {
    std::vector<int> vec{1,2,3,4,5,6,7,8,9,10};

    // all elements
    auto vslice = make_slice(vec, 0, vec.size());
    assert(len(vslice) == 10);

    vslice = vslice(beg, 8);
    std::cout << "Should print 8 elements: " << std::endl;
    print_slice(vslice);
    assert(len(vslice) == 8);

    vslice = vslice(2, end);
    std::cout << "Should print from 3 to 8" << std::endl;
    print_slice(vslice);
    assert(len(vslice) == 6);

    vslice = vslice(beg, len(vslice) - 2);
    std::cout << "Should print from 3 to 6" << std::endl;
    print_slice(vslice);
    assert(len(vslice) == 4);

    vslice = std::move(vslice(1, len(vslice)));
    std::cout << "Should print from 4 to 6" << std::endl;
    print_slice(vslice);
    assert(len(vslice) == 3);

    // reset
    std::cout << "Test operator[]: " << std::endl;

    vslice = make_slice(vec, 0, vec.size());
    for (size_t i = 0; i < len(vslice); i++) {
        vslice[i]++;
    }
    print_slice(vslice);

    binary_search_ex();


    return 0;
}

