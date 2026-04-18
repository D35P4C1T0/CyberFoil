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

#include <thread>

#include "install/install_xci.hpp"
#include "install/package_install_helper.hpp"
#include "util/file_util.hpp"
#include "util/debug.h"
#include "install/nca.hpp"

namespace tin::install::xci
{
    XCIInstallTask::XCIInstallTask(NcmStorageId destStorageId, bool ignoreReqFirmVersion, const std::shared_ptr<XCI>& xci) :
        Install(destStorageId, ignoreReqFirmVersion), m_xci(xci)
    {
        m_xci->RetrieveHeader();
    }

    std::vector<std::tuple<nx::ncm::ContentMeta, NcmContentInfo>> XCIInstallTask::ReadCNMT()
    {
        return tin::install::ReadCnmtFromPackage(*m_xci, m_destStorageId, [this](const NcmContentId& ncaId) {
            this->InstallNCA(ncaId);
        });
    }

    void XCIInstallTask::InstallNCA(const NcmContentId& ncaId)
    {
        const HFS0FileEntry* fileEntry = m_xci->GetFileEntryByNcaId(ncaId);
        tin::install::PackageNcaEntryInfo entryInfo{
            .fileName = m_xci->GetFileEntryName(fileEntry),
            .dataOffset = m_xci->GetDataOffset() + fileEntry->dataOffset,
            .fileSize = fileEntry->fileSize,
        };

        tin::install::InstallNcaFromPackage(
            ncaId,
            entryInfo,
            m_destStorageId,
            m_declinedValidation,
            [this](void* buf, u64 offset, size_t size) {
                m_xci->BufferData(buf, offset, size);
            },
            [this](std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId id) {
                m_xci->StreamToPlaceholder(contentStorage, id);
            },
            []() {
                LOG_DEBUG("                                                           \r");
            });
    }

    void XCIInstallTask::InstallTicketCert()
    {
        tin::install::InstallTicketCertFromPackage(*m_xci, [this](void* buf, u64 dataOffset, size_t size) {
            m_xci->BufferData(buf, m_xci->GetDataOffset() + dataOffset, size);
        });
    }
}
