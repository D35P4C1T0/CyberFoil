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

#include <vector>

#include "install/install.hpp"
#include "install/package_install_helper.hpp"

namespace tin::install
{
    class CollectionInstall : public Install
    {
        public:
            CollectionInstall(
                NcmStorageId destStorageId,
                bool ignoreReqFirmVersion,
                std::vector<PackageCollectionEntry> collections,
                BufferDataFunction bufferData,
                StreamCollectionToPlaceholderFunction streamToPlaceholder);

        protected:
            std::vector<std::tuple<nx::ncm::ContentMeta, NcmContentInfo>> ReadCNMT() override;
            void InstallTicketCert() override;
            void InstallNCA(const NcmContentId& ncaId) override;

            const PackageCollectionEntry* GetCollectionEntryByNcaId(const NcmContentId& ncaId) const;

        private:
            std::vector<PackageCollectionEntry> m_collections;
            BufferDataFunction m_bufferData;
            StreamCollectionToPlaceholderFunction m_streamToPlaceholder;
    };
}
