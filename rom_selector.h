#ifndef _22B2B909_1134_6471_AE6D_14EF3AF46BF0
#define _22B2B909_1134_6471_AE6D_14EF3AF46BF0

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "tar.h"

inline bool checkNESMagic(const uint8_t *data)
{
    return memcmp(data, "NES\x1a", 4) == 0;
}

class ROMSelector
{
    const uint8_t *singleROM_{};
    std::vector<TAREntry> entries_;

    int selectedIndex_ = 0;

public:
    void init(uintptr_t addr)
    {
        auto *p = reinterpret_cast<const uint8_t *>(addr);
        if (checkNESMagic(p))
        {
            singleROM_ = p;
            printf("Single ROM.\n");
            return;
        }

        entries_ = parseTAR(p, checkNESMagic);
        printf("%zd ROMs.\n", entries_.size());
        for (auto &e : entries_)
        {
            printf("  %s: %p, %zd\n", e.filename.data(), e.data, e.size);
        }
    }

    const uint8_t *getCurrentROM() const
    {
        if (singleROM_)
        {
            return singleROM_;
        }
        if (!entries_.empty())
        {
            return entries_[selectedIndex_].data;
        }
        return {};
    }

    void next()
    {
        if (singleROM_ || entries_.empty())
        {
            return;
        }
        ++selectedIndex_;
        if (selectedIndex_ == static_cast<int>(entries_.size()))
        {
            selectedIndex_ = 0;
        }
    }

    void prev()
    {
        if (singleROM_ || entries_.empty())
        {
            return;
        }
        --selectedIndex_;
        if (selectedIndex_ < 0)
        {
            selectedIndex_ = static_cast<int>(entries_.size() - 1);
        }
    }
};

#endif /* _22B2B909_1134_6471_AE6D_14EF3AF46BF0 */
