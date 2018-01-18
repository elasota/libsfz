// Copyright (c) 2009 Chris Pickel <sfiera@gmail.com>
//
// This file is part of libsfz, a free software project.  You can redistribute it and/or modify it
// under the terms of the MIT License.

#ifndef SFZ_STRING_MAP_HPP_
#define SFZ_STRING_MAP_HPP_

#include <stdlib.h>
#include <map>
#include <memory>
#include <pn/string>
#include <utility>

namespace sfz {

template <typename T, typename Compare = std::less<pn::string_view>>
class StringMap {
  public:
    typedef pn::string_view                        key_type;
    typedef T                                      mapped_type;
    typedef std::pair<const key_type, mapped_type> value_type;
    typedef size_t                                 size_type;

    class iterator;
    class const_iterator;

    StringMap() {}
    explicit StringMap(const StringMap& other);
    ~StringMap() {}

    mapped_type&              operator[](const key_type& key);
    std::pair<iterator, bool> insert(const value_type& pair);

    size_type size() const { return _map.size(); }
    bool      empty() const { return _map.empty(); }

    void      clear() { _map.clear(); }
    void      erase(iterator pos);
    void      erase(iterator start, iterator end);
    size_type erase(const key_type& key) { return _map.erase(key); }

    iterator       find(const key_type& key);
    const_iterator find(const key_type& key) const;

    iterator       begin();
    const_iterator begin() const;
    iterator       end();
    const_iterator end() const;
    iterator       rbegin();
    const_iterator rbegin() const;
    iterator       rend();
    const_iterator rend() const;

    void swap(StringMap& from) { _map.swap(from._map); }

  private:
    struct WrappedValue;
    typedef std::map<pn::string_view, std::shared_ptr<WrappedValue>, Compare> internal_map;
    typedef typename internal_map::iterator                                   wrapped_iterator;
    typedef typename internal_map::const_iterator wrapped_const_iterator;

    struct WrappedValue {
        const pn::string                              key_storage;
        std::pair<const pn::string_view, mapped_type> pair;

        WrappedValue(const pn::string_view& k)
                : key_storage(k.copy()), pair(key_storage, mapped_type()) {}

        WrappedValue(const pn::string_view& k, const mapped_type& v)
                : key_storage(k.copy()), pair(key_storage, v) {}

        WrappedValue(const WrappedValue&) = delete;
        WrappedValue(WrappedValue&&)      = delete;
    };

    template <typename wrapped_iterator>
    class iterator_base {
      public:
        typedef typename wrapped_iterator::iterator_category iterator_category;
        typedef typename StringMap::value_type               value_type;
        typedef typename wrapped_iterator::difference_type   difference_type;
        typedef value_type*                                  pointer;
        typedef value_type&                                  reference;

        iterator_base() {}
        iterator_base(wrapped_iterator it) : _it(it) {}

        reference operator*() const { return _it->second->pair; }
        pointer   operator->() const { return &_it->second->pair; }

        iterator_base& operator++() {
            ++_it;
            return *this;
        }
        iterator_base  operator++(int) { return _it++; }
        iterator_base& operator--() {
            --_it;
            return *this;
        }
        iterator_base operator--(int) { return _it--; }

        bool operator==(iterator_base it) { return _it == it._it; }
        bool operator!=(iterator_base it) { return _it != it._it; }

      private:
        wrapped_iterator _it;
    };

    internal_map _map;

    StringMap& operator=(const StringMap&) = delete;
};

template <typename T, typename Compare>
bool operator==(const StringMap<T, Compare>& x, const StringMap<T, Compare>& y) {
    typedef typename StringMap<T, Compare>::const_iterator iterator;
    iterator x_begin = x.begin(), x_end = x.end(), y_begin = y.begin(), y_end = y.end();
    while ((x_begin != x_end) && (y_begin != y_end)) {
        if ((x_begin->first != y_begin->first) || (x_begin->second != y_begin->second)) {
            return false;
        }
        ++x_begin;
        ++y_begin;
    }
    return (x_begin == x_end) && (y_begin == y_end);
}

template <typename T, typename Compare>
bool operator!=(const StringMap<T, Compare>& x, const StringMap<T, Compare>& y) {
    return !(x == y);
}

template <typename T, typename Compare>
class StringMap<T, Compare>::const_iterator : public iterator_base<wrapped_const_iterator> {
  public:
    const_iterator() {}

