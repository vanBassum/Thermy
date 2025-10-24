#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>

class SimpleStats
{
public:
    explicit SimpleStats(const char *name = "")
        : _name(name)
    {
    }

    void Reset()
    {
        _count = 0;
        _mean = 0;
        _m2 = 0;
        _min = 0;
        _max = 0;
        _last = 0;
    }

    void AddValue(double x)
    {
        if(x == 0)
            return;
        _last = x;

        if (_count == 0)
        {
            _min = _max = _mean = x;
            _m2 = 0;
            _count = 1;
            return;
        }

        _count++;
        if (x < _min) _min = x;
        if (x > _max) _max = x;

        double delta = x - _mean;
        _mean += delta / _count;
        _m2 += delta * (x - _mean);
    }

    // ---- Getters ----
    inline uint64_t Count() const { return _count; }
    inline double Min() const { return _min; }
    inline double Max() const { return _max; }
    inline double Avg() const { return _mean; }
    inline double Last() const { return _last; }
    inline double StdDev() const { return (_count > 0) ? std::sqrt(_m2 / _count) : 0.0; }
    inline const char *Name() const { return _name; }

    // ---- Print a single table row ----
    int PrintRow(char *buf, size_t len) const
    {
        return snprintf(buf, len,
                        "| %-10s | %8.2f | %8.2f | %8.2f | %8.2f | %8.2f |\n",
                        _name, Last(), Avg(), Min(), Max(), StdDev());
    }

    // ---- Static header ----
    static int PrintHeader(char *buf, size_t len)
    {
        return snprintf(buf, len,
                        "| Name       |   Last   |   Avg    |   Min    |   Max    |  StdDev  |\n"
                        "|------------|----------|----------|----------|----------|----------|\n");
    }

private:
    const char *_name;
    uint64_t _count = 0;
    double _mean = 0;
    double _m2 = 0;
    double _min = 0;
    double _max = 0;
    double _last = 0;
};
