#pragma once
#include <Common.h>
#include <Resource.h>

namespace GW2Clarity
{
struct StackedBuff
{
    unsigned int id;
    int          count;
};

union Id
{
    int id;
    struct
    {
        short grid;
        short item;
    };

    template<std::integral T1, std::integral T2>
    constexpr Id(T1 g, T2 i)
        : grid(short(g))
        , item(short(i))
    {}

    constexpr bool operator==(Id other) const
    {
        return id == other.id;
    }
    constexpr bool operator!=(Id other) const
    {
        return id != other.id;
    }
};
static_assert(sizeof(Id) == sizeof(int));

static inline constexpr short UnselectedSubId = -1;

template<std::integral T = short>
static Id Unselected(T gid = T(UnselectedSubId))
{
    return { short(gid), UnselectedSubId };
}

} // namespace GW2Clarity