#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

struct JobConf {
    std::string job_name;                   // e.g. "word-count"

    std::string input_path;                 // e.g. "/data/shakespeare/"
    std::string output_path;                // e.g. "/out/word-count/"

    std::string mapper_name;                // e.g. "WordCountMapper"
    std::string reducer_name;               // e.g. "SumReducer"
    // std::string combiner_name;           // e.g. "SumCombiner"

    int32_t num_reduce_tasks = 1;           // e.g. 8

    std::unordered_map<std::string, std::string> properties;
                                            // e.g. {{"io.sort.mb", "100"}, {"mapred.task.timeout", "600000"}}
};
