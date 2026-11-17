#pragma once

namespace gg
{

/** Pages implement this so `--screenshot <dir>` can capture each of their states headlessly. */
struct Shootable
{
    virtual ~Shootable() = default;
    virtual int numShots() const = 0;
    virtual void prepareShot (int index) = 0;
};

} // namespace gg
