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

#include "install/install_nsp.hpp"

#include <machine/endian.h>

#include "install/content_file.hpp"
#include "nx/fs.hpp"

namespace tin::install::nsp
{
    namespace
    {
        std::vector<tin::install::PackageCollectionEntry> BuildCollections(const std::shared_ptr<NSP>& remoteNSP)
        {
            remoteNSP->RetrieveHeader();

            std::vector<tin::install::PackageCollectionEntry> collections;
            const auto* baseHeader = remoteNSP->GetBaseHeader();
            collections.reserve(baseHeader->numFiles);

            const u64 dataOffset = remoteNSP->GetDataOffset();
            for (u32 i = 0; i < baseHeader->numFiles; i++)
            {
                const auto* fileEntry = remoteNSP->GetFileEntry(i);
                tin::install::PackageCollectionEntry entry;
                entry.fileName = remoteNSP->GetFileEntryName(fileEntry);
                entry.dataOffset = dataOffset + fileEntry->dataOffset;
                entry.fileSize = fileEntry->fileSize;
                entry.contentInfo = tin::install::ClassifyContentFile(entry.fileName);
                collections.push_back(std::move(entry));
            }

            return collections;
        }
    }

    NSPInstall::NSPInstall(NcmStorageId destStorageId, bool ignoreReqFirmVersion, const std::shared_ptr<NSP>& remoteNSP) :
        tin::install::CollectionInstall(
            destStorageId,
            ignoreReqFirmVersion,
            BuildCollections(remoteNSP),
            [remoteNSP](void* buf, u64 offset, size_t size) {
                remoteNSP->BufferData(buf, offset, size);
            },
            [remoteNSP](
                std::shared_ptr<nx::ncm::ContentStorage>& contentStorage,
                const tin::install::PackageCollectionEntry&,
                NcmContentId contentId) {
                remoteNSP->StreamToPlaceholder(contentStorage, contentId);
            })
    {
    }
}
