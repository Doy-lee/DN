#if !defined(DN_UTEST_H)
#define DN_UTEST_H

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
// #define DN_UTEST_IMPLEMENTATION
//     Define this in one and only one C++ file to enable the implementation
//     code of the header file. This will also automatically enable the JSMN
//     implementation.
//
// #define DN_UTEST_RESULT_LPAD
//     Define this to a number to specify how much to pad the output of the test
//     result line before the test result is printed.
//
// #define DN_UTEST_RESULT_PAD_CHAR
//     Define this to a character to specify the default character to use for
//     padding. By default this is '.'
//
// #define DN_UTEST_SPACING
//     Define this to a number to specify the number of spaces between the group
//     declaration and the test output in the group.
//
// #define DN_UTEST_BAD_COLOR
//     Define this to a terminal color code to specify what color errors will be
//     presented as.
//
// #define DN_UTEST_GOOD_COLOR
//     Define this to a terminal color code to specify what color sucess will be
//     presented as.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: Macros ////////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#if !defined(DN_UTEST_RESULT_LPAD)
    #define DN_UTEST_RESULT_LPAD 90
#endif

#if !defined(DN_UTEST_RESULT_PAD_CHAR)
    #define DN_UTEST_RESULT_PAD_CHAR '.'
#endif

#if !defined(DN_UTEST_SPACING)
    #define DN_UTEST_SPACING 2
#endif

#if !defined(DN_UTEST_BAD_COLOR)
    #define DN_UTEST_BAD_COLOR "\x1b[31m"
#endif

#if !defined(DN_UTEST_GOOD_COLOR)
    #define DN_UTEST_GOOD_COLOR "\x1b[32m"
#endif

#define DN_UTEST_COLOR_RESET "\x1b[0m"

