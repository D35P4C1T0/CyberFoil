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

#include "install/nsp.hpp"

#include <threads.h>
#include "data/buffered_placeholder_writer.hpp"
#include "util/title_util.hpp"
#include "util/error.hpp"
#include "util/debug.h"

namespace tin::install::nsp
{
    NSP::NSP() {}

    void NSP::RetrieveHeader()
    {
        LOG_DEBUG("Retrieving remote NSP header...\n");
        m_header.RetrieveHeader([this](void* buf, off_t offset, size_t size) {
            this->BufferData(buf, offset, size);
        });

        LOG_DEBUG("Full header: \n");
        printBytes(reinterpret_cast<u8*>(const_cast<PFS0BaseHeader*>(m_header.GetBaseHeader())), m_header.GetDataOffset(), true);
    }

    const PFS0FileEntry* NSP::GetFileEntry(unsigned int index)
    {
        return m_header.GetFileEntry(index);
    }

    std::vector<const PFS0FileEntry*> NSP::GetFileEntriesByExtension(std::string extension)
    {
        return m_header.GetFileEntriesByExtension(extension);
    }

    const PFS0FileEntry* NSP::GetFileEntryByName(std::string name)
    {
        return m_header.GetFileEntryByName(name);
    }

    const PFS0FileEntry* NSP::GetFileEntryByNcaId(const NcmContentId& ncaId)
    {
        return m_header.GetFileEntryByNcaId(ncaId);
    }

    const char* NSP::GetFileEntryName(const PFS0FileEntry* fileEntry)
    {
        return m_header.GetFileEntryName(fileEntry);
    }

    const PFS0BaseHeader* NSP::GetBaseHeader()
    {
        return m_header.GetBaseHeader();
    }

    u64 NSP::GetDataOffset()
    {
        return m_header.GetDataOffset();
    }
}
