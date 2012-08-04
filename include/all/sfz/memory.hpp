// Copyright (c) 2009 Chris Pickel <sfiera@gmail.com>
//
// This file is part of libsfz, a free software project.  You can redistribute it and/or modify it
// under the terms of the MIT License.

#ifndef SFZ_MEMORY_HPP_
#define SFZ_MEMORY_HPP_

#include <stdlib.h>
#include <algorithm>
#include <sfz/macros.hpp>

namespace sfz {

template <typename T, template <typename> class traits> class scoped;
template <typename T, template <typename> class traits>
inline void swap(scoped<T, traits>& x, scoped<T, traits>& y);
template <typename T, template <typename> class traits> class linked;
template <typename T, template <typename> class traits>
void swap(linked<T, traits>& x, linked<T, traits>& y);

template <typename T>
struct ptr_traits {
    typedef T   value_type;
    typedef T*  pointer;
    typedef T&  reference;
    static inline void acquire(pointer t) { }
    static inline void release(pointer t) { delete t; }
};

template <typename T>
struct array_traits {
    typedef T   value_type;
    typedef T*  pointer;
    typedef T&  reference;
    static inline void acquire(pointer t) { }
    static inline void release(pointer t) { delete[] t; }
};

template <typename T>
struct ref_traits {
    typedef T   value_type;
    typedef T*  pointer;
    typedef T&  reference;
    static inline void acquire(pointer t) { if (t) { t->ref(); } }
    static inline void release(pointer t) { if (t) { t->unref(); } }
};

template <typename T, template <typename> class traits>
class scoped {
  public:
    typedef typename traits<T>::value_type  value_type;
    typedef typename traits<T>::pointer     pointer;

    scoped(pointer ptr)
            : _ptr(ptr) {
        traits<T>::acquire(_ptr);
    }

    scoped(const scoped& other)
            : _ptr(other._ptr) {
        traits<T>::acquire(_ptr);
    }

    scoped& operator=(const scoped& other) {
        if (&other != this) {
            reset(other._ptr);
        }
        return *this;
    }

    ~scoped() { reset(); }

    pointer get() const { return _ptr; }

    pointer release() {
        using std::swap;
        pointer ptr = NULL;
        swap(ptr, _ptr);
        return ptr;
    }

    void reset(pointer new_ptr = NULL) {
        traits<T>::release(_ptr);
        _ptr = new_ptr;
        traits<T>::acquire(_ptr);
    }

    friend void swap<>(scoped& x, scoped& y);

  private:
    pointer _ptr;
};

template <typename T>
class scoped_ptr : public scoped<T, ptr_traits> {
  public:
    typedef scoped<T, ptr_traits>               super;
    typedef typename ptr_traits<T>::pointer     pointer;
    typedef typename ptr_traits<T>::reference   reference;

    explicit scoped_ptr(pointer ptr = NULL) : super(ptr) { }
    pointer operator->() const { return this->get(); }
    reference operator*() const { return *this->get(); }

    DISALLOW_COPY_AND_ASSIGN(scoped_ptr);
};

template <typename T>
class scoped_array : public scoped<T, array_traits> {
  public:
    typedef scoped<T, array_traits>             super;
    typedef typename array_traits<T>::pointer   pointer;
    typedef typename array_traits<T>::reference reference;

    explicit scoped_array(pointer ptr = NULL) : super(ptr) { }
    reference operator[](int index) const { return this->get()[index]; }

    DISALLOW_COPY_AND_ASSIGN(scoped_array);
};

template <typename T>
class scoped_ref : public scoped<T, ref_traits> {
  public:
    typedef scoped<T, ref_traits>               super;
    typedef typename ref_traits<T>::pointer     pointer;
    typedef typename ref_traits<T>::reference   reference;

    explicit scoped_ref(pointer ptr = NULL) : super(ptr) { }
    pointer operator->() const { return this->get(); }
    reference operator*() const { return *this->get(); }
};

template <typename T, template <typename> class traits>
inline void swap(scoped<T, traits>& x, scoped<T, traits>& y) {
    using std::swap;
    swap(x._ptr, y._ptr);
}

template <typename T>
inline void swap(scoped_ptr<T>& x, scoped_ptr<T>& y) {
    typename scoped_ptr<T>::super& super_x = x;
    typename scoped_ptr<T>::super& super_y = y;
    swap(super_x, super_y);
}

template <typename T>
inline void swap(scoped_array<T>& x, scoped_array<T>& y) {
    typename scoped_array<T>::super& super_x = x;
    typename scoped_array<T>::super& super_y = y;
    swap(super_x, super_y);
}

template <typename T>
inline void swap(scoped_ref<T>& x, scoped_ref<T>& y) {
    typename scoped_ref<T>::super& super_x = x;
    typename scoped_ref<T>::super& super_y = y;
    swap(super_x, super_y);
}

class linked_group {
  public:
    linked_group();

