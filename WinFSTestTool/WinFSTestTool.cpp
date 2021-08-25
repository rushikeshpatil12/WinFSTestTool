// WinFSTestTool.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <set>
#include <Windows.h>

using namespace std;

const std::string TestDir = "C:\\FSTestDir";

static const uint64_t maxFsOpIterations = 100000;
static const uint64_t maxIOIterations = 5000000;
static const uint64_t pageSize = 4096;

std::string testFileName;
std::set<std::string> filesTobeDeleted;

enum class PERF_TEST_ID {
    TEST_OPEN_CLOSE,
    TEST_READ,
    TEST_WRITE,
    TEST_RENAME,
    TEST_DELETE,
    TEST_ALL,
    TEST_INVALID
};

void Usage() 
{
    std::cout
        << "Usage: Use [options] below for FS performance(FS OP latency) testing." << std::endl
        << "    " << (uint32_t)PERF_TEST_ID::TEST_OPEN_CLOSE << "  : Open/Close Tests." << std::endl
        << "    " << (uint32_t)PERF_TEST_ID::TEST_READ << "  : Read tests." << std::endl
        << "    " << (uint32_t)PERF_TEST_ID::TEST_WRITE << "  : Write test." << std::endl
        << "    " << (uint32_t)PERF_TEST_ID::TEST_RENAME << "  : Rename test." << std::endl
        << "    " << (uint32_t)PERF_TEST_ID::TEST_DELETE << "  : Delete test." << std::endl
        << "    " << (uint32_t)PERF_TEST_ID::TEST_ALL << "  : Run All tests." << std::endl
        << std::endl;
    exit(0);
}

void TestOpenClose()
{
    std::string filename = TestDir + "\\Test_" + std::to_string(rand() % 1000) + ".txt";

    std::fstream file;
    for (auto i = 0; i < maxFsOpIterations; i++) {
        file.open(filename, std::ios::in);
        if (!file) {
            file.open(filename, std::ios::out);
        }
        if (!file) {
            cerr << "Failed to open the file: " << filename << endl;
        }

        file.close();
    }

    DeleteFileA(filename.c_str());

    cout << "[OP_COUNT] OpenClose: " << maxFsOpIterations << std::endl;
}


void TestRead()
{
    std::fstream file;
    file.open(testFileName, std::ios::in | std::ios_base::binary);
    uint32_t pagesRead = 0;
    char buff[pageSize] = {};

    if (!file) {
        cerr << "Failed to open the file " << testFileName << std::endl;
        return;
    }

    while (!file.eof()) {
        file.read(buff, pageSize);
        if (!file) {
            cout << "Finished reading file " << testFileName << " total pages read : " << pagesRead << endl;
            if (maxIOIterations != pagesRead) {
                cout << "All Read ops didn't finish... Expected Reads: " << maxIOIterations << " Actual reads: " << pagesRead << endl;
            }
            break;
        }

        pagesRead++;
    }

    cout << "[OP_COUNT] Read: " << pagesRead << std::endl;
    DeleteFileA(testFileName.c_str());
}

void TestWrite()
{
    std::fstream file;
    std::string filename = TestDir + "\\Test_" + std::to_string(rand() % 1000) + ".txt";

    file.open(filename, std::ios::out | std::ios_base::binary);
    uint32_t pagesWritten = 0;
    char buff[pageSize];

    if (!file) {
        cerr << "Failed to open the file " << filename << endl;
        return;
    }

    while (pagesWritten < maxIOIterations) {
        file.write(buff, pageSize);
        if (!file) {
            cerr << "Error writing to file " << filename << " total pages written : " << pagesWritten << endl;
            if (maxIOIterations != pagesWritten) {
                cout << "All Read ops didn't finish... Expected Reads: " << maxIOIterations << " Actual reads: " << pagesWritten << endl;
            }

            break;
        }

        pagesWritten++;
    }

    cout << "[OP_COUNT] Write: " << pagesWritten << std::endl;

    DeleteFileA(filename.c_str());
}

void TestRename()
{
    std::string name1 = TestDir + "\\Test_" + std::to_string(rand() % 1000) + ".txt";
    std::string name2 = TestDir + "\\Test_" + std::to_string(rand() % 1000) + ".txt";
    std::fstream file;

    file.open(name1, std::ios::out);

    if (!file) {
        cerr << "(TestRename)Failed to open the file " << testFileName << endl;
        return;
    }
    file.close();

    uint32_t renames = 0;
    while (renames < maxFsOpIterations) {
        if (!MoveFileExA(name1.c_str(), name2.c_str(), MOVEFILE_REPLACE_EXISTING)) {
            cerr << "MoveFile failed for: oldname: " << name1
                << " Newname " << name2
                << " Error: " << GetLastError() << endl;
        }
        renames++;

        if (!MoveFileExA(name2.c_str(), name1.c_str(), MOVEFILE_REPLACE_EXISTING)) {
            cerr << "MoveFile failed for: oldname: " << name2
                << " Newname " << name1
                << " Error: " << GetLastError() << endl;
        }
        renames++;
    }

    cout << "[OP_COUNT] Rename: " << renames << std::endl;

    DeleteFileA(name1.c_str());
    DeleteFileA(name2.c_str());
}


