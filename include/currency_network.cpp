#include "./currency_network.hpp"

void Proxifier::proxify(CurrencyConversionNetwork *network) {
    bool should_fetch_proxies = !(proxies_file_exists()) || (proxies_file_exists() && should_update_proxies());
    if (should_fetch_proxies) {
        fetch_proxies();
        load_existing_proxies();
    } else {
        load_existing_proxies();
    }

    if (!(count_alive_proxies())) {
        fetch_proxies();
        load_existing_proxies();
    }

    Proxy &proxy = get_next_proxy();
    proxy.in_use = 1;

    curl_easy_setopt(network->curl, CURLOPT_PROXY, proxy.get_address());
    curl_easy_setopt(network->curl, CURLOPT_HTTPPROXYTUNNEL, 1L);
}

void Proxifier::deem_proxy_dead() {
    auto dead_proxy_iter = std::find_if(proxies.begin(), proxies.end(), [](const Proxy &proxy) {
        return proxy.in_use == 1;
    });
    (*dead_proxy_iter).dead = true;
    (*dead_proxy_iter).in_use = 0;
}

const Proxy &Proxifier::get_recently_used_proxy() {
    return *recently_used_proxy;
}

int Proxifier::count_alive_proxies() {
    return std::count_if(proxies.cbegin(), proxies.cend(), [](const Proxy &proxy) {
        return !proxy.dead;
    });
}

size_t Proxifier::store_proxies(char *ptr, size_t size, size_t nmemb, void *write_stream) {
    const int real_size = size * nmemb;
    ((std::ofstream *) write_stream)->write(ptr, real_size);
    return real_size;
}

void Proxifier::fetch_proxies() {
    if (curl_global_init(CURL_GLOBAL_SSL) == CURLE_OK) {
        CURL *curl = curl_easy_init();
        std::ofstream proxies_file(PROXIES_FILE_LOCATION);
        curl_easy_setopt(curl, CURLOPT_URL, PROXIES_UNDER_250MS_LATENCY_API_URL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Proxifier::store_proxies);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &proxies_file);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        date_proxies();
    }
}

void Proxifier::date_proxies() {
    using namespace std;
    using json = nlohmann::json;
    std::ifstream in_proxies_file(PROXIES_FILE_LOCATION);
    try {
        json proxies_json = json::parse(in_proxies_file);
        struct tm *current_time_components = current_time();
        proxies_json["tm_mday"] = current_time_components->tm_mday;
        proxies_json["tm_mon"] = current_time_components->tm_mon;
        proxies_json["tm_year"] = current_time_components->tm_year;
        in_proxies_file.close();

        std::ofstream out_proxies_file(PROXIES_FILE_LOCATION);
        out_proxies_file << proxies_json;
        out_proxies_file.close();
    } catch (...) {}
}

bool Proxifier::should_update_proxies() {
    using namespace std;
    using json = nlohmann::json;
    bool do_update = false;
    std::ifstream in_proxies_file(PROXIES_FILE_LOCATION);
    try {
        json proxies_json = json::parse(in_proxies_file);
        struct tm *current_time_components = current_time();
        do_update = proxies_json["tm_mday"].get<int>() == current_time_components->tm_mday &&
                    proxies_json["tm_mon"].get<int>() == current_time_components->tm_mon &&
                    proxies_json["tm_year"].get<int>() == current_time_components->tm_year;
    } catch (...) {}
    in_proxies_file.close();
    return do_update;
}

bool Proxifier::proxies_file_exists() {
    std::ifstream proxies_file(PROXIES_FILE_LOCATION);
    bool exists = (proxies_file.rdstate() & std::ifstream::failbit) == 0 &&
                  (proxies_file.rdstate() & std::ifstream::badbit) == 0 &&
                  proxies_file.peek() != std::char_traits<char>::eof();
    proxies_file.close();
    return exists;
}

