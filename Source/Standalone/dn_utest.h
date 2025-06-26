#if !defined(DN_UT_H)
#define DN_UT_H

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   $$\   $$\ $$$$$$$$\ $$$$$$$$\  $$$$$$\ $$$$$$$$\
//   $$ |  $$ |\__$$  __|$$  _____|$$  __$$\\__$$  __|
//   $$ |  $$ |   $$ |   $$ |      $$ /  \__|  $$ |
//   $$ |  $$ |   $$ |   $$$$$\    \$$$$$$\    $$ |
//   $$ |  $$ |   $$ |   $$  __|    \____$$\   $$ |
//   $$ |  $$ |   $$ |   $$ |      $$\   $$ |  $$ |
//   \$$$$$$  |   $$ |   $$$$$$$$\ \$$$$$$  |  $$ |
//    \______/    \__|   \________| \______/   \__|
//
//   dn_utest.h -- Extremely minimal unit testing framework
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// A super minimal testing framework, most of the logic here is the pretty
// printing of test results.
//
// NOTE: Configuration /////////////////////////////////////////////////////////////////////////////
//
// #define DN_UT_IMPLEMENTATION
//     Define this in one and only one C++ file to enable the implementation
//     code of the header file. This will also automatically enable the JSMN
//     implementation.
//
// #define DN_UT_RESULT_LPAD
//     Define this to a number to specify how much to pad the output of the test
//     result line before the test result is printed.
//
// #define DN_UT_RESULT_PAD_CHAR
//     Define this to a character to specify the default character to use for
//     padding. By default this is '.'
//
// #define DN_UT_SPACING
//     Define this to a number to specify the number of spaces between the group
//     declaration and the test output in the group.
//
// #define DN_UT_BAD_COLOR
//     Define this to a terminal color code to specify what color errors will be
//     presented as.
//
// #define DN_UT_GOOD_COLOR
//     Define this to a terminal color code to specify what color sucess will be
//     presented as.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: Macros ////////////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if !defined(DN_UT_RESULT_LPAD)
  #define DN_UT_RESULT_LPAD 90
#endif

#if !defined(DN_UT_RESULT_PAD_CHAR)
  #define DN_UT_RESULT_PAD_CHAR '.'
#endif

#if !defined(DN_UT_SPACING)
  #define DN_UT_SPACING 2
#endif

#if !defined(DN_UT_BAD_COLOR)
  #define DN_UT_BAD_COLOR "\x1b[31m"
#endif

#if !defined(DN_UT_GOOD_COLOR)
  #define DN_UT_GOOD_COLOR "\x1b[32m"
#endif

#define DN_UT_COLOR_RESET "\x1b[0m"

