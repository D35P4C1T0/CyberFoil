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

#include <functional>
#include <string>
#include <vector>

#include <sys/types.h>
#include <switch/types.h>

#include "install/pfs0.hpp"
#include "nx/ncm.hpp"

namespace tin::install
{
    class PFS0Parser
    {
        protected:
            std::vector<u8> m_headerBytes;

        public:
            void RetrieveHeader(const std::function<void(void* buf, off_t offset, size_t size)>& bufferData);
            bool TryLoadHeader(std::vector<u8> headerBytes);

            const PFS0BaseHeader* GetBaseHeader() const;
            u64 GetDataOffset() const;

            const PFS0FileEntry* GetFileEntry(unsigned int index) const;
            const PFS0FileEntry* GetFileEntryByName(const std::string& name) const;
            const PFS0FileEntry* GetFileEntryByNcaId(const NcmContentId& ncaId) const;
            std::vector<const PFS0FileEntry*> GetFileEntriesByExtension(const std::string& extension) const;

            const char* GetFileEntryName(const PFS0FileEntry* fileEntry) const;

        private:
            size_t GetExpectedHeaderSize() const;
    };
}
