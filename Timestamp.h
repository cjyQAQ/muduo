# pragma once

#include <iostream>
#include <string>



class Timestamp
{
private:
    int64_t m_microSeconds;
public:
    Timestamp();
    explicit Timestamp(int64_t microSeconds);
    static Timestamp now();
    std::string toString() const;
    ~Timestamp();
};

