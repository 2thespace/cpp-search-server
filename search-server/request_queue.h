#pragma once
#include <vector>
#include <string>
#include "search_server.h"
#include <deque>

const std::string EMPTY_REQUEST = std::string("empty request");

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {

        std::string request;
        std::vector<Document> result;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int zero_request_count_;
    int current_min_;

};

// template
template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    // напишите реализацию
    current_min_++;
    current_min_ %= min_in_day_; // работаем по модулю 1440
    auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
    if (result.empty())
    {
        zero_request_count_++;
    }
    while (requests_.size() >= min_in_day_)
    {
        if (requests_.front().result.empty())
        {
            zero_request_count_--;
        }
        requests_.pop_front();
    }
    requests_.push_back({ raw_query, result });
    return requests_.back().result;
}
