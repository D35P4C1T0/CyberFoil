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

#include <string>
#include <vector>

#include "install/package_install_helper.hpp"

namespace tin::install::stream
{
    struct Hfs0Header {
        u32 magic;
        u32 total_files;
        u32 string_table_size;
        u32 padding;
    };

    struct Hfs0FileTableEntry {
        u64 data_offset;
        u64 data_size;
        u32 name_offset;
        u32 hash_size;
        u64 padding;
        u8 hash[0x20];
    };

    struct Hfs0Partition {
        Hfs0Header header{};
        std::vector<Hfs0FileTableEntry> file_table{};
        std::vector<std::string> string_table{};
        s64 data_offset{};
    };

    template <typename Source>
    bool ReadHfs0Partition(Source& source, s64 off, Hfs0Partition& out)
    {
        u64 bytes_read = 0;
        if (R_FAILED(source.Read(&out.header, off, sizeof(out.header), &bytes_read))) return false;
        if (out.header.magic != 0x30534648) return false;
        if (out.header.total_files == 0 || out.header.total_files > 0x4000) return false;
        if (out.header.string_table_size > (256 * 1024)) return false;
        off += bytes_read;

        out.file_table.resize(out.header.total_files);
        const auto file_table_size = static_cast<s64>(out.file_table.size() * sizeof(Hfs0FileTableEntry));
        if (file_table_size > (4 * 1024 * 1024)) return false;
        if (R_FAILED(source.Read(out.file_table.data(), off, file_table_size, &bytes_read))) return false;
        off += bytes_read;

        std::vector<char> string_table(out.header.string_table_size);
        if (R_FAILED(source.Read(string_table.data(), off, string_table.size(), &bytes_read))) return false;
        off += bytes_read;

        out.string_table.clear();
        out.string_table.reserve(out.header.total_files);
        for (u32 i = 0; i < out.header.total_files; i++) {
            out.string_table.emplace_back(string_table.data() + out.file_table[i].name_offset);
        }

        out.data_offset = off;
        return true;
    }

    template <typename Source>
    bool GetXciSecureCollections(Source& source, std::vector<PackageCollectionEntry>& out)
    {
        Hfs0Partition root{};
        s64 root_offset = 0xF000;
        if (!ReadHfs0Partition(source, root_offset, root)) {
            root_offset = 0x10000;
            if (!ReadHfs0Partition(source, root_offset, root)) {
                return false;
            }
        }

        for (u32 i = 0; i < root.header.total_files; i++) {
            if (root.string_table[i] != "secure") continue;

            Hfs0Partition secure{};
            const auto secure_offset = root.data_offset + static_cast<s64>(root.file_table[i].data_offset);
            if (!ReadHfs0Partition(source, secure_offset, secure)) return false;

            out.clear();
            out.reserve(secure.header.total_files);
            for (u32 j = 0; j < secure.header.total_files; j++) {
                PackageCollectionEntry entry;
                entry.fileName = secure.string_table[j];
                entry.dataOffset = static_cast<u64>(secure.data_offset + static_cast<s64>(secure.file_table[j].data_offset));
                entry.fileSize = secure.file_table[j].data_size;
                entry.contentInfo = ClassifyContentFile(entry.fileName);
                out.emplace_back(std::move(entry));
            }
            return true;
        }

        return false;
    }
}