void createTempFilesForDeletion()
{
    std::fstream file;

    for (auto i = 0; i < maxFsOpIterations; i++) {
        std::string filename = TestDir + "\\Test_" + std::to_string(rand() % 1000) + ".txt";

        file.open(filename, std::ios::out);
        if (!file) {
            cerr << "Failed to open the file" << endl;
        }
        filesTobeDeleted.insert(filename);
        file.close();
    }
}

void TestDelete()
{
    for (auto name : filesTobeDeleted) {
        DeleteFileA(name.c_str());
    }
    filesTobeDeleted.clear();

    cout << "[OP_COUNT] Delete: " << maxFsOpIterations << std::endl;
}

void createBigFile()
{
    testFileName = TestDir + "\\Test_" + std::to_string(rand() % 1000) + ".txt";
    uint64_t fsize = maxIOIterations * pageSize;
    DeleteFileA(testFileName.c_str());

    std::string cmd = "fsutil file createnew " + testFileName + " " + std::to_string(fsize);
    cout <<"Creating a file with command: " << cmd << endl;
    system(cmd.c_str()); // ignore error

    cmd = "fsutil file setValidData " + testFileName + " " + std::to_string(fsize);
    cout <<"Setting file size with command: " << cmd << endl;
    system(cmd.c_str()); // ignore error
}

void PrepareTest(PERF_TEST_ID perfTest)
{
    if (!CreateDirectoryA(TestDir.c_str(), NULL) &&
        ERROR_ALREADY_EXISTS != GetLastError()) {
        cerr << "Error creating Test directory: " << TestDir << endl;
        return;
    }

    switch (perfTest) {
    case PERF_TEST_ID::TEST_DELETE:
        createTempFilesForDeletion();
        break;
    case PERF_TEST_ID::TEST_READ:
        createBigFile();
        break;
    }
}

void PerformTestRun(PERF_TEST_ID perfTest)
{
    PrepareTest(perfTest);
    uint64_t iterations = maxFsOpIterations;
    auto t_start = std::chrono::high_resolution_clock::now();
    switch (perfTest) {
    case PERF_TEST_ID::TEST_OPEN_CLOSE:
        TestOpenClose();
        break;
    case PERF_TEST_ID::TEST_DELETE:
        TestDelete();
        break;
    case PERF_TEST_ID::TEST_READ:
        iterations = maxIOIterations;
        TestRead();
        break;
    case PERF_TEST_ID::TEST_WRITE:
        iterations = maxIOIterations;
        TestWrite();
        break;
    case PERF_TEST_ID::TEST_RENAME:
        TestRename();
        break;

    }

    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(t_end - t_start).count();

    cout << "Total Duration: " << duration << " ms" << " Avg. latency: " << duration / iterations << " ms" << endl;
}

PERF_TEST_ID GetTestOption(std::string testParam) {
    long testOption = atoi(testParam.c_str());
    if ((PERF_TEST_ID)testOption >= PERF_TEST_ID::TEST_INVALID)
        return PERF_TEST_ID::TEST_INVALID;

    return (PERF_TEST_ID)testOption;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        Usage();
    }

    PERF_TEST_ID testOp = GetTestOption(argv[1]);
    if (PERF_TEST_ID::TEST_INVALID == testOp) {
        Usage();
    }

    if (PERF_TEST_ID::TEST_ALL == testOp) {
        PerformTestRun(PERF_TEST_ID::TEST_OPEN_CLOSE);
        cout << "**************Open/Close Test Finished **************" << endl;
        PerformTestRun(PERF_TEST_ID::TEST_DELETE);
        cout << "**************Delete Test Finished **************" << endl;
        PerformTestRun(PERF_TEST_ID::TEST_READ);
        cout << "**************Read Test Finished **************" << endl;
        PerformTestRun(PERF_TEST_ID::TEST_WRITE);
        cout << "**************Write Test Finished **************" << endl;
        PerformTestRun(PERF_TEST_ID::TEST_RENAME);
        cout << "**************Rename Test Finished **************" << endl;
    }
    else {
        PerformTestRun(testOp);
    }

}
