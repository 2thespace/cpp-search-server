#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include<cmath> // для логарифма

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const int words_amount = words.size(); // общее количество слов
        const double invers_words_amount = 1.0 / words_amount;
        for (auto& word : words)
        {

            documents_[word][document_id] += invers_words_amount; // вычисление TF
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    void SetDocument_count_(int newDoc)
    {
        document_count_ = newDoc;
    }
private:
    struct Query
    {
        set<string> minusWord;
        set<string>plusWord;
    };

    // <слово <id, TF>>
    map<string, map<int, double>> documents_;

    set<string> stop_words_;
    int document_count_ = 0;
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const {
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-')
            {
                query_words.minusWord.insert(word.substr(1));
            }
            else
            {
                query_words.plusWord.insert(word);
            }

        }
        return query_words;
    }

    vector<Document> FindAllDocuments(const Query& query_words) const {
        vector<Document> matched_documents;
        // <id, relevance>
        map<int, double> document_to_relevance_;
        double IDF = 0;
        // здесь вычисляется IDF
        // IDF = log(allDocs/words[docs]
        for (const auto& plusWord : query_words.plusWord) {
            if (documents_.count(plusWord))
            {
                double size = documents_.at(plusWord).size();
                IDF = CalculateIDF(size);

                map<int, double> all_id_tf = documents_.at(plusWord);
                for (auto& [id, TF] : all_id_tf)
                {
                    document_to_relevance_[id] += IDF * TF;
                }
            }
        }
        for (const auto& minusWord : query_words.minusWord)
        {
            if (documents_.count(minusWord))
            {
                map<int, double> all_id_tf = documents_.at(minusWord);
                for (auto& [id, TF] : all_id_tf)
                {
                    document_to_relevance_.erase(id);
                }
            }
        }
        for (auto& [id, relevance] : document_to_relevance_)
            matched_documents.push_back({ id, relevance });
        return matched_documents;
    }
     double CalculateIDF(double size) const
    {
        return log(document_count_ / size);
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    search_server.SetDocument_count_(document_count);
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();
    setlocale(LC_ALL, "Russian");
    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}