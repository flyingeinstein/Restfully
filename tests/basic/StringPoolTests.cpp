//
// Created by Colin MacKenzie on 5/18/17.
//

#define CATCH_CONFIG_FAST_COMPILE

#include <catch.hpp>
#include <cstring>
#include <stdio.h>
#include <StringPool.h>
#include <sstream>


#define SAMPLE_L66 "Contrary to popular belief, Lorem Ipsum is not simply random text."
#define SAMPLE_L97 "It has roots in a piece of classical Latin literature from 45 BC, making it over 2000 years old."
#define SAMPLE_L165 SAMPLE_L66 " " SAMPLE_L97

#define LIST_OF_NAMES "jim,john,mary,frank,maya,julia,,matthew,david,greg,colin,kinga"

#define LOREM_IPSUM "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam euismod ex ut pulvinar sagittis. Donec eget condimentum ante. Donec pulvinar molestie lorem vel scelerisque. Nam a vulputate justo, quis ullamcorper leo. Morbi ut pretium neque. Morbi ut elit non ex ornare faucibus. Nullam eros nunc, tempus at justo at, viverra rutrum nulla. Morbi nibh arcu, convallis quis blandit eu, laoreet eget nibh. Aenean consequat bibendum tellus. Mauris et ex sit amet lectus feugiat posuere eget et arcu. Ut ac sem at diam porttitor malesuada et ut velit.\n\n" \
                    "Cras non velit cursus, elementum velit at, iaculis leo. Proin sed nisi pellentesque, iaculis justo sollicitudin, congue lacus. Etiam convallis nisl a est finibus porttitor. In hac habitasse platea dictumst. Vivamus rhoncus, tortor a vehicula bibendum, nunc turpis fermentum elit, vel fringilla urna lorem ut magna. Suspendisse potenti. Donec a dui ac dui eleifend malesuada. Suspendisse massa nisi, consectetur et tortor eget, eleifend luctus urna. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec tincidunt luctus placerat. Maecenas nec dui orci. "

#define SAMPLE_API_1_WORDS "api,sys,status,deviceid,version,restart,firmware,upload,license,log,event,adp,alarm,meta,request,eventlog,adp-log,alarm-log,dai,aliases,download,alias,vod,purge,metadata,assets,pattern,export,import,TEST_CASE,validate,stream,initStart,control,start,stop,extend,query,manifest,scanning,rvl,schedule,capture,ports,files,enums,configure,analytics,schema,view,config,save,modules,module,section,templates,summary,debug,csv,time,set,service,ntpd,oauth,policy,sparkle,daily,sysstatus"
#define SAMPLE_API_2_WORDS "sysstatus,payload,daily,sparkle,policy,oauth,ntpd,service,set,time,csv,debug,summary,templates,section,module,modules,save,config,view,schema,analytics,configure,enums,files,ports,capture,schedule,metadata,rvl,scanning,manifest,query,extend,stop,start,control,initStart,stream,validate,TEST_CASE,import,export,pattern,assets,purge,vod,alias,download,aliases,dai,alarm-log,adp-log,eventlog,request,meta,alarm,adp,event,log,license,upload,firmware,restart,version,deviceid,status,sys,api"

using Rest::StringPool;

TEST_CASE("new StringPool consumes zero space", "[binbag]")
{
    StringPool bb;
    REQUIRE (bb.bytes()==0);
    REQUIRE (bb.count()==0);
    REQUIRE (bb.capacity()==0);
}

