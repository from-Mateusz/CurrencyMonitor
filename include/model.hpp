//
// Created by itsmateusz on 21.09.24.
//

#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include <iostream>

struct Currency {
    std::string code;
    std::set<std::string> countries;

    Currency() = default;

    Currency(std::string _code, std::set<std::string> _countries) : code(_code), countries(_countries) {}

    Currency(std::string _code, std::string _country) : code(_code) {
        countries.insert(_country);
    }

    void add_country(const std::string &country) {
        countries.insert(country);
    }
};

struct CurrencyConversion {
    Currency src;
    Currency dest;
    double rate;
    std::string readout;
    int tm_mday;
    int tm_mon;
    int tm_year;

    CurrencyConversion() = default;

    CurrencyConversion(Currency &_src, Currency &_dest) {
        if (_src.code == _dest.code)
            throw std::invalid_argument(
                    "There is no sense in creating conversion with both same source and destination currency");

        src = _src;
        dest = _dest;
    }

    void print_calculation(std::ostream &out, double amount) const {
        std::string src_countries(""), dest_countries("");
        for (std::string country: src.countries) {
            src_countries += country + "; ";
        }
        for (std::string country: dest.countries) {
            dest_countries += country + "; ";
        }
        out << "===============================" << "\n";
        out << "==== " << src.code << " -> " << dest.code << "\n";
        out << "===============================" << "\n";
        out << "src currency countries: " << src_countries << "\n";
        out << "dest currency countries: " << dest_countries << "\n";
        out << "-------------------------------\n";
        out << "rate: " << rate << "\n";
        out << "-------------------------------\n";
        out << "readout: " << readout << "\n";
        out << "value [" << src.code << "]: " << amount << "\n";
        out << "value [" << dest.code << "]: " << (amount * rate) << "\n";
        out << "===============================" << "\n\n";
    }

    std::string get_src_currency_code() const {
        return src.code;
    }

    std::string get_dest_currency_code() const {
        return dest.code;
    }
};

#endif