  private:
    friend class StringMap;
    friend class iterator;
    const_iterator(wrapped_const_iterator it) : iterator_base<wrapped_const_iterator>(it) {}
};

template <typename T, typename Compare>
class StringMap<T, Compare>::iterator : public iterator_base<wrapped_iterator> {
  public:
    iterator() {}
    iterator(const_iterator it) : iterator_base<wrapped_iterator>(it._it) {}

  private:
    friend class StringMap;
    iterator(wrapped_iterator it) : iterator_base<wrapped_iterator>(it) {}
};

template <typename T, typename Compare>
void StringMap<T, Compare>::erase(iterator pos) {
    _map.erase(pos);
}
template <typename T, typename Compare>
void StringMap<T, Compare>::erase(iterator start, iterator end) {
    _map.erase(start, end);
}

template <typename T, typename Compare>
typename StringMap<T, Compare>::iterator StringMap<T, Compare>::find(const key_type& key) {
    return _map.find(key);
}
template <typename T, typename Compare>
typename StringMap<T, Compare>::const_iterator StringMap<T, Compare>::find(
        const key_type& key) const {
    return _map.find(key);
}

template <typename T, typename Compare>
typename StringMap<T, Compare>::iterator StringMap<T, Compare>::begin() {
    return _map.begin();
}
template <typename T, typename Compare>
typename StringMap<T, Compare>::const_iterator StringMap<T, Compare>::begin() const {
    return _map.begin();
}
template <typename T, typename Compare>
typename StringMap<T, Compare>::iterator StringMap<T, Compare>::end() {
    return _map.end();
}
template <typename T, typename Compare>
typename StringMap<T, Compare>::const_iterator StringMap<T, Compare>::end() const {
    return _map.end();
}
template <typename T, typename Compare>
typename StringMap<T, Compare>::iterator StringMap<T, Compare>::rbegin() {
    return _map.rbegin();
}
template <typename T, typename Compare>
typename StringMap<T, Compare>::const_iterator StringMap<T, Compare>::rbegin() const {
    return _map.rbegin();
}
template <typename T, typename Compare>
typename StringMap<T, Compare>::iterator StringMap<T, Compare>::rend() {
    return _map.rend();
}
template <typename T, typename Compare>
typename StringMap<T, Compare>::const_iterator StringMap<T, Compare>::rend() const {
    return _map.rend();
}

template <typename T, typename Compare>
StringMap<T, Compare>::StringMap(const StringMap& other) {
    for (const value_type& item : other) {
        insert(item);
    }
}

template <typename T, typename Compare>
typename StringMap<T, Compare>::mapped_type& StringMap<T, Compare>::operator[](
        const key_type& key) {
    wrapped_iterator it = _map.find(key);
    if (it == _map.end()) {
        std::shared_ptr<WrappedValue> inserted(new WrappedValue(key));
        _map.insert(typename internal_map::value_type(inserted->key_storage, inserted));
        return inserted->pair.second;
    }
    return it->second->pair.second;
}

template <typename T, typename Compare>
std::pair<typename StringMap<T, Compare>::iterator, bool> StringMap<T, Compare>::insert(
        const value_type& pair) {
    const pn::string_view& key   = pair.first;
    const mapped_type&     value = pair.second;
    wrapped_iterator       it    = _map.find(key);
    if (it == _map.end()) {
        std::shared_ptr<WrappedValue> inserted(new WrappedValue(key, value));
        it = _map.insert(typename internal_map::value_type(inserted->key_storage, inserted)).first;
        return std::make_pair(iterator(it), true);
    } else {
        return std::make_pair(iterator(it), false);
    }
}

}  // namespace sfz

#endif  // SFZ_STRING_MAP_HPP_