SCENARIO("StringPool can insert string within a single page", "[binbag]")
{
    GIVEN("An empty binbag") {
        StringPool bb(1024);

        REQUIRE( bb.count()==0 );
        REQUIRE( bb.bytes()==0 );
        REQUIRE( bb.capacity()==0 );

        WHEN("a string is inserted") {
            bb.insert(SAMPLE_L66);
            THEN("count becomes 1") {
                REQUIRE( bb.count()==1 );
            }
            THEN("capacity increases") {
                REQUIRE( bb.capacity() > 60 );
            }
            THEN("size equals the string length") {
                REQUIRE( bb.bytes() == strlen(SAMPLE_L66)+1+sizeof(char*) );
            }
            THEN("string matches by index") {
                REQUIRE( strcmp(bb[0], SAMPLE_L66)==0 );
            }
            THEN("find string returns index") {
                REQUIRE( bb.find(SAMPLE_L66)==0 );
            }
        }

        WHEN("a second string is inserted") {
            StringPool::index_type idx66 = bb.insert(SAMPLE_L66);
            StringPool::index_type idx97 = bb.insert(SAMPLE_L97);
            THEN("count becomes 2") {
                REQUIRE( bb.count()==2 );
            }
            THEN("capacity increases") {
                REQUIRE( bb.capacity() > sizeof(SAMPLE_L97)+sizeof(SAMPLE_L66));
            }
            THEN("size equals the string length") {
                REQUIRE( bb.bytes() == sizeof(SAMPLE_L97)+sizeof(SAMPLE_L66)+2*sizeof(char*));
            }
            THEN("string indexes are 0 & 1") {
                REQUIRE(idx66 == 0);
                REQUIRE(idx97 == 1);
            }
            THEN("new string matches by index") {
                REQUIRE( strcmp(bb[idx97], SAMPLE_L97)==0 );
            }
            THEN("old string matches by index") {
                REQUIRE( strcmp(bb[idx66], SAMPLE_L66)==0 );
            }
            THEN("find new string returns index") {
                REQUIRE( bb.find(SAMPLE_L97)==1 );
            }
            THEN("find old string returns index") {
                REQUIRE(bb.find(SAMPLE_L66) == 0);
            }
        }
    }
}

SCENARIO("StringPool can insert string within two pages", "[binbag]")
{
    GIVEN("An empty binbag") {
        StringPool bb(80);

        REQUIRE( bb.count()==0 );
        REQUIRE( bb.bytes()==0 );
        REQUIRE( bb.capacity()==0 );

        WHEN("a string is inserted") {
            bb.insert(SAMPLE_L66);
            THEN("count becomes 1") {
                REQUIRE( bb.count()==1 );
            }
            THEN("capacity increases") {
                REQUIRE( bb.capacity() > 60 );
            }
            THEN("size equals the string length") {
                REQUIRE( bb.bytes() == strlen(SAMPLE_L66)+1+sizeof(char*) );
            }
            THEN("string matches by index") {
                REQUIRE( strcmp(bb[0], SAMPLE_L66)==0 );
            }
            THEN("find string returns index") {
                REQUIRE( bb.find(SAMPLE_L66)==0 );
            }
        }

        WHEN("a second string is inserted") {
            StringPool::index_type idx66 = bb.insert(SAMPLE_L66);
            StringPool::index_type idx97 = bb.insert(SAMPLE_L97);
            THEN("count becomes 2") {
                REQUIRE(bb.count() == 2);
            }
            THEN("capacity increases") {
                REQUIRE(bb.capacity() > sizeof(SAMPLE_L97) + sizeof(SAMPLE_L66));
            }
            THEN("size equals the string length") {
                REQUIRE(bb.bytes() == sizeof(SAMPLE_L97) + sizeof(SAMPLE_L66)+2*sizeof(char*));
            }
            THEN("string indexes are 0 & 1") {
                REQUIRE(idx66 == 0);
                REQUIRE(idx97 == 1);
            }
            THEN("new string matches by index") {
                REQUIRE(strcmp(bb[idx97], SAMPLE_L97) == 0);
            }
            THEN("old string matches by index") {
                REQUIRE(strcmp(bb[idx66], SAMPLE_L66) == 0);
            }
            THEN("find new string returns index") {
                REQUIRE(bb.find(SAMPLE_L97) == 1);
            }
            THEN("find old string returns index") {
                REQUIRE(bb.find(SAMPLE_L66) == 0);
            }
        }

        WHEN("a third string is inserted") {
            StringPool::index_type idx97 = bb.insert(SAMPLE_L97);
            StringPool::index_type idx66 = bb.insert(SAMPLE_L66);
            StringPool::index_type idx165 = bb.insert(SAMPLE_L165);
            THEN("count becomes 2") {
                REQUIRE(bb.count() == 3);
            }
            THEN("capacity increases") {
                REQUIRE(bb.capacity() > sizeof(SAMPLE_L97) + sizeof(SAMPLE_L66) + sizeof(SAMPLE_L165));
            }
            THEN("size equals the string length") {
                REQUIRE(bb.bytes() == sizeof(SAMPLE_L97) + sizeof(SAMPLE_L66) + sizeof(SAMPLE_L165)+3*sizeof(char*));
            }
            THEN("string indexes are 0,1 & 2") {
                REQUIRE(idx97 == 0);
                REQUIRE(idx66 == 1);
                REQUIRE(idx165 == 2);
            }
            THEN("first string matches by index") {
                REQUIRE(strcmp(bb[idx97], SAMPLE_L97) == 0);
            }
            THEN("second string matches by index") {
                REQUIRE(strcmp(bb[idx66], SAMPLE_L66) == 0);
            }
            THEN("third string matches by index") {
                REQUIRE(strcmp(bb[idx165], SAMPLE_L165) == 0);
            }
            THEN("find first string returns index") {
                REQUIRE(bb.find(SAMPLE_L97) == 0);
            }
            THEN("find second string returns index") {
                REQUIRE(bb.find(SAMPLE_L66) == 1);
            }
            THEN("find thirdstring returns index") {
                REQUIRE(bb.find(SAMPLE_L165) == 2);
            }
        }
    }
}

