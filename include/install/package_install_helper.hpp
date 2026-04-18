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
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "install/content_file.hpp"
#include "nx/ipc/tin_ipc.h"
#include "nx/ncm.hpp"
#include "util/file_util.hpp"
#include "util/error.hpp"

namespace tin::install
{
    struct PackageNcaEntryInfo
    {
        std::string fileName;
        u64 dataOffset = 0;
        u64 fileSize = 0;
    };

    void InstallNcaFromPackage(
        const NcmContentId& ncaId,
        const PackageNcaEntryInfo& entryInfo,
        NcmStorageId destStorageId,
        bool& declinedValidation,
        const std::function<void(void* buf, u64 offset, size_t size)>& bufferData,
        const std::function<void(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId)>& streamToPlaceholder,
        const std::function<void()>& afterStream = {});

    template<typename Package, typename InstallNcaFn>
    std::vector<std::tuple<nx::ncm::ContentMeta, NcmContentInfo>> ReadCnmtFromPackage(Package& package, NcmStorageId destStorageId, InstallNcaFn&& installNca)
    {
        std::vector<std::tuple<nx::ncm::ContentMeta, NcmContentInfo>> cnmtList;
        const std::string cnmtExtensions[] = { "cnmt.nca", "cnmt.ncz" };

        for (const auto& extension : cnmtExtensions)
        {
            for (const auto* fileEntry : package.GetFileEntriesByExtension(extension))
            {
                std::string cnmtNcaName(package.GetFileEntryName(fileEntry));
                NcmContentId cnmtContentId = tin::util::GetNcaIdFromString(cnmtNcaName);
                size_t cnmtNcaSize = fileEntry->fileSize;

                nx::ncm::ContentStorage contentStorage(destStorageId);

                LOG_DEBUG("CNMT Name: %s\n", cnmtNcaName.c_str());

                installNca(cnmtContentId);
                std::string cnmtNcaFullPath = contentStorage.GetPath(cnmtContentId);

                NcmContentInfo cnmtContentInfo{};
                cnmtContentInfo.content_id = cnmtContentId;
                ncmU64ToContentInfoSize(cnmtNcaSize & 0xFFFFFFFFFFFF, &cnmtContentInfo);
                cnmtContentInfo.content_type = NcmContentType_Meta;

                cnmtList.push_back({ tin::util::GetContentMetaFromNCA(cnmtNcaFullPath), cnmtContentInfo });
            }
        }

        return cnmtList;
    }

    template<typename Package, typename BufferDataFn>
    void InstallTicketCertFromPackage(Package& package, BufferDataFn&& bufferData)
    {
        const auto tikFileEntries = package.GetFileEntriesByExtension("tik");
        const auto certFileEntries = package.GetFileEntriesByExtension("cert");

        using CertEntryPtr = std::remove_cvref_t<decltype(certFileEntries.front())>;
        std::unordered_map<std::string, CertEntryPtr> certEntriesByBase;
        certEntriesByBase.reserve(certFileEntries.size());
        for (const auto* certEntry : certFileEntries)
        {
            if (certEntry == nullptr)
                continue;

            const auto info = tin::install::ClassifyContentFile(package.GetFileEntryName(certEntry));
            if (!info.is_cert)
                continue;

            certEntriesByBase[info.pair_base_name] = certEntry;
        }

        for (const auto* tikEntry : tikFileEntries)
        {
            if (tikEntry == nullptr)
            {
                LOG_DEBUG("Remote tik file is missing.\n");
                THROW_FORMAT("Remote tik file is not present!");
            }

            const auto tikInfo = tin::install::ClassifyContentFile(package.GetFileEntryName(tikEntry));
            if (!tikInfo.is_ticket)
                THROW_FORMAT("Remote tik file is not present!");

            const auto certIt = certEntriesByBase.find(tikInfo.pair_base_name);
            if (certIt == certEntriesByBase.end() || certIt->second == nullptr)
            {
                LOG_DEBUG("Remote cert file is missing.\n");
                THROW_FORMAT("Remote cert file is not present!");
            }

            const u64 tikSize = tikEntry->fileSize;
            auto tikBuf = std::make_unique<u8[]>(tikSize);
            LOG_DEBUG("> Reading tik\n");
            bufferData(tikBuf.get(), tikEntry->dataOffset, tikSize);

            const u64 certSize = certIt->second->fileSize;
            auto certBuf = std::make_unique<u8[]>(certSize);
            LOG_DEBUG("> Reading cert\n");
            bufferData(certBuf.get(), certIt->second->dataOffset, certSize);

            ASSERT_OK(esImportTicket(tikBuf.get(), tikSize, certBuf.get(), certSize), "Failed to import ticket");
        }
    }
}
