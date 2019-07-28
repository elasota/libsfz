// Copyright (c) 2010-2019 The libsfz Authors
//
// This file is part of libsfz, a free software project.  You can redistribute it and/or modify it
// under the terms of the MIT License.

#include <sfz/os.hpp>

#include <dirent.h>
#include <fcntl.h>
#include <fts.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include <pn/output>
#include <sfz/encoding.hpp>
#include <sfz/error.hpp>
#include <stdexcept>

namespace sfz {

namespace path {

const static pn::rune sep{'/'};

bool exists(pn::string_view path) {
    Stat st;
    return (stat(path.copy().c_str(), &st) == 0);
}

bool isdir(pn::string_view path) {
    Stat st;
    return (stat(path.copy().c_str(), &st) == 0) && ((st.st_mode & S_IFMT) == S_IFDIR);
}

bool isfile(pn::string_view path) {
    Stat st;
    return (stat(path.copy().c_str(), &st) == 0) && ((st.st_mode & S_IFMT) == S_IFREG);
}

bool islink(pn::string_view path) {
    Stat st;
    return (lstat(path.copy().c_str(), &st) == 0) && ((st.st_mode & S_IFMT) == S_IFLNK);
}

std::pair<pn::string_view, pn::string_view> splitdrive(pn::string_view path) {
    return {pn::string_view{}, path};
}

std::pair<pn::string_view, pn::string_view> split(pn::string_view path) {
    return {dirname(path), basename(path)};
}

pn::string_view basename(pn::string_view path) {
    if (path == sep) {
        return path;
    }
    int pos = path.rfind(sep);
    if (pos == path.npos) {
        return path;
    } else if (pos == path.size() - 1) {
        return basename(path.substr(0, path.size() - 1));
    } else {
        return path.substr(pos + 1);
    }
}

pn::string_view dirname(pn::string_view path) {
    int pos = path.rfind(sep);
    if (pos == 0) {
        return sep;
    } else if (pos == path.npos) {
        return ".";
    } else if (pos == path.size() - 1) {
        return dirname(path.substr(0, path.size() - 1));
    } else {
        return path.substr(0, pos);
    }
}

pn::string joinv(pn::string_view root, std::initializer_list<pn::string_view> segments) {
    auto from_segment = segments.end();
    for (auto it = segments.begin(); it != segments.end(); ++it) {
        if (it->empty()) {
            continue;
        } else if (*it->begin() == sep) {
            root         = *it;
            from_segment = it;
        }
    }
    pn::string result = root.copy();
    if (from_segment == segments.end()) {
        from_segment = segments.begin();
    } else {
        ++from_segment;
    }
    for (auto it = from_segment; it != segments.end(); ++it) {
        if (!result.empty()) {
            auto jt = result.end();
            if (*--jt != sep) {
                result += sep;
            }
        }
        result += *it;
    }
    return result;
}

}  // namespace path

void chdir(pn::string_view path) {
    if (::chdir(path.copy().c_str()) < 0) {
        throw std::runtime_error(pn::format("chdir: {0}: {1}", path, posix_strerror()).c_str());
    }
}

pn::string getcwd() {
    char path[PATH_MAX];
    return pn::string{::getcwd(path, PATH_MAX)};
}

void symlink(pn::string_view content, pn::string_view container) {
    if (::symlink(content.copy().data(), container.copy().data()) < 0) {
        throw std::runtime_error(
                pn::format("symlink: {0}: {1}", container, posix_strerror()).c_str());
    }
}

void mkdir(pn::string_view path, mode_t mode) {
    if (::mkdir(path.copy().c_str(), mode) != 0) {
        throw std::runtime_error(pn::format("mkdir: {0}: {1}", path, posix_strerror()).c_str());
    }
}

void mkfifo(pn::string_view path, mode_t mode) {
    if (::mkfifo(path.copy().c_str(), mode) != 0) {
        throw std::runtime_error(pn::format("mkfifo: {0}: {1}", path, posix_strerror()).c_str());
    }
}

void makedirs(pn::string_view path, mode_t mode) {
    if (!path::isdir(path)) {
        makedirs(path::dirname(path), mode);
        mkdir(path, mode);
    }
}

void unlink(pn::string_view path) {
    if (::unlink(path.copy().c_str()) < 0) {
        throw std::runtime_error(pn::format("unlink: {0}: {1}", path, posix_strerror()).c_str());
    }
}

void rmdir(pn::string_view path) {
    if (::rmdir(path.copy().c_str()) < 0) {
        throw std::runtime_error(pn::format("rmdir: {0}: {1}", path, posix_strerror()).c_str());
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
    pn::string path = pn::format("/tmp/{0}XXXXXX", prefix);
    if (!mkdtemp(path.data())) {
        throw std::runtime_error("mkdtemp() failed");
    }
    _path = std::move(path);
}

TemporaryDirectory::~TemporaryDirectory() { rmtree(_path); }

const pn::string& TemporaryDirectory::path() const { return _path; }

static void scandir_end(void* ptr) {
    if (ptr) {
        DIR* dir = reinterpret_cast<DIR*>(ptr);
        closedir(dir);
    }
}

scandir_container::iterator::iterator() : _state{nullptr, scandir_end} {}

scandir_container::iterator::iterator(pn::string dir)
        : _dir{std::move(dir)}, _state{opendir(_dir.c_str()), scandir_end} {
    if (!_state.get()) {
        throw std::runtime_error(pn::format("scandir: {0}: {1}", _dir, posix_strerror()).c_str());
    }
    ++*this;
}

scandir_container::iterator::iterator(iterator&&) = default;

scandir_container::iterator::~iterator() = default;

scandir_container::iterator& scandir_container::iterator::operator++() {
    dirent* ent;
    if (!(ent = readdir(reinterpret_cast<DIR*>(_state.get())))) {
        _state.reset();
        return *this;
    }
    pn::string_view name{ent->d_name};
    if ((name == ".") || (name == "..")) {
        return ++*this;
    }
    _entry.name = name.copy();
    if (stat(path::join(_dir, name).c_str(), &_entry.st) != 0) {
        throw std::runtime_error(pn::format("scandir: {0}: {1}", name, posix_strerror()).c_str());
    }
    return *this;
}

namespace {

class FtsCloser {
  public:
    FtsCloser(FTS* fts) : _fts(fts) {}
    FtsCloser(const FtsCloser&) = delete;
    FtsCloser(FtsCloser&&)      = delete;
    ~FtsCloser() { fts_close(_fts); }

  private:
    FTS* _fts;
};

int compare_ftsent(const FTSENT** lhs, const FTSENT** rhs) {
    return strcmp((*lhs)->fts_name, (*rhs)->fts_name);
}

}  // namespace

void walk(pn::string_view root, WalkType type, const TreeWalker& visitor) {
    pn::string  root_copy = root.copy();
    char* const pathv[]   = {root_copy.data(), NULL};
    int         options   = FTS_NOCHDIR;
    if (type == WALK_PHYSICAL) {
        options |= FTS_PHYSICAL;
    } else {
        options |= FTS_LOGICAL;
    }

    FTS* fts = fts_open(pathv, options, compare_ftsent);
    if (fts == NULL) {
        throw std::runtime_error(pn::format("fts_open: {0}: {1}", root, posix_strerror()).c_str());
    }
    FtsCloser deleter(fts);
    while (FTSENT* ent = fts_read(fts)) {
        const Stat& st = *ent->fts_statp;
        switch (ent->fts_info) {
            case FTS_D: visitor.pre_directory(ent->fts_path, st); break;
            case FTS_DC: visitor.cycle_directory(ent->fts_path, st); break;
            case FTS_DEFAULT: visitor.other(ent->fts_path, st); break;
            case FTS_DP: visitor.post_directory(ent->fts_path, st); break;
            case FTS_F: visitor.file(ent->fts_path, st); break;
            case FTS_SL: visitor.symlink(ent->fts_path, st); break;
            case FTS_SLNONE: visitor.broken_symlink(ent->fts_path, st); break;

            case FTS_DNR:
            case FTS_ERR:
            case FTS_NS:
                throw std::runtime_error(
                        pn::format("fts_read: {0}: {1}", ent->fts_path, posix_strerror()).c_str());

            case FTS_DOT:
            case FTS_NSOK:
            default:
                throw std::runtime_error(
                        pn::format("walk: got invalid type {0}", ent->fts_info).c_str());
        }
    }
}

TreeWalker::~TreeWalker() {}

}  // namespace sfz