std::vector<std::string> generate_strings(int count, int min_words, int max_words, std::string source)
{
    // tokenize the source
    std::string word;
    std::vector<std::string> words;
    for(const auto c : source) {
        if(isalpha(c))
            word += c;
        else if(word.length() >0) {
            words.insert(words.end(), word);
            word = std::string();
        }
    }

    std::vector<std::string> out;
    for(int i=0; i<count; i++) {
        std::stringstream ss;
        int n = (max_words==min_words)
                ? min_words
                : min_words + rand() % (max_words-min_words);
        for(int j=0; j<n; j++) {
            int n = rand() % words.size();
            std::string word = words[n];
            if(j == 0) {
                // first word
                word[0] = toupper(word[0]); // upper case first word
            } else {
                // subsequent word
                ss << ' ';
                word[0] = tolower(word[0]); // always lower case first char in word
            }
            ss << word;
        }
        if(min_words<max_words || min_words>1)  // dont add period if min_words==max_words==1
            ss << '.';
        out.insert(out.begin(), ss.str());
    }

    return out;
}

void test_strings(StringPool& bb, const std::vector<std::string>& strings)
{
    // generate a number of sentences
    std::vector<StringPool::index_type> IDs(strings.size());

    size_t n=0, id;
    for( auto const& s : strings) {
        id = IDs[n] = bb.insert_distinct(s.c_str());
        n++;

        REQUIRE ( s == bb[id] );
    }

    for( int i=0; i<strings.size(); i++) {
        const std::string& s = strings[i];
        StringPool::index_type id = IDs[i];

        REQUIRE ( bb.find(s.c_str()) == id );
        REQUIRE ( s == bb[id] );
    }
}

const char* split_string(int sep, const char* str, char* word, size_t sz_word)
{
    const char *p=str;

    if(str==nullptr || *str==0)
        return nullptr;

    // find next seperator
    while (*p && *p != sep)
        p++;

    size_t cnt = p - str;
    if (word!=nullptr && cnt > 0) {
        if(cnt >= sz_word-1)
            cnt = sz_word-1;

        memcpy(word, str, cnt);
        word[cnt] = 0;
    }

    return *p ? (p+1) : p;
}

const char* split_string_inline(int sep, const char* str, const char*& _begin, const char*& _end)
{
    const char *p=str;

    if(str== nullptr || *str==0)
        return nullptr;

    // find next seperator
    while (*p && *p != sep)
        p++;

    size_t cnt = p - str;
    if (cnt > 0) {
        _begin = str;
        _end = str + cnt;
    }

    return *p ? (p+1) : p;
}

std::vector<std::string> split_string_to_vector(int sep, const char* str) {
    std::vector<std::string> strings;
    char word[512];
    const char* p = str;

    // grab a word from the input each iteration
    while(NULL != (p = split_string(sep, p, word, sizeof(word))) ) {
        if(word[0]!=0)
            strings.insert(strings.end(), word);
    }
    return strings;
}

long split_string_add(StringPool& bb, int sep, const char* str) {
    char word[512];
    const char* p = str;
    ssize_t total = 0;

    // grab a word from the input each iteration
    while(NULL != (p = split_string(sep, p, word, sizeof(word))) ) {
        long idx = bb.insert_distinct(word);
        if (idx < 0)
            return idx;
        total++;
    }
    return total;
}

