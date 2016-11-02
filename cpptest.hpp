#include <cxxabi.h>
#include <stdio.h>
#include <stdlib.h>
#include <exception>


namespace cpptest {
struct AssertionFailure {
  const char *const format;
  const char *const file;
  const int line;
  const char *const expr;

  AssertionFailure(const char *format, const char *file, int line,
                   const char *expr)
      : format(format), file(file), line(line), expr(expr) {}
};

struct Section {
  Section *prev, *next;
  const char *const file;
  const int line;
  const char *const desc;
  bool done;
  bool entered;

  Section(const char *file, int line, const char *desc)
      : prev(NULL), next(NULL), file(file), line(line), desc(desc), done(false),
        entered(false) {}
};

struct State {
  bool done;
  Section *first, *current;

  State() : done(false), first(NULL), current(NULL) {}
};

struct TestCase {
  static TestCase *first, *last;
  TestCase *prev, *next;
  const char *file;
  const int line;
  const char *desc;
  void (*const f)(State &);

  TestCase(const char *file, int line, const char *desc, void (*f)(State &))
      : prev(last), next(NULL), file(file), line(line), desc(desc), f(f) {
    if (!first)
      first = this;
    if (last)
      last->next = this;
    last = this;
  }

  void print_exception_name() {
    int status = 0;
    char *buff = ::abi::__cxa_demangle(
        ::abi::__cxa_current_exception_type()->name(), NULL, NULL, &status);
    fprintf(stderr, "Caught exception of type '%s'\n", buff);
    free(buff);
  }

  void print_state(const State &state) const {
    fprintf(stderr, "Testcase state:\n%s%s:%d: %s\n",
            (state.current) ? "   " : "-> ", file, line, desc);
    int count = 1;
    for (Section *p = state.first; p; p = p->next)
      fprintf(stderr, "%*s%s:%d: %s\n", (3 + 3 * count++),
              (p == state.current) ? "-> " : "", p->file, p->line, p->desc);
  }

  bool run() {
    bool failed = false;

    while (true) {
      State state = State();

      try {
        f(state);
      } catch (const AssertionFailure &e) {
        failed = true;
        print_exception_name();
        fprintf(stderr, e.format, e.file, e.line, e.expr);
        print_state(state);
      } catch (const ::std::exception &e) {
        failed = true;
        print_exception_name();
        printf("  %s\n", e.what());
        print_state(state);
      } catch (...) {
        failed = true;
        print_exception_name();
        print_state(state);
      }

      if (!state.done)
        break;
    }

    return !failed;
  }
};

struct Cond {
  State &state;
  Section &section;

  Cond(State &state, Section &section) : state(state), section(section) {}

  ~Cond() {
    if (section.entered && !::std::uncaught_exception())
      state.current = section.prev;

    section.entered = false;

    if (section.done || state.done)
      return;

    state.done = true;
    section.done = true;
  }

  operator bool() {
    if (section.done || state.done)
      return false;

    section.prev = state.current;
    section.next = NULL;
    section.entered = true;

    if (state.current)
      state.current->next = &section;

    if (!state.first)
      state.first = &section;

    state.current = &section;

    return true;
  }
};

TestCase *TestCase::first = NULL;
TestCase *TestCase::last = NULL;
}

int main() {
  bool failed = false;

  for (::cpptest::TestCase *p = ::cpptest::TestCase::first; p; p = p->next)
    if (!(p->run()))
      failed = true;

  if (failed)
    return 1;

  return 0;
}

#define _CPPTEST_NAME(s, x) __cpptest_##s##x

#define _CPPTEST_TESTCASE(x, desc)                                             \
  static void _CPPTEST_NAME(func, x)(::cpptest::State &);                      \
  static ::cpptest::TestCase _CPPTEST_NAME(testcase, x)(                       \
      __FILE__, __LINE__, (desc), _CPPTEST_NAME(func, x));                     \
  void _CPPTEST_NAME(func, x)(::cpptest::State & _cpptest_state)

#define _CPPTEST_SECTION(x, desc)                                              \
  static ::cpptest::Section _CPPTEST_NAME(section, x)(__FILE__, __LINE__,      \
                                                      (desc));                 \
  if (auto __cpptest_cond =                                                    \
          ::cpptest::Cond(_cpptest_state, _CPPTEST_NAME(section, x)))

#define SECTION(desc) _CPPTEST_SECTION(__COUNTER__, (desc))
#define TESTCASE(desc) _CPPTEST_TESTCASE(__COUNTER__, (desc))

#define ASSERT(...)                                                            \
  {                                                                            \
    if (!(__VA_ARGS__))                                                        \
      throw ::cpptest::AssertionFailure("%s:%d: ASSERTION FAILED\n  %s\n",     \
                                        __FILE__, __LINE__, #__VA_ARGS__);     \
  }

#define ASSERT_THROW(type, ...)                                                \
  {                                                                            \
    bool __cpptest_throw = false;                                              \
    try {                                                                      \
      __VA_ARGS__;                                                             \
    } catch (const type &e) {                                                  \
      __cpptest_throw = true;                                                  \
    }                                                                          \
    if (!__cpptest_throw)                                                      \
      throw ::cpptest::AssertionFailure(                                       \
          "%s:%d: No exception caught in\n  %s\n", __FILE__, __LINE__,         \
          #__VA_ARGS__);                                                       \
  }
