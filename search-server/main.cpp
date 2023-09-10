#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <optional>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

static bool IsNotValidWord(const string& word) {
    // A valid word must not contain special characters
    // возвращает 0 если есть спец. символы
    auto special_sign_flag = (none_of(word.begin(), word.end(), [](char c) {
        return (c >= '\0' && c < ' '); }));


    //return special_sign_flag || incorrect_word_flag;
    return !special_sign_flag || (((word.empty()) || (word.at(0) == '-') || (word.at(word.size() - 1) == '-')));
}

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
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
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
            if (IsNotValidWord(str))
            {
                throw str;
            }
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words) 
    {
        
        // можно было бы сразу выкидывать ислючение в MakeUnique, но IsNotValidWord определен только в классе. Поэтому обходим еще раз
        try
        {
           this->stop_words_ = MakeUniqueNonEmptyStrings(stop_words);
           
        }
        catch (const string& s)
        {
            throw invalid_argument(s);
        }
        
        
    }

    inline static constexpr int INVALID_DOCUMENT_ID = -1;
    int GetDocumentId(int index) const {
        if (index > GetDocumentCount() - 1 || index < 0) 
        {
            throw out_of_range("ID index = "s + to_string(index) + " is out of range"s);
            return INVALID_DOCUMENT_ID;
        }
        return document_id_[index];

    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(
           // convert sting to vector<string>
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {


        if (document_id < 0) 
        {

            throw invalid_argument("Document ID "s + to_string(document_id) + " is negative"s);
        }
        if ( documents_.count(document_id) )
        {

            throw invalid_argument("Document ID "s + to_string(document_id) + " is exist)"s);
        }

        try {
            const vector<string> words = SplitIntoWordsNoStop(document);
            const double inv_word_count = 1.0 / words.size();

            for (const string& word : words)
            {
                word_to_document_freqs_[word][document_id] += inv_word_count;
            }
            document_id_.push_back(document_id);
            documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        }
        catch (string word)
        {
            throw invalid_argument("Word "s + word + " in document with id = "s + to_string(document_id) + " have incorrect charecters"s);
        }

        
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query,
        DocumentPredicate document_predicate) const {


        const Query query = ParseQuery(raw_query);
        try
        {
            auto matched_documents = FindAllDocuments(query, document_predicate);



            sort(matched_documents.begin(), matched_documents.end(),
                [](const Document& lhs, const Document& rhs) {
                    if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                        return lhs.rating > rhs.rating;
                    }
                    else {
                        return lhs.relevance > rhs.relevance;
                    }
                });

            if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
                matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }

            return  matched_documents;
        }
        catch (string word)
        {
            throw invalid_argument("Word "s + word + " is incorrect in query"s);
            return {};

        }
    }

   vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

   vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        if ((document_id) < 0)
        {
            throw invalid_argument("Document ID "s + to_string(document_id) + " is negative"s);
        }
        if (count(document_id_.begin(), document_id_.end(), document_id) == 0)
        {
            throw invalid_argument("Document ID "s + to_string(document_id) + " is not exist"s);
        }
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (IsNotValidWord(word))
                throw invalid_argument("Word "s + word + " is uncorrect"s);
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (IsNotValidWord(word))
                throw invalid_argument("Word "s + word + " is uncorrect"s);
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        DocumentStatus status = documents_.at(document_id).status;

        return make_tuple(matched_words, status);
    }


private:

    vector<int> document_id_ = {};
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;



    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
   
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                if (IsNotValidWord(word))
                {
                    throw (word);
                }
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
   vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (IsNotValidWord(word))
            {
                throw(word);
            }
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {

                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (IsNotValidWord(word))
            {
                throw(word);
            }
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

// ==================== для примера =========================
void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}

void TryStopWords(const string& s)
{
    try
    {
        SearchServer search_server(s);
    }
    catch(invalid_argument& e)
    {
        cout << e.what() << " have incorrect characters " << endl;
            return;
    }
    cout << "IsCorrect" << endl;

}