long split_string_add_by_range(StringPool& bb, int sep, const char* str) {
    char word[512];
    const char* p = str;
    const char *_begin, *_end;
    ssize_t total = 0;

    // grab a word from the input each iteration
    while(NULL != (p = split_string_inline(sep, p, _begin, _end)) ) {
        long idx = bb.insert_distinct(_begin, _end-_begin, strncmp);
        if (idx < 0)
            return idx;
        total++;
    }
    return total;
}

TEST_CASE("StringPool inserting 5 words with pagesize 128b","[stringpool]")
{
    // generate a number of sentences
    StringPool bb(128);
    auto strings = generate_strings(5, 1, 1, LOREM_IPSUM);
    test_strings(bb, strings);
}

TEST_CASE("StringPool inserting 50 words with pagesize 128b","[stringpool]")
{
    // generate a number of sentences
    StringPool bb(128);
    auto strings = generate_strings(50, 1, 1, LOREM_IPSUM);
    test_strings(bb, strings);
}

TEST_CASE("StringPool inserting 500 words with pagesize 128b","[stringpool]")
{
    // generate a number of sentences
    StringPool bb(128);
    auto strings = generate_strings(500, 1, 1, LOREM_IPSUM);
    test_strings(bb, strings);
}

TEST_CASE("StringPool inserting 5 sentences with pagesize 256b","[stringpool]")
{
    // generate a number of sentences
    StringPool bb(256);
    auto strings = generate_strings(5, 1, 25, LOREM_IPSUM);
    test_strings(bb, strings);
}

TEST_CASE("StringPool inserting 50 sentences with pagesize 256b","[stringpool]")
{
    // generate a number of sentences
    StringPool bb(256);
    auto strings = generate_strings(50, 1, 25, LOREM_IPSUM);
    test_strings(bb, strings);
}

TEST_CASE("StringPool inserting 500 sentences with pagesize 256b","[stringpool]")
{
    // generate a number of sentences
    StringPool bb(256);
    auto strings = generate_strings(500, 1, 25, LOREM_IPSUM);
    test_strings(bb, strings);
}

TEST_CASE("StringPool can iterate over a list of API keywords (API-2)","[stringpool]")
{
    auto bb = StringPool::split(',', SF_DISTINCT_NO_CASE, SAMPLE_API_2_WORDS);
    auto strings = split_string_to_vector(',', SAMPLE_API_2_WORDS);
    for(StringPool::const_iterator s=bb.begin(), _s=bb.end(); s!=_s; s++) {
        auto strings_itr = std::find(strings.begin(), strings.end(), *s);
        REQUIRE ( strings_itr != strings.end() );
    }
}

TEST_CASE("StringPool can iterate over a range (c++11) of API keywords (API-2)","[stringpool]")
{
    auto bb = StringPool::split(',', SF_DISTINCT_NO_CASE, SAMPLE_API_2_WORDS);
    auto strings = split_string_to_vector(',', SAMPLE_API_2_WORDS);
    for(const auto& s : bb) {
        auto strings_itr = std::find(strings.begin(), strings.end(), s);
        REQUIRE ( strings_itr != strings.end() );
    }
}

