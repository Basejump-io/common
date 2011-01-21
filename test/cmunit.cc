/**
 * @file
 * Common classes unit tests
 */

/******************************************************************************
 * $Revision: 14/5 $
 *
 * Copyright 2009-2011, Qualcomm Innovation Center, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 ******************************************************************************/

#include <qcc/platform.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

#include <string>
#include <signal.h>
#include <stdio.h>
#include <vector>

#include <qcc/Debug.h>
#include <qcc/FileStream.h>
#include <qcc/ManagedObj.h>
#include <qcc/String.h>

#include <Status.h>

#define QCC_MODULE "COMMON"

using namespace std;
using namespace qcc;

#define TEST_ASSERT(cond)                       \
    do {                                            \
        if (!(cond)) {                              \
            QCC_LogError(ER_FAIL, ("%s", # cond));   \
            return ER_FAIL;                         \
        }                                           \
    } while (0)

struct Managed {
    Managed() : val(0) {
        printf("Created Managed\n");
    }

    ~Managed() {
        printf("Destroyed Managed\n");
    }

    void SetValue(int val) { this->val = val; }

    int GetValue(void) const { return val; }

  private:
    int val;
};

static QStatus testManagedObj(void)
{

    ManagedObj<Managed> foo0;
    TEST_ASSERT(0 == (*foo0).GetValue());

    ManagedObj<Managed> foo1;
    foo1->SetValue(1);
    TEST_ASSERT(0 == foo0->GetValue());
    TEST_ASSERT(1 == foo1->GetValue());

    foo0 = foo1;
    TEST_ASSERT(1 == foo0->GetValue());
    TEST_ASSERT(1 == foo1->GetValue());

    foo0->SetValue(0);
    TEST_ASSERT(0 == foo0->GetValue());
    TEST_ASSERT(0 == foo1->GetValue());

    return ER_OK;
}

static QStatus testString()
{
    const char* testStr = "abcdefgdijk";

    qcc::String s(testStr);
    TEST_ASSERT(0 == ::strcmp(testStr, s.c_str()));
    TEST_ASSERT(::strlen(testStr) == s.size());

    TEST_ASSERT(3 == s.find_first_of('d'));
    TEST_ASSERT(3 == s.find_first_of('d', 3));
    TEST_ASSERT(3 == s.find_first_of("owed", 3));
    TEST_ASSERT(qcc::String::npos == s.find_first_of('d', 8));

    TEST_ASSERT(7 == s.find_last_of('d'));
    TEST_ASSERT(3 == s.find_last_of('d', 7));
    TEST_ASSERT(qcc::String::npos == s.find_last_of('d', 2));

    TEST_ASSERT(false == s.empty());
    s.clear();
    TEST_ASSERT(true == s.empty());
    TEST_ASSERT(0 == s.size());

    s = testStr;
    return ER_OK;
}

/* This test assumes that ./testFile, ./testDir, and //testDir don't exist prior to running */
static QStatus testFileSink()
{
    const char* pass[] = {
        "testFile",               /* Creation of file */
        "testFile",               /* Open existing file */
        "testDir/foo",            /* Creation of both directory and file */
        "testDir/bar",            /* Creation of file in existing directory */
        "testDir/../testDir/foo", /* Normalize paths and open existing file */
        "testDir//bar",           /* Normalize path for extra slashes */
        "//testDir/foo",          /* Leading slashes */
        "testDir/dir/foo",        /* Create multiple directories */
        "testDir/dir/bar",        /* Creation of file in existing nested directory */
        NULL
    };
    for (const char** pathname = pass; *pathname; ++pathname) {
        FileSink f = FileSink(*pathname, FileSink::PRIVATE);
        TEST_ASSERT(f.IsValid());
    }

    const char* xfail[] = {
        "testDir/dir", /* Create a file that is already an existing directory */
        NULL
    };
    for (const char** pathname = xfail; *pathname; ++pathname) {
        FileSink f = FileSink(*pathname, FileSink::PRIVATE);
        TEST_ASSERT(!f.IsValid());
    }

    return ER_OK;
}

/* Test structure describes an individual test */
struct Test {

    Test(string name, QStatus(*test)(void), string desc) : name(name), test(test), desc(desc) { }

    string name;
    QStatus (* test)(void);
    string desc;
};

/* Test List */
static Test tests[] = {
    Test("ManagedObj", testManagedObj, "Test ManagedObj implementation"),
    Test("String",     testString,     "Test String implementation"),
    Test("FileSink",   testFileSink,   "Test FileSink implementation")
};


int main(int argc, char** argv)
{
    // signal(SIGINT, SigIntHandler);

#ifdef _WIN32
    WSADATA wsaData;
    WORD version = MAKEWORD(2, 0);
    int error = WSAStartup(version, &wsaData);
#endif

    int testsFailed = 0;
    for (size_t i = 0; i < sizeof(tests) / sizeof(Test); ++i) {
        printf("----- STARTING TEST %s ------\n", tests[i].name.c_str());
        printf("----- DESCRIPTION: %s -----\n", tests[i].desc.c_str());
        QStatus result = tests[i].test();
        if (ER_OK == result) {
            printf("----- TEST %s RETURNED SUCCESSFULLY -----\n", tests[i].name.c_str());
        } else {
            printf("----- ERROR: TEST %s returned 0x%x (%s) -----\n", tests[i].name.c_str(), result, QCC_StatusText(result));
            testsFailed++;
        }
    }
    QCC_DbgPrintf(("There were %d test failures\n", testsFailed));
    return testsFailed;
}
