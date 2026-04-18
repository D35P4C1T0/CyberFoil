/*
Copyright (c) 2017-2018 Adubbz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <array>
#include <string>
#include <string_view>

#include "nx/ncm.hpp"
#include "util/title_util.hpp"

namespace tin::install
{
    struct ContentFileInfo
    {
        bool is_nca = false;
        bool is_cnmt = false;
        bool is_ticket = false;
        bool is_cert = false;
        bool has_nca_id = false;
        NcmContentId nca_id{};
        std::string pair_base_name;
    };

    inline bool HasSuffix(const std::string_view value, const std::string_view suffix)
    {
        return value.size() >= suffix.size() &&
            value.substr(value.size() - suffix.size()) == suffix;
    }

    inline ContentFileInfo ClassifyContentFile(const std::string_view name)
    {
        ContentFileInfo info;
        info.is_cnmt = HasSuffix(name, ".cnmt.nca") || HasSuffix(name, ".cnmt.ncz");
        info.is_nca = info.is_cnmt || HasSuffix(name, ".nca") || HasSuffix(name, ".ncz");
        info.is_ticket = HasSuffix(name, ".tik");
        info.is_cert = HasSuffix(name, ".cert");

        if (info.is_ticket)
            info.pair_base_name = std::string(name.substr(0, name.size() - 4));
        else if (info.is_cert)
            info.pair_base_name = std::string(name.substr(0, name.size() - 5));

        if (info.is_nca && name.size() >= 32)
        {
            info.nca_id = tin::util::GetNcaIdFromString(std::string(name.substr(0, 32)));
            info.has_nca_id = true;
        }

        return info;
    }

    inline std::array<std::string, 4> GetContentFileNameCandidates(const NcmContentId& ncaId)
    {
        const std::string ncaIdStr = tin::util::GetNcaIdString(ncaId);
        return {
            ncaIdStr + ".nca",
            ncaIdStr + ".cnmt.nca",
            ncaIdStr + ".ncz",
            ncaIdStr + ".cnmt.ncz",
        };
    }
}
