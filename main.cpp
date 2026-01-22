#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <mutex>
#include <vector>
#include <unordered_map>

using namespace std;
namespace fs = filesystem;

// Shared index: word -> list of files
unordered_map<string, vector<string>> indexMap;
mutex indexMutex;

// Reads a single file
void processFile(const fs::path& filePath) {
    ifstream file(filePath);
    if (!file.is_open()) return;

    string word;
    while (file >> word) {
        lock_guard<mutex> lock(indexMutex);
        indexMap[word].push_back(filePath.filename().string());
    }
}

// Work done by each thread
void worker(const vector<fs::path>& files, int start, int end) {
    for (int i = start; i < end; i++) {
        processFile(files[i]);
    }
}

int main(int argc, char* argv[]) {

    // Step A: Check folder path is given
    if (argc < 2) {
        cerr << "Usage: ./indexer <directory_path>\n";
        return 1;
    }

    vector<fs::path> files;

    // Step B: Collect all .txt files inside the given folder
    for (const auto& entry : fs::recursive_directory_iterator(argv[1])) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            files.push_back(entry.path());
        }
    }

    // Step C: Decide number of threads
    int threadCount = thread::hardware_concurrency();
    if (threadCount == 0) threadCount = 2;

    vector<thread> threads;
    int chunkSize = files.size() / threadCount;

    // Step D: Create threads
    for (int i = 0; i < threadCount; i++) {
        int start = i * chunkSize;
        int end = (i == threadCount - 1) ? files.size() : start + chunkSize;
        threads.emplace_back(worker, cref(files), start, end);
    }

    // Step E: Wait for all threads
    for (auto& t : threads) {
        t.join();
    }

    // Step F: Print index
    for (const auto& [word, fileList] : indexMap) {
        cout << word << ": ";
        for (const auto& file : fileList)
            cout << file << " ";
        cout << "\n";
    }

    return 0;
}
