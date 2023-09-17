#include "request_queue.h"


// public
RequestQueue::RequestQueue(const SearchServer& search_server) :search_server_(search_server)
{
    // напишите реализацию
    zero_request_count_ = 0;
    current_min_ = 0;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    // напишите реализацию
    return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating)
        {
            return document_status == status;
        });
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    // напишите реализацию
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);

}

int RequestQueue::GetNoResultRequests() const {
    // напишите реализацию
    return zero_request_count_;
}


// private 