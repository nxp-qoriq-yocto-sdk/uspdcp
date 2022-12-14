#ifndef _MOCKS_HEADER_
#define _MOCKS_HEADER_

#include <inttypes.h>
#include <stdarg.h>
#include "reporter.h"
#include "constraint.h"

#define mock(...) _mock(__func__, #__VA_ARGS__, (intptr_t)__VA_ARGS__ +0)
#define expect(f, ...) _expect(#f, __FILE__, __LINE__, (Constraint *)__VA_ARGS__ +0, (Constraint *)0)
#define always_expect(f, ...) _always_expect(#f, __FILE__, __LINE__, (Constraint *)__VA_ARGS__ +0, (Constraint *)0)
#define expect_never(f) _expect_never(#f, __FILE__, __LINE__)
#define expect_not(f) _expect_not(#f, __FILE__, __LINE__)
#define will_return(f, r) _will_return(#f, (intptr_t)r)
#define always_return(f, r) _always_return(#f, (intptr_t)r)
#define will_respond(f, r, ...) _will_return(#f, (intptr_t)r); _expect(#f, __FILE__, __LINE__, (Constraint *)__VA_ARGS__ +0, (Constraint *)0)
#define always_respond(f, r, ...) _always_return(#f, (intptr_t)r); _always_expect(#f, __FILE__, __LINE__, (Constraint *)__VA_ARGS__ +0, (Constraint *)0)

intptr_t _mock(const char *function, const char *parameters, ...);
void _expect(const char *function, const char *test_file, int test_line, ...);
void _always_expect(const char *function, const char *test_file, int test_line, ...);
void _expect_never(const char *function, const char *test_file, int test_line);
void _expect_not(const char *function, const char *test_file, int test_line);
void _will_return(const char *function, intptr_t result);
void _always_return(const char *function, intptr_t result);
void clear_mocks();
void tally_mocks(TestReporter *reporter);

#endif
