/*
 * SPDX-FileCopyrightText: Copyright 2023 Denis Glazkov <glazzk.off@mail.ru>
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

#include "../qqsort.h"

namespace utils {

template <typename T>
T env(const char* name) {
    if (!std::getenv(name)) {
        std::cerr << "[error] environment variable not specified: " << name << std::endl;
        std::exit(1);
    }

    std::stringstream sstream(std::getenv(name));
    T result;
    sstream >> result;

    return result;
}

} /* namespace utils */

namespace generator {

    std::mt19937 engine(utils::env<int>("SEED"));
    std::uniform_int_distribution<unsigned int> age(18, 100);
    std::uniform_int_distribution<unsigned int> balance(100, 1000000);

} /* namespace generator */

class Person {
public:
    Person(unsigned int age, unsigned int balance)
        : age(age)
        , balance(balance)
    {}

    static Person generate() {
        return Person(generator::age(generator::engine), generator::balance(generator::engine));
    }

    bool operator<(const Person& other) const {
        return this->rating() < other.rating();
    }

    int rating() const {
        return this->balance / this->age;
    }

private:
    unsigned int age;
    unsigned int balance;
};

namespace sort {

using function = std::function<void(std::vector<Person>&)>;

} /* namespace sort */

namespace {

std::vector<Person> generatePersonsArray() {
    std::vector<Person> persons;
    size_t size = utils::env<size_t>("SIZE");

    persons.reserve(size);

    for (size_t idx = 0; idx < size; idx++) {
        persons.emplace_back(Person::generate());
    }

    return persons;
}

void cppSortPersonsArray(std::vector<Person>& persons) {
    std::sort(persons.begin(), persons.end());
}

int qSortPersonComparator(const void* a, const void* b) {
    const Person* first = static_cast<const Person*>(a);
    const Person* second = static_cast<const Person*>(b);

    return first->rating() - second->rating();
}

void qSortPersonsArray(std::vector<Person>& persons) {
    qsort(persons.data(),
          persons.size(),
          sizeof(Person),
          qSortPersonComparator);
}

void qqSortPersonsArray(std::vector<Person>& persons) {
    qqsort(persons.data(),
           persons.size(),
           sizeof(Person),
           qqsortcmp(Person *a, Person *b) {
                qqsortret(a->rating() - b->rating());
           });
}

void sortPersonsArray(const std::string &info, const sort::function &sort, std::vector<Person>& persons) {
    const auto begin = std::chrono::steady_clock::now();
    sort(persons);
    const auto end = std::chrono::steady_clock::now();

    std::cout << "[" << info << "] " << "estimated: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
              << " ms"
              << std::endl;
}

void checkPersonsArray(const std::vector<Person>& persons) {
    if (!std::is_sorted(persons.begin(), persons.end())) {
        std::cerr << "[error] persons array not sorted" << std::endl;
        std::exit(1);
    }
}

} /* namespace */

int main() {
    std::vector<std::pair<std::string, sort::function>> sorts = {
        {"cppsort", cppSortPersonsArray},
        {"qsort", qSortPersonsArray},
        {"qqsort", qqSortPersonsArray},
    };

    for (const auto& [info, sort] : sorts) {
        std::vector<Person> persons = generatePersonsArray();
        sortPersonsArray(info, sort, persons);
        checkPersonsArray(persons);
    }

    return 0;
}
