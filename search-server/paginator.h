#pragma once

#include <iterator>
#include <string>
#include <vector>


template<typename RandomIt>
class IteratorRange
{

public:
    IteratorRange(RandomIt begin, RandomIt end) :begin_(begin), end_(end)
    {
        size_ = std::distance(begin_, end_);
    }


    RandomIt begin() const {
        return begin_;
    }
    RandomIt end() const {
        return end_;
    }
    RandomIt size() const {
        return size_;
    }

private:
    RandomIt begin_;
    RandomIt end_;
    size_t size_;
};

template <typename RandomIt>
std::ostream& operator<<(std::ostream& os, IteratorRange<RandomIt> it)
{
    for (auto i = it.begin(); i != it.end(); i++)
    {
        os << *i;
    }
    return os;
}



template<typename RandomIt>
class Paginator
{
public:
    explicit Paginator(RandomIt begin, RandomIt end, size_t page_size):begin_(begin), end_(end)
    {
        for (RandomIt start = begin; start != end; )
        {
            size_t d = distance(start, end);
            auto finish = page_size < d ? page_size : d;
            IteratorRange it(start, start + finish);
            start += finish;
            pages_.push_back(it);
        }
    }
    auto begin(void) const
    {
        return pages_.begin();
    }
    auto end(void) const
    {
        return pages_.end();
    }
    size_t size(void)
    {
        return distance(this->begin(), this->end());
    }
private:

    std::vector<IteratorRange<RandomIt>> pages_;
    RandomIt begin_, end_;
};


template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}