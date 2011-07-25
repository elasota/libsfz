// Copyright (c) 2011 Chris Pickel <sfiera@gmail.com>
//
// This file is part of libsfz, a free software project.  You can redistribute
// it and/or modify it under the terms of the MIT License.

#ifndef SFZ_ARGS_HPP_
#define SFZ_ARGS_HPP_

#include <map>
#include <vector>
#include <sfz/algorithm.hpp>
#include <sfz/macros.hpp>
#include <sfz/print.hpp>
#include <sfz/string.hpp>
#include <sfz/string-map.hpp>

namespace sfz {
namespace args {

class Action;
class Argument;

class Parser {
  public:
    Parser(PrintItem description);

    Argument& add_argument(PrintItem name, Action action);
    Argument& add_argument(PrintItem short_name, PrintItem long_name, Action action);

    void parse_args(const std::vector<StringSlice>& args) const;
    void parse_args(int argc, const char* const* argv) const;

    void usage(StringSlice program_name) const;

  private:
    friend class Argument;

    class State;

    const String _description;
    std::vector<linked_ptr<Argument> > _argument_specs;
    std::map<Rune, linked_ptr<Argument> > _short_options_by_name;
    StringMap<linked_ptr<Argument> > _long_options_by_name;

    DISALLOW_COPY_AND_ASSIGN(Parser);
};

template <typename To>
Action store(To& to);

template <typename To, typename Constant>
Action store_const(To& to, const Constant& constant);

template <typename To>
Action increment(To& to);

class Action {
  public:
    class Impl {
      public:
        virtual ~Impl() { }
        virtual bool takes_value() const = 0;
        virtual void process() const { }
        virtual void process(StringSlice value) const { }
    };

    Action(const linked_ptr<Impl>& impl);
    Action(const Action& other);
    Action& operator=(const Action& other);
    ~Action();

    bool takes_value() const;
    void process() const;
    void process(StringSlice value) const;

  private:
    linked_ptr<Impl> _impl;
};

class Argument {
  public:
    Argument& help(PrintItem s);

  private:
    friend class Parser;
    friend class Parser::State;

    Argument(Action action);

    Action _action;
    String _help;

    DISALLOW_COPY_AND_ASSIGN(Argument);
};

void store_argument(bool& to, StringSlice value);

void store_argument(int8_t& to, StringSlice value);
void store_argument(uint8_t& to, StringSlice value);
void store_argument(int16_t& to, StringSlice value);
void store_argument(uint16_t& to, StringSlice value);
void store_argument(int32_t& to, StringSlice value);
void store_argument(uint32_t& to, StringSlice value);
void store_argument(int64_t& to, StringSlice value);
void store_argument(uint64_t& to, StringSlice value);

template <typename To>
struct StoreAction : public Action::Impl {
    StoreAction(To& to): to(to) { }
    virtual bool takes_value() const { return true; }
    virtual void process(StringSlice value) const { store_argument(to, value); }
    To& to;
};

template <typename To>
Action store(To& to) {
    return linked_ptr<Action::Impl>(new StoreAction<To>(to));
}

template <typename To>
struct StoreConstAction : public Action::Impl {
    template <typename Constant>
    StoreConstAction(To& to, Constant constant): to(to), constant(constant) { }
    virtual bool takes_value() const { return false; }
    virtual void process() const { copy(to, constant); }
    To& to;
    To constant;
};

template <typename To, typename Constant>
Action store_const(To& to, const Constant& constant) {
    return linked_ptr<Action::Impl>(new StoreConstAction<To>(to, constant));
}

template <typename To>
struct IncrementAction : public Action::Impl {
    IncrementAction(To& arg): arg(arg) { }
    virtual bool takes_value() const { return false; }
    virtual void process() const { ++arg; }
    To& arg;
};

template <typename To>
Action increment(To& to) {
    return linked_ptr<Action::Impl>(new IncrementAction<To>(to));
}

}  // namespace args
}  // namespace sfz

#endif  // SFZ_ARGS_HPP_