    void join(linked_group* group);
    bool depart();

    bool unique() const;
    void check_unique() const;

  private:
    linked_group* _prev;
    linked_group* _next;

    DISALLOW_COPY_AND_ASSIGN(linked_group);
};

template <typename T, template <typename> class traits>
class linked {
  public:
    typedef typename traits<T>::value_type  value_type;
    typedef typename traits<T>::pointer     pointer;

    linked(pointer ptr)
            : _ptr(ptr) {
        traits<T>::acquire(_ptr);
    }

    linked(const linked& other) {
        copy(other);
    }

    linked& operator=(const linked& other) {
        if (&other != this) {
            reset();
            copy(other);
        }
        return *this;
    }

    template <typename U>
    linked(const linked<U, traits>& other) {
        copy(other);
    }

    template <typename U>
    linked& operator=(const linked<U, traits>& other) {
        reset();
        copy(other);
        return *this;
    }

    ~linked() { reset(); }

    pointer get() const { return _ptr; }
    bool unique() const { return _group.unique(); }

    pointer release() {
        using std::swap;
        _group.check_unique();
        pointer ptr = NULL;
        swap(ptr, _ptr);
        return ptr;
    }

    void reset(pointer new_ptr = NULL) {
        if (_group.depart()) {
            traits<T>::release(_ptr);
        }
        _ptr = new_ptr;
        traits<T>::acquire(_ptr);
    }

    friend void swap<>(linked& x, linked& y);

  private:
    template <typename U, template <typename V> class other_traits>
    friend class linked;

    template <typename U>
    void copy(const linked<U, traits>& other) {
        _ptr = other._ptr;
        _group.join(&other._group);
    }

    pointer _ptr;
    mutable linked_group _group;
};

template <typename T>
class linked_ptr : public linked<T, ptr_traits> {
  public:
    typedef linked<T, ptr_traits>               super;
    typedef typename ptr_traits<T>::pointer     pointer;
    typedef typename ptr_traits<T>::reference   reference;

    explicit linked_ptr(pointer ptr = NULL) : super(ptr) { }
    pointer operator->() const { return this->get(); }
    reference operator*() const { return *this->get(); }

    template <typename U> linked_ptr(const linked_ptr<U>& other) : super(other) { }
    template <typename U> linked_ptr& operator=(const linked_ptr<U>& other) {
        super::operator=(other);
        return *this;
    }
};

template <typename T>
class linked_array : public linked<T, array_traits> {
  public:
    typedef linked<T, array_traits>             super;
    typedef typename array_traits<T>::pointer   pointer;
    typedef typename array_traits<T>::reference reference;

    explicit linked_array(pointer ptr = NULL) : super(ptr) { }
    reference operator[](int index) const { return this->get()[index]; }

    template <typename U> linked_array(const linked_array<U>& other) : super(other) { }
    template <typename U> linked_array& operator=(const linked_array<U>& other) {
        super::operator=(other);
        return *this;
    }
};

template <typename T>
linked_ptr<T> make_linked_ptr(T* ptr) {
    return linked_ptr<T>(ptr);
}

template <typename T, template <typename> class traits>
void swap(linked<T, traits>& x, linked<T, traits>& y) {
    using std::swap;
    x._group.check_unique();
    y._group.check_unique();
    swap(x._ptr, y._ptr);
}

template <typename T>
void swap(linked_ptr<T>& x, linked_ptr<T>& y) {
    typename linked_ptr<T>::super& super_x = x;
    typename linked_ptr<T>::super& super_y = y;
    swap(super_x, super_y);
}

template <typename T>
void swap(linked_array<T>& x, linked_array<T>& y) {
    typename linked_array<T>::super& super_x = x;
    typename linked_array<T>::super& super_y = y;
    swap(super_x, super_y);
}

}  // namespace sfz

#endif  // SFZ_MEMORY_HPP_