TEST_CASE("StringPool various string split cases","[stringpool]")
{
    SECTION("split empty string") {
        auto bb = StringPool::split(',', SF_NONE, "");
        REQUIRE (bb.count() == 0);
        REQUIRE (bb.bytes() == 0);
    }

    SECTION("split singleton string") {
        auto bb = StringPool::split(',', SF_NONE, "single entry");
        REQUIRE (bb.count() == 1);
        REQUIRE (bb.bytes() == 13 + sizeof(char *)); // sizeof string plus 1 entry in index
    }

    SECTION("split singleton string with terminator") {
        auto bb = StringPool::split(',', SF_IGNORE_EMPTY, "single entry,,,");
        REQUIRE (bb.count() == 1);
        REQUIRE (bb.bytes() == 13 + sizeof(char *)); // sizeof string plus 1 entry in index
    }

    SECTION("split many names with ending terminator") {
        auto bb = StringPool::split(',', SF_IGNORE_EMPTY,
                                    "jim,john,mary,frank,maya,julia,matthew,david,greg,colin,kinga,");
        REQUIRE (bb.count() == 11);
    }

    SECTION("split names with empty entries") {
        auto bb = StringPool::split(',', SF_IGNORE_EMPTY,
                                    "jim,john,,mary,frank,maya,,julia,,matthew,david,greg,colin,kinga");
        REQUIRE (bb.count() == 11);
    }

    SECTION("split names with empty entries and ending terminator") {
        auto bb = StringPool::split(',', SF_IGNORE_EMPTY,
                                    "jim,john,,mary,frank,maya,,julia,,matthew,david,greg,colin,kinga,");
        REQUIRE (bb.count() == 11);
    }

    SECTION("split names that count duplicates") {
        auto bb = StringPool::split(',', SF_IGNORE_EMPTY,
                                    "jim,john,mary,frank,maya,julia,matthew,julia,david,greg,john,colin,kinga");
        REQUIRE (bb.count() == 13);
    }

    SECTION("split names that eliminate duplicates except counts case differences") {
        auto bb = StringPool::split(',', SF_IGNORE_EMPTY|SF_DISTINCT,
                                    "jim,john,mary,frank,maya,julia,matthew,julia,david,Julia,greg,john,colin,John,kinga");
        REQUIRE (bb.count() == 13);
    }

    SECTION("split names that eliminate duplicates") {
        auto bb = StringPool::split(',', SF_IGNORE_EMPTY|SF_DISTINCT,
                                    "jim,john,mary,frank,maya,julia,matthew,julia,david,greg,john,colin,kinga");
        REQUIRE (bb.count() == 11);
    }

    SECTION("split names that count duplicates ignoring case") {
        auto bb = StringPool::split(',', SF_IGNORE_EMPTY|SF_DISTINCT_NO_CASE,
                                    "jim,john,mary,julia,frank,maya,julia,matthew,Julia,david,greg,joHn,colin,kinga");
        REQUIRE (bb.count() == 11);
    }

    SECTION("split a string into names")
    {
        auto bb = StringPool::split(',', SF_DISTINCT_NO_CASE, LIST_OF_NAMES);
        auto strings = split_string_to_vector(',', LIST_OF_NAMES);

        for(const auto& s : strings) {
            REQUIRE ( bb.find(s.c_str()) >= 0 );
        }
    }

    SECTION("split a string into a set of API keywords (API-1)")
    {
        auto bb = StringPool::split(',', SF_DISTINCT_NO_CASE, SAMPLE_API_1_WORDS);
        auto strings = split_string_to_vector(',', SAMPLE_API_1_WORDS);
        for(const auto& s : strings) {
            auto id = bb.find(s.c_str());
            REQUIRE ( id >= 0 );

            auto _str = bb[id];
            REQUIRE ( strlen(_str) == s.length() );
            REQUIRE ( s == _str );
        }
    }

    SECTION("split a string into a set of API keywords (API-2)")
    {
        auto bb = StringPool::split(',', SF_DISTINCT_NO_CASE, SAMPLE_API_2_WORDS, 64);
        auto strings = split_string_to_vector(',', SAMPLE_API_2_WORDS);
        for(const auto& s : strings) {
            auto id = bb.find(s.c_str());
            REQUIRE ( id >= 0 );

            auto _str = bb[id];
            REQUIRE ( strlen(_str) == s.length() );
            REQUIRE ( s == _str );
        }
    }

    SECTION("split a string into a set of API keywords (API-2) into a single page")
    {
        auto bb = StringPool::split(',', SF_DISTINCT_NO_CASE | SF_SINGLE_PAGE, SAMPLE_API_2_WORDS);
        auto strings = split_string_to_vector(',', SAMPLE_API_2_WORDS);
        for(const auto& s : strings) {
            auto id = bb.find(s.c_str());
            REQUIRE ( id >= 0 );

            auto _str = bb[id];
            REQUIRE ( strlen(_str) == s.length() );
            REQUIRE ( s == _str );
        }
    }

}

TEST_CASE("StringPool insert using substring","[stringpool]")
{
    StringPool bb;
    long first, last, middle;
    const char* fullname = "Colin Freeman MacKenzie";

    first=bb.insert(fullname, 5);
    REQUIRE ( strcmp(bb[first], "Colin") ==0 );

    middle=bb.insert(fullname+6, 7);
    REQUIRE ( strcmp(bb[middle], "Freeman") ==0 );

    last=bb.insert(fullname+14);
    REQUIRE ( strcmp(bb[last], "MacKenzie") ==0 );
}