#define DN_UTEST_GROUP(test, fmt, ...) \
    for (DN_UTest *test_var_ = (printf(fmt "\n", ## __VA_ARGS__), &test); \
         test_var_ != nullptr; \
         DN_UTest_PrintStats(&test), test_var_ = nullptr)

#define DN_UTEST_TEST(fmt, ...) \
    for (int dummy_ = (DN_UTest_Begin(test_var_, fmt, ## __VA_ARGS__), 0); \
         (void)dummy_, test_var_->state == DN_UTestState_TestBegun; \
         DN_UTest_End(test_var_))

#define DN_UTEST_ASSERTF(test, expr, fmt, ...) \
    DN_UTEST_ASSERTF_AT((test), __FILE__, __LINE__, (expr), fmt, ##__VA_ARGS__)

#define DN_UTEST_ASSERT(test, expr) \
    DN_UTEST_ASSERT_AT((test), __FILE__, __LINE__, (expr))

// TODO: Fix the logs. They print before the tests, we should accumulate logs
// per test, then, dump them on test on. But to do this nicely without crappy 
// mem management we need to implement an arena.
#define DN_UTEST_LOG(fmt, ...) \
    fprintf(stdout, "%*s" fmt "\n", DN_UTEST_SPACING * 2, "", ##__VA_ARGS__)

#define DN_UTEST_ASSERTF_AT(test, file, line, expr, fmt, ...) \
    do {                                                       \
        if (!(expr)) {                                         \
            (test)->state = DN_UTestState_TestFailed;         \
            fprintf(stderr,                                    \
                    "%*sAssertion Triggered\n"                 \
                    "%*sFile: %s:%d\n"                         \
                    "%*sExpression: [" #expr "]\n"             \
                    "%*sReason: " fmt "\n\n",                  \
                    DN_UTEST_SPACING * 2,                     \
                    "",                                        \
                    DN_UTEST_SPACING * 3,                     \
                    "",                                        \
                    file,                                      \
                    line,                                      \
                    DN_UTEST_SPACING * 3,                     \
                    "",                                        \
                    DN_UTEST_SPACING * 3,                     \
                    "",                                        \
                    ##__VA_ARGS__);                            \
        }                                                      \
    } while (0)

#define DN_UTEST_ASSERT_AT(test, file, line, expr)    \
    do {                                               \
        if (!(expr)) {                                 \
            (test)->state = DN_UTestState_TestFailed; \
            fprintf(stderr,                            \
                    "%*sFile: %s:%d\n"                 \
                    "%*sExpression: [" #expr "]\n\n",  \
                    DN_UTEST_SPACING * 2,             \
                    "",                                \
                    file,                              \
                    line,                              \
                    DN_UTEST_SPACING * 2,             \
                    "");                               \
        }                                              \
    } while (0)

// NOTE: Header ////////////////////////////////////////////////////////////////////////////////////
typedef enum DN_UTestState {
    DN_UTestState_Nil,
    DN_UTestState_TestBegun,
    DN_UTestState_TestFailed,
} DN_UTestState;

typedef struct DN_UTest {
    int             num_tests_in_group;
    int             num_tests_ok_in_group;
    DN_UTestState  state;
    bool            finished;
    char            name[256];
    size_t          name_size;
} DN_UTest;

void DN_UTest_PrintStats(DN_UTest *test);
void DN_UTest_BeginV(DN_UTest *test, char const *fmt, va_list args);
void DN_UTest_Begin(DN_UTest *test, char const *fmt, ...);
void DN_UTest_End(DN_UTest *test);
#endif // DN_UTEST_H

// NOTE: Implementation ////////////////////////////////////////////////////////////////////////////
#if defined(DN_UTEST_IMPLEMENTATION)
void DN_UTest_PrintStats(DN_UTest *test)
{
    if (test->finished)
        return;

    test->finished = true;
    bool all_clear = test->num_tests_ok_in_group == test->num_tests_in_group;
    fprintf(stdout,
            "%s\n  %02d/%02d tests passed -- %s\n\n" DN_UTEST_COLOR_RESET,
            all_clear ? DN_UTEST_GOOD_COLOR : DN_UTEST_BAD_COLOR,
            test->num_tests_ok_in_group,
            test->num_tests_in_group,
            all_clear ? "OK" : "FAILED");
}

void DN_UTest_BeginV(DN_UTest *test, char const *fmt, va_list args)
{
    assert(test->state == DN_UTestState_Nil &&
           "Nesting a unit test within another unit test is not allowed, ensure"
           "the first test has finished by calling DN_UTest_End");

    test->num_tests_in_group++;
    test->state     = DN_UTestState_TestBegun;

    test->name_size = 0;
    {
        va_list args_copy;
        va_copy(args_copy, args);
        test->name_size = vsnprintf(NULL, 0, fmt, args_copy);
        va_end(args_copy);
    }

    assert(test->name_size < sizeof(test->name));
    vsnprintf(test->name, sizeof(test->name), fmt, args);
}

void DN_UTest_Begin(DN_UTest *test, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_UTest_BeginV(test, fmt, args);
    va_end(args);
}

void DN_UTest_End(DN_UTest *test)
{
    assert(test->state != DN_UTestState_Nil && "Test was marked as ended but a test was never commenced using DN_UTest_Begin");
    size_t pad_size = DN_UTEST_RESULT_LPAD - (DN_UTEST_SPACING + test->name_size);
    if (pad_size < 0)
        pad_size = 0;

    char pad_buffer[DN_UTEST_RESULT_LPAD] = {};
    memset(pad_buffer, DN_UTEST_RESULT_PAD_CHAR, pad_size);

    printf("%*s%.*s%.*s", DN_UTEST_SPACING, "", (int)test->name_size, test->name, (int)pad_size, pad_buffer);
    if (test->state == DN_UTestState_TestFailed) {
        printf(DN_UTEST_BAD_COLOR " FAILED");
    } else {
        printf(DN_UTEST_GOOD_COLOR " OK");
        test->num_tests_ok_in_group++;
    }
    printf(DN_UTEST_COLOR_RESET "\n");
    test->state = DN_UTestState_Nil;
}
#endif // DN_UTEST_IMPLEMENTATION
