// Copyright (c) 2009 Chris Pickel <sfiera@gmail.com>
//
// This file is part of libsfz, a free software project.  You can redistribute
// it and/or modify it under the terms of the MIT License.

#include "sfz/Bytes.hpp"

#include <algorithm>

namespace sfz {

using std::max;
using std::swap;

namespace {

const size_t kDefaultBytesSize = 16;

}  // namespace

const size_t Bytes::npos = -1;
const size_t BytesPiece::npos = -1;

Bytes::Bytes()
    : _data(new uint8_t[kDefaultBytesSize]),
      _size(0),
      _capacity(kDefaultBytesSize) { }

Bytes::Bytes(const Bytes& bytes)
    : _data(new uint8_t[bytes._capacity]),
      _size(bytes._size),
      _capacity(bytes._capacity) {
    memcpy(_data.get(), bytes._data.get(), _size);
}

Bytes::Bytes(const BytesPiece& bytes)
    : _data(new uint8_t[max(bytes.size(), kDefaultBytesSize)]),
      _size(bytes.size()),
      _capacity(max(bytes.size(), kDefaultBytesSize)) {
    memcpy(_data.get(), bytes.data(), _size);
}

Bytes::Bytes(const uint8_t* data, size_t size)
    : _data(new uint8_t[max(size, kDefaultBytesSize)]),
      _size(size),
      _capacity(max(size, kDefaultBytesSize)) {
    memcpy(_data.get(), data, size);
}

Bytes::Bytes(WriteItem item)
    : _data(new uint8_t[kDefaultBytesSize]),
      _size(0),
      _capacity(kDefaultBytesSize) {
    item.write_to(this);
}

Bytes::Bytes(size_t num, uint8_t byte)
    : _data(new uint8_t[max(num, kDefaultBytesSize)]),
      _size(num),
      _capacity(max(num, kDefaultBytesSize)) {
    memset(_data.get(), byte, num);
}

Bytes::~Bytes() { }

const uint8_t* Bytes::data() const {
    return _data.get();
}

uint8_t* Bytes::mutable_data() const {
    return _data.get();
}

size_t Bytes::size() const {
    return _size;
}

void Bytes::append(const BytesPiece& bytes) {
    append(bytes.data(), bytes.size());
}

void Bytes::append(const uint8_t* data, size_t size) {
    reserve(size + _size);
    memcpy(_data.get() + _size, data, size);
    _size += size;
}

void Bytes::append(WriteItem item) {
    item.write_to(this);
}

void Bytes::append(size_t num, uint8_t byte) {
    reserve(num + _size);
    memset(_data.get() + _size, byte, num);
    _size += num;
}

void Bytes::assign(const BytesPiece& bytes) {
    assign(bytes.data(), bytes.size());
}

void Bytes::assign(const uint8_t* data, size_t size) {
    reserve(size);
    memcpy(_data.get(), data, size);
    _size = size;
}

void Bytes::assign(WriteItem item) {
    clear();
    item.write_to(this);
}

void Bytes::assign(size_t num, uint8_t byte) {
    reserve(num);
    memset(_data.get(), byte, num);
    _size = num;
}

uint8_t Bytes::at(size_t loc) const {
    if (loc >= _size) {
        abort();
    }
    return _data[loc];
}

void Bytes::clear() {
    _size = 0;
}

bool Bytes::empty() const {
    return _size == 0;
}

void Bytes::reserve(size_t capacity) {
    if (_capacity < capacity) {
        size_t new_capacity = _capacity * 2;
        while (new_capacity < capacity) {
            new_capacity *= 2;
        }
        scoped_array<uint8_t> new_data(new uint8_t[new_capacity]);
        memcpy(new_data.get(), _data.get(), _size);
        swap(_data, new_data);
        _capacity = new_capacity;
    }
}

void Bytes::resize(size_t size, uint8_t byte) {
    if (size < _size) {
        _size = size;
    } else {
        reserve(size);
        memset(_data.get() + _size, byte, size - _size);
        _size = size;
    }
}

BytesPiece::BytesPiece()
    : _data(NULL),
      _size(0) { }

BytesPiece::BytesPiece(const Bytes& bytes)
    : _data(bytes.data()),
      _size(bytes.size()) { }

BytesPiece::BytesPiece(const char* data)
    : _data(reinterpret_cast<const uint8_t*>(data)),
      _size(strlen(data)) { }

BytesPiece::BytesPiece(const uint8_t* data, size_t size)
    : _data(data),
      _size(size) { }

const uint8_t* BytesPiece::data() const {
    return _data;
}

size_t BytesPiece::size() const {
    return _size;
}

uint8_t BytesPiece::at(size_t loc) const {
    if (loc >= _size) {
        abort();
    }
    return _data[loc];
}

bool BytesPiece::empty() const {
    return _size == 0;
}

BytesPiece BytesPiece::substr(size_t index) const {
    if (index > _size) {
        abort();
    }
    return BytesPiece(_data + index, _size - index);
}

BytesPiece BytesPiece::substr(size_t index, size_t size) const {
    if (index + size > _size) {
        abort();
    }
    return BytesPiece(_data + index, size);
}

void BytesPiece::shift(size_t size) {
    if (size > _size) {
        abort();
    }
    _data += size;
    _size -= size;
}

void BytesPiece::shift(uint8_t* data, size_t size) {
    shift(size);
    memcpy(data, _data - size, size);
}

void swap(Bytes& x, Bytes& y) {
    swap(x._data, y._data);
    swap(x._size, y._size);
    swap(x._capacity, y._capacity);
}

void swap(BytesPiece& x, BytesPiece& y) {
    swap(x._data, y._data);
    swap(x._size, y._size);
}

// Equality operators.

bool operator==(const Bytes& lhs, const Bytes& rhs) {
    return BytesPiece(lhs) == BytesPiece(rhs);
}

bool operator!=(const Bytes& lhs, const Bytes& rhs) {
    return BytesPiece(lhs) != BytesPiece(rhs);
}

bool operator==(const BytesPiece& lhs, const BytesPiece& rhs) {
    return (lhs.size() == rhs.size())
        && (memcmp(lhs.data(), rhs.data(), lhs.size()) == 0);
}

bool operator!=(const BytesPiece& lhs, const BytesPiece& rhs) {
    return !(lhs == rhs);
}

}  // namespace sfz