template<typename container>
void TryStopWords(const container& s)
{
    try
    {
        SearchServer search_server(s);
    }
    catch (invalid_argument& e)
    {
        cout << e.what() << " have incorrect characters " << endl;
        return;
    }
    cout << "IsCorrect" << endl;

}
void TestStopWords(void)
{
    // correct
    vector<string> stop_words1 = { "и", "в", "на", "" };
    TryStopWords(stop_words1);
    vector<string> stop_words2 = {};
    TryStopWords(stop_words2);
    string stop_words3 = "и в на"s;
    TryStopWords(stop_words3);
    string stop_words4 = ""s;
    TryStopWords(stop_words4);
    // incorrect
    vector<string> stop_words5 = { "or\x12", "в", "на", "" };
    string stop_words6 = "or\x12 в на"s;
    TryStopWords(stop_words5);
    TryStopWords(stop_words6);

}

void TryAddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status,
    const vector<int>& ratings)
{
    
    try
    {
        search_server.AddDocument(document_id, document, status,ratings);
    }
    catch (const invalid_argument & e)
    {
        cout << "Error: " << e.what() << endl;
        return;
    }
    cout << "IsCorrect" << endl;
}
void TestAddDocument(void)
{
    std::vector<string> stop_words_vec = { "and", "в", "на", "" };
    SearchServer search_server(stop_words_vec);
    // correct
    TryAddDocument(search_server, 0, "white cat and moddern tail", DocumentStatus::ACTUAL, { 1 });
    //uncorrect
    // id is exist
    TryAddDocument(search_server, 0, "white cat and moddern tail", DocumentStatus::ACTUAL, { 1 });
    // id is negaitve
    TryAddDocument(search_server, -1, "white cat and moddern tail", DocumentStatus::ACTUAL, { 1 });
    // document have uncorrect symbols
    TryAddDocument(search_server, 1, "white\x12 cat and moddern tail", DocumentStatus::ACTUAL, { 1 });
}

void TestDocumentID(void)
{
    std::vector<string> stop_words_vec = { "and", "в", "на", "" };
    SearchServer search_server(stop_words_vec);
    search_server.AddDocument(0, "white cat and moddern tail", DocumentStatus::ACTUAL, { 1 });
    search_server.AddDocument(1, "white cat and moddern tail", DocumentStatus::ACTUAL, { 1 });
    search_server.AddDocument(2, "white cat and moddern tail", DocumentStatus::ACTUAL, { 1 });
    try
    {
        // correct
        search_server.GetDocumentId(1);
        // out of range(4 > 3)
        search_server.GetDocumentId(4);
        
        
    }
    catch (const std::out_of_range& e)
    {
        cout << "Error: " << e.what() << endl;
    }
    
}


void TryFindDocument(SearchServer& search_server, const string& query)
{
    try
    {
        search_server.FindTopDocuments(query);
    }
    catch (invalid_argument& e)
    {
        cout << "Error: " << e.what() << endl;
        return;
    }
    cout << "IsCorrect : " << endl;
}
void TestFindTopDocument(void)
{
    std::vector<string> stop_words_vec = { "and", "в", "на", "" };
    SearchServer search_server(stop_words_vec);
    search_server.AddDocument(0, "white cat and moddern tail", DocumentStatus::ACTUAL, { 1 });
    search_server.AddDocument(1, "black black tail", DocumentStatus::ACTUAL, { 1 });
    search_server.AddDocument(2, "prety dog and perfect yes", DocumentStatus::ACTUAL, { 1 });
    // correct
    TryFindDocument(search_server, "black cat"s);
    // uncorrect
    TryFindDocument(search_server, "black --cat"s);
    TryFindDocument(search_server, "black -"s);
    TryFindDocument(search_server, "black ca\x12t"s);

    
}

void TestSearchServer(void)
{
    cout << "Test is started" << endl;
    cout << "StopWords" << endl;
    TestStopWords();
    cout << "AddDocument" << endl;
    TestAddDocument();
    cout << "DocumentID" << endl;
    TestDocumentID();
    cout << "FindTopDocument" << endl;
    TestFindTopDocument();
}

int main() {

    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
    

}
