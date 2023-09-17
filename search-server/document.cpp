#include "document.h"

using namespace std::string_literals;

std::ostream& operator<<(std::ostream& os, Document d)
{
    // example { document_id = 2, relevance = 0.402359, rating = 2 }
    os << "{ document_id = "s << d.id << ", relevance = "s << d.relevance << ", rating = " << d.rating << " }"s;
    return os;
}