void Proxifier::load_existing_proxies() {
    if (proxies.empty()) {
        using namespace std;
        using json = nlohmann::json;
        std::ifstream in_proxies_file(PROXIES_FILE_LOCATION);
        json proxies_json = json::parse(in_proxies_file);
        vector<json> proxies_in_json = proxies_json["proxies"].get<vector<json>>();
        for (auto proxies_it = proxies_in_json.cbegin(); proxies_it != proxies_in_json.cend(); ++proxies_it) {
            bool alive = (*proxies_it)["alive"].get<bool>();
            bool is_country_wise = (*proxies_it).find("ip_data") != (*proxies_it).cend();
            string country = (is_country_wise ? (*proxies_it)["ip_data"]["country"].get<string>() : "n/a");
            string protocol = (*proxies_it)["protocol"].get<string>();
            int port = (*proxies_it)["port"].get<int>();
            string ip = (*proxies_it)["ip"].get<string>();
            proxies.emplace_back(protocol, ip, port, country);
        }
        in_proxies_file.close();
    }
}

Proxy &Proxifier::get_next_proxy() {
    size_t i_next_proxy = 0;
    for (size_t i_proxy = 0; i_proxy < proxies.size(); ++i_proxy) {
        if (!proxies[i_proxy].dead) {
            i_next_proxy = i_proxy;
        }
    }
    recently_used_proxy = &proxies[i_next_proxy];
    return proxies[i_next_proxy];
}

bool CurrencyConversionNetwork::should_download_conversion(const CurrencyConversion &cconversion) {
    bool do_download = false;
    ifstream conversion_file(CONVERSION_FILE_LOCATION);
    try {
        json conversion_json = json::parse(conversion_file);
        string src_currency_code = cconversion.get_src_currency_code();
        string dest_currency_code = cconversion.get_dest_currency_code();
        const string conversion_pair = src_currency_code + "_" + dest_currency_code;
        struct tm *current_time_components = current_time();
        do_download = conversion_json.find(conversion_pair) == conversion_json.end() ||
                      (conversion_json[conversion_pair]["tm_mday"].get<int>() !=
                       current_time_components->tm_mday ||
                       conversion_json[conversion_pair]["tm_mon"].get<int>() !=
                       current_time_components->tm_mon ||
                       conversion_json[conversion_pair]["tm_year"].get<int>() !=
                       current_time_components->tm_year);
    }
    catch (...) {
        cout << "Conversion file is empty" << endl;
        do_download = true;
    }
    conversion_file.close();
    return do_download;
}

size_t CurrencyConversionNetwork::read_data(char *ptr, size_t size, size_t nmemb, void *read_buffer) {
    const int real_size = size * nmemb;
    ((std::string *) read_buffer)->append(ptr, real_size);
    return real_size;
}

void CurrencyConversionNetwork::download_conversion(CurrencyConversion &cconversion) {
    if (curl_global_init(CURL_GLOBAL_SSL) == CURLE_OK) {
        curl = curl_easy_init();
        std::string rawBody;
        const std::string url = CURRENCY_CONVERSIONS_API_URL + cconversion.get_src_currency_code();
        char converted_url[url.length() + 1];
        convert_str_to_array(url, converted_url);
        curl_easy_setopt(curl, CURLOPT_URL, converted_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, read_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rawBody);
        CURLcode res;
        if (proxify) {
            cout << "Be patient, because I am looking for the working proxy....";
            Proxifier proxifier;
            proxifier.proxify(this);
            while ((res = curl_easy_perform(curl)) != CURLE_OK) {
                cout << ".";
                proxifier.deem_proxy_dead();
                proxifier.proxify(this);
            }
            const Proxy &recently_used_proxy = proxifier.get_recently_used_proxy();
            cout << endl << "Found working proxy: " << recently_used_proxy.get_address() << endl << endl;
            json conversion_json = json::parse(rawBody);
            update_conversion(conversion_json, cconversion);
            store_conversions(conversion_json);
        } else if ((res = curl_easy_perform(curl)) == CURLE_OK) {
            json conversion_json = json::parse(rawBody);
            update_conversion(conversion_json, cconversion);
            store_conversions(conversion_json);
        }
        curl_easy_cleanup(curl);
        curl_global_cleanup();
    }
}

