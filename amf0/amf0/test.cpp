#include <iostream>
#include <stdio.h>

#include "simple_buffer.h"
#include "amf0.h"

using namespace std;

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            cout << __FILE__ << ":" << __LINE__ << ": expect:" << expect << " actual: " << actual << endl;\
            main_ret = 1;\
        }\
        } while(0)

#define EXPECT_EQ_STRING(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual)

static void test_parse_number()
{
    SimpleBuffer expect, actual;

    Amf0Number expect_number(10);
    expect_number.write(&expect);

    Amf0Number actual_number;
    actual_number.read(&expect);
    actual_number.write(&actual);

    EXPECT_EQ_STRING(expect.to_string(), actual.to_string());
}

static void test_parse_boolean()
{
    SimpleBuffer expect, actual;

    Amf0Boolean expect_boolean(false);
    expect_boolean.write(&expect);

    Amf0Boolean actual_boolean;
    actual_boolean.read(&expect);
    actual_boolean.write(&actual);

    EXPECT_EQ_STRING(expect.to_string(), actual.to_string());
}

static void test_parse()
{
    test_parse_number();
    test_parse_boolean();
}

int main()
{
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}

