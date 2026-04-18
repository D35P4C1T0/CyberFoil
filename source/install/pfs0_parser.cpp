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

#include "install/pfs0_parser.hpp"

#include "install/content_file.hpp"
#include "util/error.hpp"
#include "util/title_util.hpp"

namespace tin::install
{
    namespace
    {
        constexpr u32 PFS0Magic = 0x30534650;

        const PFS0BaseHeader* RequireBaseHeader(const std::vector<u8>& headerBytes)
        {
            if (headerBytes.size() < sizeof(PFS0BaseHeader))
                THROW_FORMAT("Cannot retrieve header as header bytes are empty. Have you retrieved it yet?\n");

            const auto* header = reinterpret_cast<const PFS0BaseHeader*>(headerBytes.data());
            if (header->magic != PFS0Magic)
                THROW_FORMAT("Invalid PFS0 magic");

            return header;
        }

        size_t GetStringTableStart(const PFS0BaseHeader* header)
        {
            return sizeof(PFS0BaseHeader) + static_cast<size_t>(header->numFiles) * sizeof(PFS0FileEntry);
        }
    }

    void PFS0Parser::RetrieveHeader(const std::function<void(void* buf, off_t offset, size_t size)>& bufferData)
    {
        m_headerBytes.resize(sizeof(PFS0BaseHeader), 0);
        bufferData(m_headerBytes.data(), 0x0, sizeof(PFS0BaseHeader));

        const size_t headerSize = this->GetExpectedHeaderSize();
        const size_t remainingHeaderSize = headerSize - sizeof(PFS0BaseHeader);

        m_headerBytes.resize(headerSize, 0);
        bufferData(m_headerBytes.data() + sizeof(PFS0BaseHeader), sizeof(PFS0BaseHeader), remainingHeaderSize);
    }

    bool PFS0Parser::TryLoadHeader(std::vector<u8> headerBytes)
    {
        if (headerBytes.size() < sizeof(PFS0BaseHeader))
            return false;

        const auto* header = RequireBaseHeader(headerBytes);
        const size_t headerSize = sizeof(PFS0BaseHeader) +
            static_cast<size_t>(header->numFiles) * sizeof(PFS0FileEntry) +
            header->stringTableSize;

        if (headerBytes.size() < headerSize)
            return false;

        headerBytes.resize(headerSize);
        m_headerBytes = std::move(headerBytes);
        return true;
    }

    const PFS0BaseHeader* PFS0Parser::GetBaseHeader() const
    {
        return RequireBaseHeader(m_headerBytes);
    }

    u64 PFS0Parser::GetDataOffset() const
    {
        if (m_headerBytes.empty())
            THROW_FORMAT("Cannot get data offset as header is empty. Have you retrieved it yet?\n");

        return m_headerBytes.size();
    }

    const PFS0FileEntry* PFS0Parser::GetFileEntry(unsigned int index) const
    {
        const auto* header = this->GetBaseHeader();
        if (index >= header->numFiles)
            THROW_FORMAT("File entry index is out of bounds\n");

        const size_t fileEntryOffset = sizeof(PFS0BaseHeader) + static_cast<size_t>(index) * sizeof(PFS0FileEntry);
        if (m_headerBytes.size() < fileEntryOffset + sizeof(PFS0FileEntry))
            THROW_FORMAT("Header bytes is too small to get file entry!");

        return reinterpret_cast<const PFS0FileEntry*>(m_headerBytes.data() + fileEntryOffset);
    }

    const PFS0FileEntry* PFS0Parser::GetFileEntryByName(const std::string& name) const
    {
        const auto* header = this->GetBaseHeader();
        for (unsigned int i = 0; i < header->numFiles; i++)
        {
            const auto* fileEntry = this->GetFileEntry(i);
            const std::string foundName(this->GetFileEntryName(fileEntry));
            if (foundName == name)
                return fileEntry;
        }

        return nullptr;
    }

    const PFS0FileEntry* PFS0Parser::GetFileEntryByNcaId(const NcmContentId& ncaId) const
    {
        for (const auto& candidate : tin::install::GetContentFileNameCandidates(ncaId))
        {
            if (const auto* fileEntry = this->GetFileEntryByName(candidate))
                return fileEntry;
        }

        return nullptr;
    }

    std::vector<const PFS0FileEntry*> PFS0Parser::GetFileEntriesByExtension(const std::string& extension) const
    {
        std::vector<const PFS0FileEntry*> entryList;
        const auto* header = this->GetBaseHeader();

        for (unsigned int i = 0; i < header->numFiles; i++)
        {
            const auto* fileEntry = this->GetFileEntry(i);
            const std::string name(this->GetFileEntryName(fileEntry));
            const auto separator = name.find('.');
            if (separator == std::string::npos)
                continue;

            const auto foundExtension = name.substr(separator + 1);
            if (foundExtension == extension)
                entryList.push_back(fileEntry);
        }

        return entryList;
    }

    const char* PFS0Parser::GetFileEntryName(const PFS0FileEntry* fileEntry) const
    {
        const auto* header = this->GetBaseHeader();
        const size_t stringTableStart = GetStringTableStart(header);
        const size_t nameOffset = stringTableStart + fileEntry->stringTableOffset;

        if (m_headerBytes.size() <= nameOffset)
            THROW_FORMAT("Header bytes is too small to get file name!");

        return reinterpret_cast<const char*>(m_headerBytes.data() + nameOffset);
    }

    size_t PFS0Parser::GetExpectedHeaderSize() const
    {
        const auto* header = this->GetBaseHeader();
        return sizeof(PFS0BaseHeader) +
            static_cast<size_t>(header->numFiles) * sizeof(PFS0FileEntry) +
            header->stringTableSize;
    }
}
