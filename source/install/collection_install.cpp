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

#include "install/collection_install.hpp"

#include "util/error.hpp"
#include "util/title_util.hpp"

namespace tin::install
{
    CollectionInstall::CollectionInstall(
        NcmStorageId destStorageId,
        bool ignoreReqFirmVersion,
        std::vector<PackageCollectionEntry> collections,
        BufferDataFunction bufferData,
        StreamCollectionToPlaceholderFunction streamToPlaceholder)
        : Install(destStorageId, ignoreReqFirmVersion),
          m_collections(std::move(collections)),
          m_bufferData(std::move(bufferData)),
          m_streamToPlaceholder(std::move(streamToPlaceholder))
    {
    }

    const PackageCollectionEntry* CollectionInstall::GetCollectionEntryByNcaId(const NcmContentId& ncaId) const
    {
        return FindCollectionEntryByNcaId(m_collections, ncaId);
    }

    std::vector<std::tuple<nx::ncm::ContentMeta, NcmContentInfo>> CollectionInstall::ReadCNMT()
    {
        return ReadCnmtFromCollections(
            m_collections,
            m_destStorageId,
            [this](const PackageCollectionEntry&, const NcmContentId& ncaId) {
                this->InstallNCA(ncaId);
            });
    }

    void CollectionInstall::InstallTicketCert()
    {
        InstallTicketCertFromCollections(m_collections, m_bufferData);
    }

    void CollectionInstall::InstallNCA(const NcmContentId& ncaId)
    {
        const auto* entry = GetCollectionEntryByNcaId(ncaId);
        if (entry == nullptr)
            THROW_FORMAT(("Missing install entry for " + tin::util::GetNcaIdString(ncaId)).c_str());

        InstallNcaFromCollection(
            ncaId,
            *entry,
            m_destStorageId,
            m_declinedValidation,
            m_bufferData,
            m_streamToPlaceholder);
    }
}