TEST_CASE("StringPool insert distinct words from 'There was an old lady'","[stringpool]")
{
    StringPool bb;
    auto line1 = split_string_to_vector(' ', "She swallowed the goat to catch the dog");
    auto line2 = split_string_to_vector(' ', "She swallowed the dog to catch the cat");
    auto line3 = split_string_to_vector(' ', "She swallowed the cat to catch the bird");

    for(auto const& w : line1)
        bb.insert_distinct(w.c_str());
    REQUIRE ( bb.count() == 7 );

    for(auto const& w : line2)
        bb.insert_distinct(w.c_str());
    REQUIRE ( bb.count() == 8 );

    for(auto const& w : line3)
        bb.insert_distinct(w.c_str());
    REQUIRE ( bb.count() == 9 );
}

TEST_CASE("StringPool insert distinct words from 'There was an old lady' using char-range insert","[stringpool]")
{
    StringPool bb;
    auto line1 = "She swallowed the goat to catch the dog";
    auto line2 = "She swallowed the dog to catch the cat";      // note: catch has the keyword cat in it, but should be counted unique
    auto line3 = "She swallowed the cat to catch the bird";

    split_string_add_by_range(bb, ' ', line1);
    REQUIRE ( bb.count() == 7 );

    split_string_add_by_range(bb, ' ', line2);
    REQUIRE ( bb.count() == 8 );

    split_string_add_by_range(bb, ' ', line3);
    REQUIRE ( bb.count() == 9 );
}

TEST_CASE("StringPool various find cases","[stringpool]")
{
    StringPool bb = StringPool::split(' ', 0, "jim john mary frank maya julia matthew david greg colin kinga");
    REQUIRE ( bb.count() == 11);

    SECTION("can find julia") {
        REQUIRE (bb.find("julia") == 5);
        REQUIRE (bb.find("Julia") == -1);
        REQUIRE (bb.find_nocase("juliax") == -1);   // negative case
    }
    SECTION("can find Julia using case insensitive") {
        REQUIRE (bb.find_nocase("julia") == 5);
        REQUIRE (bb.find_nocase("Julia") == 5);
        REQUIRE (bb.find_nocase("Juliax") == -1);   // negative case
    }

    SECTION("can find kinga using range") {
        REQUIRE (bb.find("kinga mackenzie", 5) == 10);
        REQUIRE (bb.find("Kinga MacKenzie", 5) == -1);          // negative case
        REQUIRE (bb.find_nocase("kinga mackenzie", 6) == -1);   // negative case
    }
    SECTION("can find Kinga using case insensitive") {
        REQUIRE (bb.find_nocase("kinga mackenzie", 5) == 10);
        REQUIRE (bb.find_nocase("Kinga MacKenzie", 5) == 10);
        REQUIRE (bb.find_nocase("kinga mackenzie", 6) == -1);   // negative case
    }

    SECTION("can find jim") {
        REQUIRE (bb.find("jim") == 0);
        REQUIRE (bb.find("Jim") == -1);
        REQUIRE (bb.find_nocase("jimx") == -1);   // negative case
    }
    SECTION("can find Jim using case insensitive") {
        REQUIRE (bb.find_nocase("jim") == 0);
        REQUIRE (bb.find_nocase("Jim") == 0);
        REQUIRE (bb.find_nocase("Jimx") == -1);   // negative case
    }

}

TEST_CASE("StringPool various find cases when multiple pages","[stringpool]")
{
    StringPool bb = StringPool::split(' ', 0, "jim john mary frank maya julia matthew david greg colin kinga", 16);
    REQUIRE ( bb.count() == 11);

    SECTION("can find julia") {
        REQUIRE (bb.find("julia") == 5);
        REQUIRE (bb.find("Julia") == -1);
        REQUIRE (bb.find_nocase("juliax") == -1);   // negative case
    }
    SECTION("can find Julia using case insensitive") {
        REQUIRE (bb.find_nocase("julia") == 5);
        REQUIRE (bb.find_nocase("Julia") == 5);
        REQUIRE (bb.find_nocase("Juliax") == -1);   // negative case
    }

    SECTION("can find kinga using range") {
        REQUIRE (bb.find("kinga mackenzie", 5) == 10);
        REQUIRE (bb.find("Kinga MacKenzie", 5) == -1);          // negative case
        REQUIRE (bb.find_nocase("kinga mackenzie", 6) == -1);   // negative case
    }
    SECTION("can find Kinga using case insensitive") {
        REQUIRE (bb.find_nocase("kinga mackenzie", 5) == 10);
        REQUIRE (bb.find_nocase("Kinga MacKenzie", 5) == 10);
        REQUIRE (bb.find_nocase("kinga mackenzie", 6) == -1);   // negative case
    }

    SECTION("can find jim") {
        REQUIRE (bb.find("jim") == 0);
        REQUIRE (bb.find("Jim") == -1);
        REQUIRE (bb.find_nocase("jimx") == -1);   // negative case
    }
    SECTION("can find Jim using case insensitive") {
        REQUIRE (bb.find_nocase("jim") == 0);
        REQUIRE (bb.find_nocase("Jim") == 0);
        REQUIRE (bb.find_nocase("Jimx") == -1);   // negative case
    }

}