void CurrencyConversionNetwork::store_conversions(const json &conversion_json) {
    ifstream in_conversion_file(CONVERSION_FILE_LOCATION);
    json in_conversion_json;
    try {
        in_conversion_json = json::parse(in_conversion_file);
    } catch (...) {}

    struct tm *current_time_components = current_time();
    const string readout_rep = string(std::asctime(current_time_components));

    const Currency &src_currency = currencies[conversion_json["base"].get<string>()];
    const json rates = conversion_json["rates"].get<json>();

    for (pair<const string, Currency> &dest_currency: currencies) {
        string source_currency_code = src_currency.code;
        string dest_currency_code = dest_currency.second.code;
        json source_currency_countries = json::array();
        json dest_currency_countries = json::array();
        const string conversion_pair = source_currency_code + "_" + dest_currency_code;
        if (in_conversion_json.find(conversion_pair) == in_conversion_json.end()) {
            in_conversion_json[conversion_pair]["src"]["code"] = source_currency_code;
            for_each(src_currency.countries.cbegin(), src_currency.countries.cend(),
                     [&source_currency_countries](const string &country) {
                         source_currency_countries += country;
                     });
            in_conversion_json[conversion_pair]["src"]["countries"] = source_currency_countries;
            in_conversion_json[conversion_pair]["dest"]["code"] = dest_currency_code;
            for_each(dest_currency.second.countries.cbegin(), dest_currency.second.countries.cend(),
                     [&dest_currency_countries](const string &country) {
                         dest_currency_countries += country;
                     });
            in_conversion_json[conversion_pair]["dest"]["countries"] = dest_currency_countries;
        }
        in_conversion_json[conversion_pair]["rate"] =
                rates.find(dest_currency_code) != rates.end() ? rates[dest_currency_code].get<double>() : 0.0;
        in_conversion_json[conversion_pair]["readout"] = readout_rep.substr(0, readout_rep.length() - 1);
        in_conversion_json[conversion_pair]["tm_mday"] = current_time_components->tm_mday;
        in_conversion_json[conversion_pair]["tm_mon"] = current_time_components->tm_mon;
        in_conversion_json[conversion_pair]["tm_year"] = current_time_components->tm_year;
    }

    in_conversion_file.close();

    ofstream out_conversion_file(CONVERSION_FILE_LOCATION);
    out_conversion_file << in_conversion_json;
    out_conversion_file.close();
}

//void CurrencyConversionNetwork::store_conversion(const CurrencyConversion &cconversion) {
//    ifstream in_conversion_file(CONVERSION_FILE_LOCATION);
//    json in_conversion_json = json::parse(in_conversion_file);
//    string source_currency_code = cconversion.get_src_currency_code();
//    string dest_currency_code = cconversion.get_dest_currency_code();
//    const string conversion_pair = source_currency_code + "_" + dest_currency_code;
//    in_conversion_json[conversion_pair]["src"]["code"] = source_currency_code;
//    in_conversion_json[conversion_pair]["src"]["country"] = cconversion.get_src_currency_country();
//    in_conversion_json[conversion_pair]["dest"]["code"] = dest_currency_code;
//    in_conversion_json[conversion_pair]["dest"]["country"] = cconversion.get_dest_currency_country();
//    in_conversion_json[conversion_pair]["rate"] = cconversion.rate;
//    in_conversion_json[conversion_pair]["readout"] = cconversion.readout;
//    in_conversion_json[conversion_pair]["tm_mday"] = cconversion.tm_mday;
//    in_conversion_json[conversion_pair]["tm_mon"] = cconversion.tm_mon;
//    in_conversion_json[conversion_pair]["tm_year"] = cconversion.tm_year;
//    in_conversion_file.close();
//
//    ofstream out_conversion_file(CONVERSION_FILE_LOCATION);
//    out_conversion_file << in_conversion_json;
//    out_conversion_file.close();
//}