#define DN_UT_Test(test, fmt, ...)                            \
  int dummy_ = (DN_UT_BeginF((test), fmt, ##__VA_ARGS__), 0); \
  (void)dummy_, (test)->state == DN_UTState_TestBegun;        \
  DN_UT_End(test)

#define DN_UT_AssertF(test, expr, fmt, ...) \
  DN_UT_AssertAtF((test), __FILE__, __LINE__, (expr), fmt, ##__VA_ARGS__)

#define DN_UT_Assert(test, expr) \
  DN_UT_AssertAt((test), __FILE__, __LINE__, (expr))

// TODO: Fix the logs. They print before the tests, we should accumulate logs
// per test, then, dump them on test on. But to do this nicely without crappy
// mem management we need to implement an arena.
#define DN_UT_Log(test, fmt, ...) \
  DN_UT_LogF(test, "%*s" fmt "\n", DN_UT_SPACING * 2, "", ##__VA_ARGS__)

#define DN_UT_AssertAtF(test, file, line, expr, fmt, ...) \
  do {                                                    \
    if (!(expr)) {                                        \
      (test)->state = DN_UTState_TestFailed;              \
      DN_UT_LogInsideTestF(test,                          \
                           "%*sAssertion File: %s:%d\n"   \
                           "%*sExpression: [" #expr       \
                           "]\n"                          \
                           "%*sReason: " fmt "\n",        \
                           DN_UT_SPACING * 2,             \
                           "",                            \
                           file,                          \
                           line,                          \
                           DN_UT_SPACING * 2,             \
                           "",                            \
                           DN_UT_SPACING * 2,             \
                           "",                            \
                           ##__VA_ARGS__);                \
    }                                                     \
  } while (0)

#define DN_UT_AssertAt(test, file, line, expr)               \
  do {                                                       \
    if (!(expr)) {                                           \
      (test)->state = DN_UTState_TestFailed;                 \
      DN_UT_LogInsideTestF(test,                             \
                           "%*sAssertion File: %s:%d\n"      \
                           "%*sExpression: [" #expr "]\n",   \
                           DN_UT_SPACING * 2,                \
                           "",                               \
                           file,                             \
                           line,                             \
                           DN_UT_SPACING * 2,                \
                           "");                              \
    }                                                        \
  } while (0)

// NOTE: Header ////////////////////////////////////////////////////////////////////////////////////
typedef enum DN_UTState
{
  DN_UTState_Nil,
  DN_UTState_TestBegun,
  DN_UTState_TestFailed,
} DN_UTState;

typedef struct DN_UTStr8Link
{
  char          *data;
  size_t         size;
  DN_UTStr8Link *next;
  DN_UTStr8Link *prev;
} DN_UTStr8Link;

typedef struct DN_UTCore
{
  int            num_tests_in_group;
  int            num_tests_ok_in_group;
  DN_UTState     state;
  char           name[256];
  size_t         name_size;
  DN_UTStr8Link *curr_test_messages;
  DN_UTStr8Link *output;
} DN_UTCore;

void DN_UT_BeginFV(DN_UTCore *test, char const *fmt, va_list args);
void DN_UT_BeginF(DN_UTCore *test, char const *fmt, ...);
void DN_UT_End(DN_UTCore *test);

void DN_UT_LogF(DN_UTCore *test, char const *fmt, ...);
void DN_UT_LogInsideTestF(DN_UTCore *test, char const *fmt, ...);

bool DN_UT_AllTestsPassed(DN_UTCore const *test);
void DN_UT_PrintTests(DN_UTCore const *test);
#endif // DN_UT_H

// NOTE: Implementation ////////////////////////////////////////////////////////////////////////////
#if defined(DN_UT_IMPLEMENTATION)
DN_UTCore DN_UT_Init()
{
  DN_UTCore result                = {};
  result.output                   = (DN_UTStr8Link *)calloc(1, sizeof(*result.output));
  result.curr_test_messages       = (DN_UTStr8Link *)calloc(1, sizeof(*result.curr_test_messages));
  assert(result.output);
  assert(result.curr_test_messages);
  result.output->next             = result.output->prev = result.output;
  result.curr_test_messages->next = result.curr_test_messages->prev = result.curr_test_messages;
  return result;
}

void DN_UT_Deinit(DN_UTCore *ut)
{
  for (DN_UTStr8Link *it = ut->output->next; it != ut->output; it = ut->output->next) {
    it->next->prev = it->prev;
    it->prev->next = it->next;
    free(it);
  }
  free(ut->output);

  for (DN_UTStr8Link *it = ut->curr_test_messages->next; it != ut->curr_test_messages; it = ut->curr_test_messages->next) {
    it->next->prev = it->prev;
    it->prev->next = it->next;
    free(it);
  }
  free(ut->curr_test_messages);
}

void DN_UT_BeginFV(DN_UTCore *ut, char const *fmt, va_list args)
{
  assert(ut->output && ut->curr_test_messages && "Test must be initialised by calling DN_UT_Init()");
  assert(ut->state == DN_UTState_Nil &&
         "Nesting a unit ut within another unit test is not allowed, ensure"
         "the first test has finished by calling DN_UT_End");

  ut->num_tests_in_group++;
  ut->state = DN_UTState_TestBegun;
  ut->name_size = 0;
  {
    va_list args_copy;
    va_copy(args_copy, args);
    ut->name_size = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
  }

  assert(ut->name_size < sizeof(ut->name));
  vsnprintf(ut->name, sizeof(ut->name), fmt, args);
}

void DN_UT_BeginF(DN_UTCore *ut, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_UT_BeginFV(ut, fmt, args);
  va_end(args);
}

static DN_UTStr8Link *DN_UT_AllocStr8LinkFV(char const *fmt, va_list args)
{
  va_list args_copy;
  va_copy(args_copy, args);
  size_t size = vsnprintf(nullptr, 0, fmt, args_copy) + 1;
  va_end(args_copy);

  DN_UTStr8Link *result = (DN_UTStr8Link *)malloc(sizeof(*result) + size);
  if (result) {
    result->data = (char *)result + sizeof(*result);
    result->size = vsnprintf(result->data, size, fmt, args);
  }
  return result;
}

void DN_UT_End(DN_UTCore *ut)
{
  assert(ut->state != DN_UTState_Nil && "Test was marked as ended but a ut was never commenced using DN_UT_Begin");
  size_t pad_size = DN_UT_RESULT_LPAD - (DN_UT_SPACING + ut->name_size);
  if (pad_size < 0)
    pad_size = 0;

  char pad_buffer[DN_UT_RESULT_LPAD] = {};
  memset(pad_buffer, DN_UT_RESULT_PAD_CHAR, pad_size);

  DN_UT_LogF(ut, "%*s%.*s%.*s", DN_UT_SPACING, "", (int)ut->name_size, ut->name, (int)pad_size, pad_buffer);
  if (ut->state == DN_UTState_TestFailed) {
    DN_UT_LogF(ut, DN_UT_BAD_COLOR " FAILED");
  } else {
    DN_UT_LogF(ut, DN_UT_GOOD_COLOR " OK");
    ut->num_tests_ok_in_group++;
  }
  DN_UT_LogF(ut, DN_UT_COLOR_RESET "\n");
  ut->state = DN_UTState_Nil;

  // NOTE: Append any test messages (like assertions) into the main output buffer
  for (DN_UTStr8Link *it = ut->curr_test_messages->next; it != ut->curr_test_messages; it = ut->curr_test_messages->next) {
    // NOTE: Detach
    it->next->prev = it->prev;
    it->prev->next = it->next;

    // NOTE: Attach
    it->next       = ut->output;
    it->prev       = ut->output->prev;
    it->next->prev = it;
    it->prev->next = it;
  }
}

void DN_UT_LogF(DN_UTCore *ut, char const *fmt, ...)
{
  assert(ut->output && ut->curr_test_messages && "UT was not initialised by calling UT_Init yet");
  va_list args;
  va_start(args, fmt);
  DN_UTStr8Link *result = DN_UT_AllocStr8LinkFV(fmt, args);
  va_end(args);

  result->next       = ut->output;
  result->prev       = ut->output->prev;
  result->next->prev = result;
  result->prev->next = result;
}

void DN_UT_LogInsideTestF(DN_UTCore *ut, char const *fmt, ...)
{
  assert(ut->state >= DN_UTState_TestBegun && "");
  va_list args;
  va_start(args, fmt);
  DN_UTStr8Link *result = DN_UT_AllocStr8LinkFV(fmt, args);
  va_end(args);

  result->next       = ut->curr_test_messages;
  result->prev       = ut->curr_test_messages->prev;
  result->next->prev = result;
  result->prev->next = result;
}

bool DN_UT_AllTestsPassed(DN_UTCore const *ut)
{
  bool result = ut->num_tests_ok_in_group == ut->num_tests_in_group;
  return result;
}

void DN_UT_PrintTests(DN_UTCore const *ut)
{
  for (DN_UTStr8Link *it = ut->output->next; it != ut->output; it = it->next)
    fprintf(stdout, "%.*s", (int)it->size, it->data);

  bool all_clear = DN_UT_AllTestsPassed(ut);
  fprintf(stdout,
          "%s\n  %02d/%02d tests passed -- %s\n\n" DN_UT_COLOR_RESET,
          all_clear ? DN_UT_GOOD_COLOR : DN_UT_BAD_COLOR,
          ut->num_tests_ok_in_group,
          ut->num_tests_in_group,
          all_clear ? "OK" : "FAILED");
}
#endif // DN_UT_IMPLEMENTATION