#if 0

long split_string_verify(binbag* bb, int sep, const char* str) {
    char word[512];
    const char* p = str;
    long total = 0;

    // grab a word from the input each iteration
    while(NULL != (p = split_string(',', p, word, sizeof(word))) ) {
        long idx = binbag_find_case(bb, word);
        if (idx < 0)
            return idx;
        total++;
    }
    return total;
}

TEST_CASE("binbag_api_sample_using_growth_1000","[stringpool]")
{
    ssize_t added;
    size_t bbcount;
    binbag* bb = binbag_create(1000, 1.5);
    if((added=split_string_add(bb, ',', SAMPLE_API_WORDS)) <1)
        return FAIL;

    // validate counts
    bbcount = binbag_count(bb);
    if (added!=bbcount || bbcount!=68)
        return FAIL;

    if(split_string_verify(bb, ',', SAMPLE_API_WORDS) <1) {
        binbag_debug_print(bb);
        return FAIL;
    }

    binbag_free(bb);
    return OK;
}

TEST_CASE("binbag_api_sample_pack_mem","[stringpool]")
{
    ssize_t added;
    size_t bbcount;
    binbag* bb = binbag_create(1000, 1.5);
    if((added=split_string_add(bb, ',', SAMPLE_API_WORDS)) <1)
        return FAIL;

    // validate counts
    bbcount = binbag_count(bb);
    if (added!=bbcount || bbcount!=68)
        return FAIL;

    binbag_resize(bb, 0); // pack memory
    size_t capacity = binbag_free_space(bb);

    if(split_string_verify(bb, ',', SAMPLE_API_WORDS) <1) {
        binbag_debug_print(bb);
        return FAIL;
    }

    binbag_free(bb);
    return (capacity==0) ? OK : FAIL;
}


TEST_CASE("binbag_sort_names","[stringpool]")
{
    const char* str = "jim,john,mary,frank,maya,julia,matthew,david,greg,colin,kinga";
    binbag* bb = binbag_split_string(',', SF_IGNORE_EMPTY, str);
    if((bb==NULL) || binbag_count(bb)!=11)
        return FAIL;
    binbag* sorted = binbag_sort(bb, binbag_element_sort_asc);
    binbag_free(bb);
    if( strcmp(binbag_get(sorted, 0), "colin")!=0 ||
        strcmp(binbag_get(sorted, 1), "david")!=0 ||
        strcmp(binbag_get(sorted, 2), "frank")!=0 ||
        strcmp(binbag_get(sorted, 3), "greg")!=0 ||
        strcmp(binbag_get(sorted, 4), "jim")!=0 ||
        strcmp(binbag_get(sorted, 5), "john")!=0 ||
        strcmp(binbag_get(sorted, 6), "julia")!=0 ||
        strcmp(binbag_get(sorted, 7), "kinga")!=0 ||
        strcmp(binbag_get(sorted, 8), "mary")!=0 ||
        strcmp(binbag_get(sorted, 9), "matthew")!=0 ||
        strcmp(binbag_get(sorted, 10), "maya")!=0)
        return FAIL;
    return ((sorted!=NULL) && binbag_count(sorted)==11) ? OK : FAIL;
}

