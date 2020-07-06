// Copyright (c) 2019 The libsfz Authors
//
// This file is part of libsfz, a free software project.  You can redistribute it and/or modify it
// under the terms of the MIT License.

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pn/output>
#include <sfz/encoding.hpp>
#include <sfz/optional.hpp>
#include <sfz/os.hpp>
#include <stdexcept>

#include "error.hpp"
#include "win.hpp"

namespace sfz {

namespace {

struct handle {
    HANDLE h = nullptr;
    handle(HANDLE handle) : h{handle} {}
    ~handle() {
        if (h) {
            CloseHandle(h);
        }
    }
};

}  // namespace

namespace path {

const static pn::rune sep{'\\'};

static bool is_sep(const pn::rune& r) {
    switch (r.value()) {
        case '/':
        case '\\': return true;
        default: return false;
    }
}

bool exists(pn::string_view path) {
    return GetFileAttributesA(path.copy().c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool isdir(pn::string_view path) {
    DWORD attrs;
    return ((attrs = GetFileAttributesA(path.copy().c_str())) != INVALID_FILE_ATTRIBUTES) &&
           (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool isfile(pn::string_view path) {
    DWORD attrs;
    return ((attrs = GetFileAttributesA(path.copy().c_str())) != INVALID_FILE_ATTRIBUTES) &&
           !(attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
}

bool islink(pn::string_view path) {
    WIN32_FIND_DATAA file_data;
    return (handle{FindFirstFile(path.copy().c_str(), &file_data)}.h != INVALID_HANDLE_VALUE) &&
           (file_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
           (file_data.dwReserved0 == IO_REPARSE_TAG_SYMLINK);
}

std::pair<pn::string_view, pn::string_view> splitdrive(pn::string_view path) {
    auto it = path.begin();
    if ((it == path.end()) || (++it == path.end())) {
        return {"", path};
    }
    pn::rune second = *(it++);

    pn::string_view first_two = path.substr(0, it.offset());
    pn::string_view remainder = path.substr(it.offset());
    if (second == pn::rune{':'}) {
        // "X:\path\to\file"
        return {first_two, remainder};
    }
    if (first_two == "\\\\") {
        // "\\machine\mountpoint\path\to\file
        pn::string_view machine;
        if (partition(&machine, "\\", &remainder) && !machine.empty()) {
            pn::string_view mountpoint;
            if (partition(&mountpoint, "\\", &remainder)) {
                if (!mountpoint.empty()) {
                    int drive_len = path.size() - remainder.size() - 1;
                    return {path.substr(0, drive_len), path.substr(drive_len)};
                }
            } else if (!mountpoint.empty()) {
                return {path, ""};
            }
        }
    }
    return {"", path};
}

std::pair<pn::string_view, pn::string_view> split(pn::string_view path) {
    if (path.empty()) {
        return {".", ""};
    }

    auto            drive_local = splitdrive(path);
    pn::string_view drive       = drive_local.first;
    pn::string_view local       = drive_local.second;
    if (local.empty()) {
        return {drive, local};
    }

    sfz::optional<pn::string_view::size_type> basename_end;
    sfz::optional<pn::string_view::size_type> basename_start;
    sfz::optional<pn::string_view::size_type> dirname_end;
    for (auto it = local.rbegin(); it != local.rend(); ++it) {
        if (is_sep(*it)) {
            if (!basename_end.has_value()) {
                // keep looping for end of basename.
            } else if (!basename_start.has_value()) {
                basename_start.emplace(it.offset() + drive.size());
            }
        } else {
            if (!basename_end.has_value()) {
                basename_end.emplace(it.offset() + drive.size());
            } else if (!basename_start.has_value()) {
                // keep looping for start of basename.
            } else if (!dirname_end.has_value()) {
                dirname_end.emplace(it.offset() + drive.size());
                break;
            }
        }
    }

    if (!basename_end.has_value()) {
        return {path.substr(0, drive.size() + 1), path.substr(drive.size(), 1)};
    }
    if (!basename_start.has_value()) {
        basename_start.emplace(drive.size());
    }
    if (!dirname_end.has_value()) {
        dirname_end.emplace(*basename_start);
    }

    if (!*basename_start) {
        return {".", path.substr(*basename_start, *basename_end - *basename_start)};
    } else if (!*dirname_end) {
        return {path.substr(0, 1), path.substr(*basename_start, *basename_end - *basename_start)};
    } else {
        return {path.substr(0, *dirname_end),
                path.substr(*basename_start, *basename_end - *basename_start)};
    }
}

pn::string_view dirname(pn::string_view path) { return split(path).first; }
pn::string_view basename(pn::string_view path) { return split(path).second; }

pn::string joinv(pn::string_view root, std::initializer_list<pn::string_view> segments) {
    auto drive_local  = splitdrive(root);
    auto drive        = drive_local.first;
    auto local        = drive_local.second;
    auto from_segment = segments.end();
    for (auto it = segments.begin(); it != segments.end(); ++it) {
        if (it->empty()) {
            continue;
        }
        auto drive_local = splitdrive(*it);
        if (!drive_local.first.empty()) {
            drive        = drive_local.first;
            local        = drive_local.second;
            from_segment = it;
        } else if (is_sep(*it->begin())) {
            local        = *it;
            from_segment = it;
        }
    }
    pn::string result = pn::format("{}{}", drive, local);
    if (from_segment == segments.end()) {
        from_segment = segments.begin();
    } else {
        ++from_segment;
    }
    for (auto it = from_segment; it != segments.end(); ++it) {
        if (!result.empty()) {
            auto jt = result.end();
            if (!(splitdrive(result).second.empty() || is_sep(*--jt))) {
                result += sep;
            }
        }
        result += *it;
    }
    return result;
}

}  // namespace path

void chdir(pn::string_view path) {
    if (!SetCurrentDirectory(path.copy().c_str())) {
        throw std::runtime_error(pn::format("chdir: {0}: {1}", path, posix_strerror()).c_str());
    }
}

pn::string getcwd() {
    char path[PATH_MAX];
    return pn::string{::getcwd(path, PATH_MAX)};
}

void symlink(pn::string_view content, pn::string_view container) {
    int flags = path::isdir(content) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0x0;
    if (!CreateSymbolicLinkA(container.copy().c_str(), content.copy().c_str(), flags)) {
        throw std::runtime_error(
                pn::format("symlink: {0}: {1}", container, posix_strerror()).c_str());
    }
}

void mkdir(pn::string_view path, mode_t mode) {
    static_cast<void>(mode);
    if (!CreateDirectoryA(path.copy().c_str(), nullptr)) {
        throw std::runtime_error(pn::format("mkdir: {0}: {1}", path, posix_strerror()).c_str());
    }
}

void mkfifo(pn::string_view path, mode_t mode) {
    static_cast<void>(mode);
    throw std::runtime_error(pn::format("mkfifo: {0}: not supported", path).c_str());
}

void makedirs(pn::string_view path, mode_t mode) {
    if (!path::isdir(path)) {
        makedirs(path::dirname(path), mode);
        mkdir(path, mode);
    }
}

void unlink(pn::string_view path) {
    if (!DeleteFileA(path.copy().c_str())) {
        throw std::runtime_error(pn::format("unlink: {0}: {1}", path, win_strerror()).c_str());
    }
}

void rmdir(pn::string_view path) {
    if (!RemoveDirectoryA(path.copy().c_str())) {
        throw std::runtime_error(pn::format("rmdir: {0}: {1}", path, win_strerror()).c_str());
    }
}

void rmtree(pn::string_view path) {
    if (path::exists(path)) {
        class RmtreeVisitor : public TreeWalker {
          public:
            void pre_directory(pn::string_view path, const Stat& stat) const {
                static_cast<void>(path);
                static_cast<void>(stat);
            }
            void cycle_directory(pn::string_view path, const Stat& stat) const {
                static_cast<void>(path);
                static_cast<void>(stat);
            }

            void post_directory(pn::string_view path, const Stat&) const { rmdir(path); }

            void file(pn::string_view path, const Stat&) const { unlink(path); }
            void symlink(pn::string_view path, const Stat&) const { unlink(path); }
            void broken_symlink(pn::string_view path, const Stat&) const { unlink(path); }
            void other(pn::string_view path, const Stat&) const { unlink(path); }
        };
        RmtreeVisitor visitor;
        walk(path, WALK_PHYSICAL, visitor);
    }
}

TemporaryDirectory::TemporaryDirectory(pn::string_view prefix) {
    char temp_path_buf[MAX_PATH + 1];
    if (GetTempPathA(MAX_PATH + 1, temp_path_buf) == 0) {
        throw std::runtime_error(pn::format("GetTempPath: {0}", win_strerror()).c_str());
    }
    char temp_file_buf[MAX_PATH + 1];
    if (GetTempFileNameA(
                temp_path_buf, pn::format("{}XXXXXX", prefix).c_str(), 0, temp_file_buf) == 0) {
        throw std::runtime_error(pn::format("GetTempFileName: {0}", win_strerror()).c_str());
    }
    pn::string temp_file = temp_file_buf;
    unlink(temp_file);
    mkdir(temp_file, 0000);
    _path = std::move(temp_file);
}

TemporaryDirectory::~TemporaryDirectory() { rmtree(_path); }

const pn::string& TemporaryDirectory::path() const { return _path; }

static void scandir_end(void* ptr) {
    if (ptr) {
        HANDLE dir = reinterpret_cast<HANDLE>(ptr);
        CloseHandle(dir);
    }
}

scandir_container::iterator::iterator() : _state{nullptr, scandir_end} {}

scandir_container::iterator::iterator(pn::string dir)
        : _dir{std::move(dir)}, _state{nullptr, scandir_end} {
    WIN32_FIND_DATAA file_data;
    _state.reset(FindFirstFileA(pn::format("{}\\*", _dir).c_str(), &file_data));
    if (_state.get() == INVALID_HANDLE_VALUE) {
        throw std::runtime_error(pn::format("scandir: {0}: {1}", _dir, win_strerror()).c_str());
    }
    pn::string_view name{file_data.cFileName};
    if ((name == ".") || (name == "..")) {
        ++*this;
        return;
    }
    _entry.name = name.copy();
    if (stat(path::join(_dir, name).c_str(), &_entry.st) != 0) {
        throw std::runtime_error(pn::format("scandir: {0}: {1}", name, posix_strerror()).c_str());
    }
}

scandir_container::iterator::iterator(iterator&&) = default;

scandir_container::iterator::~iterator() = default;

scandir_container::iterator& scandir_container::iterator::operator++() {
    WIN32_FIND_DATAA file_data;
    if (!FindNextFileA(reinterpret_cast<HANDLE>(_state.get()), &file_data)) {
        if (win_last_error() == ERROR_NO_MORE_FILES) {
            _state.reset();
            return *this;
        } else {
            throw std::runtime_error(
                    pn::format("scandir: {0}: {1}", _dir, win_strerror()).c_str());
        }
    }
    pn::string_view name = file_data.cFileName;
    if ((name == ".") || (name == "..")) {
        return ++*this;
    }
    _entry.name = name.copy();
    if (stat(path::join(_dir, name).c_str(), &_entry.st) != 0) {
        throw std::runtime_error(pn::format("scandir: {0}: {1}", name, posix_strerror()).c_str());
    }
    return *this;
}

static void visit_file(
        pn::string_view path, const WIN32_FIND_DATAA* file_data, const TreeWalker& visitor);

static void walk_dir(pn::string_view root, const TreeWalker& visitor) {
    WIN32_FIND_DATAA file_data;
    handle           list{FindFirstFileA(pn::format("{}\\*", root).c_str(), &file_data)};
    if (!list.h) {
        throw std::runtime_error(pn::format("walk: {0}: {1}", root, win_strerror()).c_str());
    }
    while (FindNextFileA(list.h, &file_data)) {
        pn::string_view file = file_data.cFileName;
        if ((file == ".") || (file == "..")) {
            continue;
        }
        visit_file(pn::format("{0}\\{1}", root, file_data.cFileName), &file_data, visitor);
    }
    if (win_last_error() != ERROR_NO_MORE_FILES) {
        throw std::runtime_error(pn::format("walk: {0}: {1}", root, win_strerror()).c_str());
    }
}

static void visit_file(
        pn::string_view path, const WIN32_FIND_DATAA* file_data, const TreeWalker& visitor) {
    Stat st;
    if (stat(path.copy().c_str(), &st) < 0) {
        throw std::runtime_error(pn::format("stat: {0}: {1}", path, posix_strerror()).c_str());
    }

    if (file_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        visitor.pre_directory(path, st);
        walk_dir(path, visitor);
        visitor.post_directory(path, st);
    } else if (!(file_data->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
        visitor.file(path, st);
    } else if (file_data->dwReserved0 == IO_REPARSE_TAG_SYMLINK) {
        visitor.symlink(path, st);
    } else {
        visitor.other(path, st);
    }
}

void walk(pn::string_view root, WalkType type, const TreeWalker& visitor) {
    static_cast<void>(type);
    WIN32_FIND_DATAA file_data;
    handle           list{FindFirstFileA(root.copy().c_str(), &file_data)};
    if (list.h == INVALID_HANDLE_VALUE) {
        throw std::runtime_error(pn::format("walk: {}: {}", root, win_strerror()).c_str());
    }
    visit_file(root, &file_data, visitor);
}

TreeWalker::~TreeWalker() {}

}  // namespace sfz
