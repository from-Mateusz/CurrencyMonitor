//
// Created by itsmateusz on 22.09.24.
//

#ifndef TRAINING_CURRENCY_NETWORK_HPP
#define TRAINING_CURRENCY_NETWORK_HPP

#define PROXIES_UNDER_250MS_LATENCY_API_URL "https://api.proxyscrape.com/v4/free-proxy-list/get?request=display_proxies&proxy_format=protocolipport&format=json&timeout=250"
#define PROXIES_FILE_LOCATION "./resources/proxies.json"
#define CONVERSION_FILE_LOCATION "./resources/conversions.json"
#define CURRENCIES_FILE_LOCATION "./resources/currencies.csv"
#define CURRENCY_CONVERSIONS_API_URL "https://api.exchangerate-api.com/v4/latest/"
#define WITH_PROXY_SERVER_RESPONSE_TIMEOUT_MS 500L

#include <curl/curl.h>
#include <string.h>
#include <fstream>
#include <type_traits>
#include <ctime>
#include <stdlib.h>
#include <algorithm>
#include <stdexcept>
#include <set>
#include "./json.hpp"
#include "./model.hpp"
#include "./utilities.hpp"

using namespace std;
using json = nlohmann::json;

class Proxifier;

class CurrencyConversionNetwork {
    friend class Proxifier;

public:
    CurrencyConversionNetwork() = default;

    explicit CurrencyConversionNetwork(bool use_proxy) : proxify(use_proxy) {}

    void load_conversion(CurrencyConversion &);

    Currency find_currency(const string &);

private:
    CURL *curl;
    std::map<std::string,Currency> currencies;
    bool proxify;

    bool should_download_conversion(const CurrencyConversion &);

    static size_t read_data(char *, size_t, size_t, void *);

    void download_conversion(CurrencyConversion &);

    void load_stored_conversion(CurrencyConversion &);

    void store_conversions(const json &);

    void store_conversion(const CurrencyConversion &);

    void update_conversion(const json &conversion_json, CurrencyConversion &cconversion);

    void load_currencies();
};

struct Proxy {
    std::string protocol;
    std::string ip;
    std::string port;
    std::string country;
    int in_use;
    bool dead;

    Proxy() = default;

    std::string get_address() const {
        return protocol + "://" + ip + ":" + port;
    }

    Proxy(std::string _protocol, std::string _ip, int _port, std::string _country) : protocol(_protocol), ip(_ip),
                                                                                     port(std::to_string(_port)),
                                                                                     country(_country) {}
};

class Proxifier {
public:
    Proxifier() = default;

    void proxify(CurrencyConversionNetwork *);

    void deem_proxy_dead();

    const Proxy &get_recently_used_proxy();

private:
    std::vector<Proxy> proxies;

    Proxy *recently_used_proxy;

    int count_alive_proxies();

    Proxy &get_next_proxy();

    static size_t store_proxies(char *ptr, size_t size, size_t nmemb, void *write_stream);

    void load_existing_proxies();

    void fetch_proxies();

    void date_proxies();

    bool proxies_file_exists();

    bool should_update_proxies();
};

#endif //TRAINING_CURRENCY_NETWORK_HPP