TEST_CASE("binbag_sort_with_duplicates","[stringpool]")
{
    const char* str = "john,jim,john,colin,mary,frank,maya,julia,matthew,david,maya,greg,colin,kinga,john";
    binbag* bb = binbag_split_string(',', SF_IGNORE_EMPTY, str);
    if((bb==NULL) || binbag_count(bb)!=15)
        return FAIL;
    binbag* sorted = binbag_sort(bb, binbag_element_sort_asc);
    binbag_free(bb);
    if( strcmp(binbag_get(sorted, 0), "colin")!=0 ||
        strcmp(binbag_get(sorted, 1), "colin")!=0 ||
        strcmp(binbag_get(sorted, 2), "david")!=0 ||
        strcmp(binbag_get(sorted, 3), "frank")!=0 ||
        strcmp(binbag_get(sorted, 4), "greg")!=0 ||
        strcmp(binbag_get(sorted, 5), "jim")!=0 ||
        strcmp(binbag_get(sorted, 6), "john")!=0 ||
        strcmp(binbag_get(sorted, 7), "john")!=0 ||
        strcmp(binbag_get(sorted, 8), "john")!=0 ||
        strcmp(binbag_get(sorted, 9), "julia")!=0 ||
        strcmp(binbag_get(sorted, 10), "kinga")!=0 ||
        strcmp(binbag_get(sorted, 11), "mary")!=0 ||
        strcmp(binbag_get(sorted, 12), "matthew")!=0 ||
        strcmp(binbag_get(sorted, 13), "maya")!=0 ||
        strcmp(binbag_get(sorted, 14), "maya")!=0)
        return FAIL;
    return ((sorted!=NULL) && binbag_count(sorted)==15) ? OK : FAIL;
}

TEST_CASE("binbag_reverse_3","[stringpool]")
{
    const char* str = "jim,john,mary";
    binbag* bb = binbag_split_string(',', SF_IGNORE_EMPTY, str);
    if((bb==NULL) || binbag_count(bb)!=3) return FAIL;
    binbag_inplace_reverse(bb);
    if( strcmp(binbag_get(bb, 0), "mary")!=0 ||
        strcmp(binbag_get(bb, 1), "john")!=0 ||
        strcmp(binbag_get(bb, 2), "jim")!=0)
        return FAIL;
    return ((bb!=NULL) && binbag_count(bb)==3) ? OK : FAIL;
}

TEST_CASE("binbag_reverse_1","[stringpool]")
{
    const char* str = "mary";
    binbag* bb = binbag_split_string(',', SF_IGNORE_EMPTY, str);
    if((bb==NULL) || binbag_count(bb)!=1) return FAIL;
    binbag_inplace_reverse(bb);
    if( strcmp(binbag_get(bb, 0), "mary")!=0)
        return FAIL;
    return ((bb!=NULL) && binbag_count(bb)==1) ? OK : FAIL;
}

TEST_CASE("binbag_reverse_2","[stringpool]")
{
    const char* str = "mary,jane";
    binbag* bb = binbag_split_string(',', SF_IGNORE_EMPTY, str);
    if((bb==NULL) || binbag_count(bb)!=2) return FAIL;
    binbag_inplace_reverse(bb);
    if( strcmp(binbag_get(bb, 0), "jane")!=0 || strcmp(binbag_get(bb, 1), "mary")!=0)
        return FAIL;
    return ((bb!=NULL) && binbag_count(bb)==2) ? OK : FAIL;
}

TEST_CASE("binbag_reverse_11","[stringpool]")
{
    const char* str = "jim,john,mary,frank,maya,julia,matthew,david,greg,colin,kinga";
    binbag* bb = binbag_split_string(',', SF_IGNORE_EMPTY, str);
    if((bb==NULL) || binbag_count(bb)!=11) return FAIL;
    binbag_inplace_reverse(bb);
    if( strcmp(binbag_get(bb, 0), "kinga")!=0 ||
        strcmp(binbag_get(bb, 1), "colin")!=0 ||
        strcmp(binbag_get(bb, 2), "greg")!=0 ||
        strcmp(binbag_get(bb, 3), "david")!=0 ||
        strcmp(binbag_get(bb, 4), "matthew")!=0 ||
        strcmp(binbag_get(bb, 5), "julia")!=0 ||
        strcmp(binbag_get(bb, 6), "maya")!=0 ||
        strcmp(binbag_get(bb, 7), "frank")!=0 ||
        strcmp(binbag_get(bb, 8), "mary")!=0 ||
        strcmp(binbag_get(bb, 9), "john")!=0 ||
        strcmp(binbag_get(bb, 10), "jim")!=0)
        return FAIL;
    return ((bb!=NULL) && binbag_count(bb)==11) ? OK : FAIL;
}
#endif
