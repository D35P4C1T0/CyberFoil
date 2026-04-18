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

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "install/install.hpp"
#include "nx/ncm.hpp"
#include "util/file_util.hpp"
#include "util/title_util.hpp"

namespace tin::install
{
    class StreamingInstallHelper final : public Install
    {
        public:
            StreamingInstallHelper(NcmStorageId destStorageId, bool ignoreReqFirmVersion) :
                Install(destStorageId, ignoreReqFirmVersion) {}

            void AddContentMeta(const nx::ncm::ContentMeta& meta, const NcmContentInfo& info)
            {
                m_contentMeta.push_back(meta);
                m_cnmtInfos.push_back(info);
            }

            void CommitLatest()
            {
                if (m_contentMeta.empty())
                    return;

                this->CommitAtIndex(m_contentMeta.size() - 1);
            }

            void CommitAll()
            {
                for (size_t i = 0; i < m_contentMeta.size(); i++)
                    this->CommitAtIndex(i);
            }

            bool CommitInstalledCnmt(const std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, const NcmContentId& cnmtContentId, u64 cnmtSize, u64* outBaseTitleId = nullptr)
            {
                try
                {
                    std::string cnmtPath = contentStorage->GetPath(cnmtContentId);
                    nx::ncm::ContentMeta meta = tin::util::GetContentMetaFromNCA(cnmtPath);

                    if (outBaseTitleId != nullptr)
                    {
                        const auto key = meta.GetContentMetaKey();
                        *outBaseTitleId = tin::util::GetBaseTitleId(key.id, static_cast<NcmContentMetaType>(key.type));
                    }

                    NcmContentInfo cnmtInfo{};
                    cnmtInfo.content_id = cnmtContentId;
                    ncmU64ToContentInfoSize(cnmtSize & 0xFFFFFFFFFFFF, &cnmtInfo);
                    cnmtInfo.content_type = NcmContentType_Meta;

                    this->AddContentMeta(meta, cnmtInfo);
                    this->CommitLatest();
                    return true;
                }
                catch (...)
                {
                    return false;
                }
            }

        private:
            std::vector<NcmContentInfo> m_cnmtInfos;

            void CommitAtIndex(size_t index)
            {
                tin::data::ByteBuffer installContentMetaBuf;
                m_contentMeta[index].GetInstallContentMeta(installContentMetaBuf, m_cnmtInfos[index], m_ignoreReqFirmVersion);
                InstallContentMetaRecords(installContentMetaBuf, index);
                InstallApplicationRecord(index);
            }

            std::vector<std::tuple<nx::ncm::ContentMeta, NcmContentInfo>> ReadCNMT() override { return {}; }
            void InstallTicketCert() override {}
            void InstallNCA(const NcmContentId& /*ncaId*/) override {}
    };
}