void CurrencyConversionNetwork::load_stored_conversion(CurrencyConversion &cconversion) {
    ifstream conversion_file(CONVERSION_FILE_LOCATION);
    try {
        json conversion_json = json::parse(conversion_file);
        string source_currency_code = cconversion.get_src_currency_code();
        string dest_currency_code = cconversion.get_dest_currency_code();
        const string conversion_pair = source_currency_code + "_" + dest_currency_code;
        struct tm *current_time_components = current_time();
        if (conversion_json.find(conversion_pair) != conversion_json.end()) {
            json src_currency_countries_json = conversion_json[conversion_pair]["src"]["countries"].get<json>();
            set<string> src_currency_countries(src_currency_countries_json.begin(), src_currency_countries_json.end());
            cconversion.src = Currency(conversion_json[conversion_pair]["src"]["code"].get<string>(),
                                       src_currency_countries);
            json dest_currency_countries_json = conversion_json[conversion_pair]["dest"]["countries"].get<json>();
            set<string> dest_currency_countries(dest_currency_countries_json.cbegin(),
                                                dest_currency_countries_json.cend());
            cconversion.dest = Currency(conversion_json[conversion_pair]["dest"]["code"].get<string>(),
                                        dest_currency_countries);
            cconversion.rate = conversion_json[conversion_pair]["rate"].get<double>();
            cconversion.readout = conversion_json[conversion_pair]["readout"].get<string>();
        }
    } catch (...) {
        cout << "Conversion file is empty" << endl;
    }
    conversion_file.close();
}

void CurrencyConversionNetwork::update_conversion(const json &conversion_json, CurrencyConversion &cconversion) {
    struct std::tm *current_time_components = current_time();
    const string readout_rep = string(std::asctime(current_time_components));

    string source_currency_code = cconversion.get_src_currency_code();
    string dest_currency_code = cconversion.get_dest_currency_code();
    string conversion_pair = source_currency_code + "_" + dest_currency_code;
    double rate = conversion_json["rates"][dest_currency_code].get<double>();
    cconversion.rate = rate;
    cconversion.readout = readout_rep.substr(0, readout_rep.length() - 1);
    cconversion.tm_mday = current_time_components->tm_mday;
    cconversion.tm_mon = current_time_components->tm_mon;
    cconversion.tm_year = current_time_components->tm_year;
}

void CurrencyConversionNetwork::load_currencies() {
    currencies.erase(currencies.begin(), currencies.end());
    int country_col = 0, currency_col = 1, code_col = 2, n_columns = 3;
    ifstream currencies_file(CURRENCIES_FILE_LOCATION);
    string currencies_data_entry;
    string currencies_data_col;
    int current_col = 0;

    string currency_code;
    string currency_country;
    while (!currencies_file.eof()) {
        if (getline(currencies_file, currencies_data_entry)) {
            istringstream entry(currencies_data_entry);
            while (!entry.eof()) {
                if (getline(entry, currencies_data_col, ',')) {
                    if (current_col % n_columns == country_col) {
                        currency_country = currencies_data_col;
                    } else if (current_col % n_columns == code_col) {
                        currency_code = currencies_data_col;
                    }
                }
                ++current_col;
            }
            if (!currency_code.empty() && !currency_country.empty()) {
                auto currency_iter = currencies.find(currency_code);
                if (currency_iter != currencies.end()) {
                    (*currency_iter).second.add_country(currency_country);
                } else {
                    currencies.insert(
                            std::pair<string, Currency>(currency_code, Currency(currency_code, currency_country)));
                }
            }
        }
    }
    currencies_file.close();
}

Currency CurrencyConversionNetwork::find_currency(const string &currency_code) {
    if (currencies.empty()) {
        load_currencies();
    }
    return currencies[currency_code];
}

void CurrencyConversionNetwork::load_conversion(CurrencyConversion &cconversion) {
    if (currencies.empty()) {
        load_currencies();
    }

    string unknown_currency = currencies.find(cconversion.get_src_currency_code()) != currencies.end()
                              ? cconversion.get_src_currency_code() :
                              currencies.find(cconversion.get_dest_currency_code()) != currencies.end()
                              ? cconversion.get_dest_currency_code() : "";

    if (unknown_currency.empty()) {
        throw std::runtime_error("Sorry, Currency code " + unknown_currency + " is unknown");
    }

    if (should_download_conversion(cconversion)) {
        download_conversion(cconversion);
    } else
        load_stored_conversion(cconversion);
}