//
// Created by itsmateusz on 20.09.24.
//

#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <vector>
#include <set>
#include <ctime>
#include "./include/currency_network.hpp"

struct ProgramInput {
    set<string> src_currencies;
    double src_currency_amount;
    string dest_currency;
    bool use_proxy;

    ProgramInput() : src_currency_amount(0.0), use_proxy(false) {}
};

void parse_input_into(int argc, char *args[], ProgramInput *input) {
    const string src_currency_flag = "-s";
    const string dest_currency_flag = "-d";
    const string amount_flag = "-a";
    const string proxy_flag = "-p";
    const int EQUAL(0);
    for (int iarg = 1; iarg < argc; ++iarg) {
        if (src_currency_flag.compare(args[iarg]) == EQUAL) {
            for (int jarg = ++iarg; jarg < argc; ++jarg) {
                if (amount_flag.compare(args[jarg]) == EQUAL || dest_currency_flag.compare(args[jarg]) == EQUAL ||
                    proxy_flag.compare(args[jarg]) == EQUAL) {
                    break;
                }
                input->src_currencies.insert(args[jarg]);
            }
        } else if (amount_flag.compare(args[iarg]) == EQUAL && input->src_currency_amount == 0.0 && ++iarg < argc) {
            input->src_currency_amount = stod(args[iarg]);
        } else if (dest_currency_flag.compare(args[iarg]) == EQUAL && input->dest_currency.empty() && ++iarg < argc) {
            input->dest_currency = args[iarg];
        } else if (proxy_flag.compare(args[iarg]) == EQUAL) {
            input->use_proxy = true;
        }
    }
}

int main(int argc, char *args[]) {

    ProgramInput prog_input;
    parse_input_into(argc, args, &prog_input);

    CurrencyConversionNetwork *cconversion_network = new CurrencyConversionNetwork(prog_input.use_proxy);

    vector<CurrencyConversion> cconversions;
    for_each(prog_input.src_currencies.cbegin(), prog_input.src_currencies.cend(),
             [&cconversions, cconversion_network, &prog_input](const string &currency) {
                 Currency src_currency = cconversion_network->find_currency(currency);
                 Currency dest_currency = cconversion_network->find_currency(prog_input.dest_currency);
                 cconversions.emplace_back(src_currency, dest_currency);
                 cconversion_network->load_conversion(cconversions.back());
             });


    for_each(cconversions.cbegin(), cconversions.cend(), [&prog_input](const CurrencyConversion &cconversion) {
        cconversion.print_calculation(cout, prog_input.src_currency_amount);
    });
}
