#include "./Period.h"

namespace ml
{
    int64_t Period::duration() const
    {
        return Date(_end - _start).time();
    }
    bool Period::operator==(const Period& other) const 
    {
        return _start == other._start && _end == other._end;
    }

    bool Period::operator!=(const Period& other) const 
    {
        return !(*this == other);
    }

    bool Period::operator<(const Period& other) const 
    {
        return _end < other._end;
    }

    bool Period::operator>(const Period& other) const 
    {
        return _start > other._start;
    }

    bool Period::operator<=(const Period& other) const 
    {
        return _end <= other._end;
    }

    bool Period::operator>=(const Period& other) const 
    {
        return _start >= other._start;
    }

    json Period::serialize() const
    {
        json _r = {};
        _r["start"] = _start.serialize();
        _r["end"] = _end.serialize();
        return _r;
    }

    void Period::deserialize(const json& _j)
    {
        if (_j.contains("start"))
            _start.deserialize(_j["start"]);

        if (_j.contains("end"))
        _end.deserialize(_j["end"]);
    }
}
