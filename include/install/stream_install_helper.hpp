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

#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "install/install.hpp"
#include "install/content_file.hpp"
#include "install/package_install_helper.hpp"
#include "nx/ncm.hpp"
#include "nx/ipc/es.h"
#include "nx/nca_writer.h"
#include "util/file_util.hpp"
#include "util/title_util.hpp"

namespace tin::install
{
    class StreamingInstallHelper final : public Install
    {
        public:
            struct StreamInstallEntryState
            {
                std::string name;
                NcmContentId nca_id{};
                std::uint64_t size = 0;
                std::uint64_t written = 0;
                bool started = false;
                bool complete = false;
                bool is_nca = false;
                bool is_cnmt = false;
                bool is_ticket = false;
                bool is_cert = false;
                bool has_nca_id = false;
                std::shared_ptr<nx::ncm::ContentStorage> storage;
                std::unique_ptr<NcaWriter> nca_writer;
                std::vector<std::uint8_t> ticket_buf;
                std::vector<std::uint8_t> cert_buf;
                std::string pair_base_name;
            };

            using TicketCertBufferMap = std::unordered_map<std::string, std::vector<std::uint8_t>>;

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

            static void PopulateEntryStateFromName(StreamInstallEntryState& entry)
            {
                const auto info = tin::install::ClassifyContentFile(entry.name);
                PopulateEntryStateFromInfo(entry, info);
            }

            static void PopulateEntryStateFromCollection(StreamInstallEntryState& entry, const PackageCollectionEntry& collection)
            {
                entry.name = collection.fileName;
                entry.size = collection.fileSize;
                PopulateEntryStateFromInfo(entry, collection.contentInfo);
            }

            template <typename EntryRange, typename GetEntry, typename OnImportError, typename OnCommitError>
            static bool FinalizeInstallEntries(
                StreamingInstallHelper& helper,
                const EntryRange& entries,
                GetEntry&& getEntry,
                OnImportError&& onImportError,
                OnCommitError&& onCommitError)
            {
                TicketCertBufferMap ticketsByBase;
                TicketCertBufferMap certsByBase;
                CollectTicketCertBuffers(entries, ticketsByBase, certsByBase, getEntry);

                const bool ok = ImportTicketCertPairs(ticketsByBase, certsByBase, onImportError);
                try
                {
                    helper.CommitAll();
                }
                catch (const std::exception& e)
                {
                    onCommitError(e.what());
                    return false;
                }
                catch (...)
                {
                    onCommitError(nullptr);
                    return false;
                }

                return ok;
            }

            template <typename Source, typename EntryState, typename EntryMap, typename BeforeRead, typename OnEntryStart, typename OnChunk, typename OnEntryDone>
            static bool InstallCollectionSetFromSource(
                StreamingInstallHelper& helper,
                Source& source,
                const std::vector<PackageCollectionEntry>& collections,
                NcmStorageId destStorageId,
                std::size_t bufferSize,
                EntryMap& entries,
                BeforeRead&& beforeRead,
                OnEntryStart&& onEntryStart,
                OnChunk&& onChunk,
                OnEntryDone&& onEntryDone)
            {
                std::vector<std::uint8_t> buf(bufferSize);

                for (const auto& collection : collections)
                {
                    EntryState entry;
                    PopulateEntryStateFromCollection(entry, collection);

                    if (!EnsureEntryStarted(entry, destStorageId))
                        return false;
                    if (!onEntryStart(collection, entry))
                        return false;

                    u64 remaining = collection.fileSize;
                    u64 offset = collection.dataOffset;
                    while (remaining > 0)
                    {
                        beforeRead(collection, entry);

                        const auto chunk = static_cast<size_t>(std::min<u64>(remaining, buf.size()));
                        u64 bytes_read = 0;
                        if (R_FAILED(source.Read(buf.data(), static_cast<s64>(offset), static_cast<s64>(chunk), &bytes_read)))
                            return false;
                        if (bytes_read == 0)
                            return false;

                        if (!WriteSequentialEntryData(entry, buf.data(), static_cast<size_t>(bytes_read), &helper))
                            return false;

                        offset += bytes_read;
                        remaining -= bytes_read;
                        if (!onChunk(collection, entry, bytes_read, offset, remaining))
                            return false;
                    }

                    if (!onEntryDone(collection, entry))
                        return false;

                    entries.emplace(entry.name, std::move(entry));
                }

                return true;
            }

        private:
            static void PopulateEntryStateFromInfo(StreamInstallEntryState& entry, const ContentFileInfo& info)
            {
                entry.is_nca = info.is_nca;
                entry.is_cnmt = info.is_cnmt;
                entry.is_ticket = info.is_ticket;
                entry.is_cert = info.is_cert;
                entry.has_nca_id = info.has_nca_id;
                entry.nca_id = info.nca_id;
                entry.pair_base_name = info.pair_base_name;
            }

        public:
            static bool EnsureEntryStarted(StreamInstallEntryState& entry, NcmStorageId destStorageId)
            {
                if (entry.started)
                    return true;
                if (!entry.is_nca)
                {
                    entry.started = true;
                    return true;
                }

                entry.storage = std::make_shared<nx::ncm::ContentStorage>(destStorageId);
                try
                {
                    entry.storage->DeletePlaceholder(*(NcmPlaceHolderId*)&entry.nca_id);
                }
                catch (...) {}
                entry.nca_writer = std::make_unique<NcaWriter>(entry.nca_id, entry.storage);
                entry.started = true;
                return true;
            }

            static bool WriteSequentialEntryData(StreamInstallEntryState& entry, const std::uint8_t* data, size_t size, StreamingInstallHelper* helper = nullptr, u64* outBaseTitleId = nullptr)
            {
                if (entry.is_ticket)
                {
                    entry.ticket_buf.insert(entry.ticket_buf.end(), data, data + size);
                    entry.written += size;
                    if (entry.written >= entry.size)
                        entry.complete = true;
                    return true;
                }
                if (entry.is_cert)
                {
                    entry.cert_buf.insert(entry.cert_buf.end(), data, data + size);
                    entry.written += size;
                    if (entry.written >= entry.size)
                        entry.complete = true;
                    return true;
                }
                if (!entry.is_nca)
                {
                    entry.written += size;
                    if (entry.written >= entry.size)
                        entry.complete = true;
                    return true;
                }
                if (!entry.nca_writer)
                    return false;

                entry.nca_writer->write(data, size);
                entry.written += size;
                if (entry.written < entry.size)
                    return true;

                entry.nca_writer->close();
                try
                {
                    entry.storage->Register(*(NcmPlaceHolderId*)&entry.nca_id, entry.nca_id);
                    entry.storage->DeletePlaceholder(*(NcmPlaceHolderId*)&entry.nca_id);
                }
                catch (...) {}
                entry.complete = true;
                if (entry.is_cnmt && helper != nullptr)
                    (void)helper->CommitInstalledCnmt(entry.storage, entry.nca_id, entry.size, outBaseTitleId);
                return true;
            }

            template <typename EntryRange, typename GetEntry>
            static void CleanupInstallEntries(EntryRange& entries, GetEntry&& getEntry)
            {
                for (auto& item : entries)
                {
                    auto& entry = getEntry(item);
                    if (!entry.is_nca || !entry.storage)
                        continue;
                    try
                    {
                        if (entry.nca_writer)
                            entry.nca_writer->close();
                    }
                    catch (...) {}
                    try
                    {
                        entry.storage->DeletePlaceholder(*(NcmPlaceHolderId*)&entry.nca_id);
                    }
                    catch (...) {}
                    try
                    {
                        if (entry.storage->Has(entry.nca_id))
                            entry.storage->Delete(entry.nca_id);
                    }
                    catch (...) {}
                }
            }

            template <typename EntryRange, typename GetEntry>
            static void CollectTicketCertBuffers(const EntryRange& entries, TicketCertBufferMap& ticketsByBase, TicketCertBufferMap& certsByBase, GetEntry&& getEntry)
            {
                ticketsByBase.clear();
                certsByBase.clear();

                for (const auto& item : entries)
                {
                    const auto& entry = getEntry(item);
                    if (entry.pair_base_name.empty())
                        continue;

                    if (entry.is_ticket)
                        ticketsByBase[entry.pair_base_name] = entry.ticket_buf;
                    if (entry.is_cert)
                        certsByBase[entry.pair_base_name] = entry.cert_buf;
                }
            }

            template <typename OnImportError>
            static bool ImportTicketCertPairs(const TicketCertBufferMap& ticketsByBase, const TicketCertBufferMap& certsByBase, OnImportError&& onImportError)
            {
                bool ok = true;

                for (const auto& [baseName, ticketBuf] : ticketsByBase)
                {
                    const auto certIt = certsByBase.find(baseName);
                    if (certIt == certsByBase.end())
                        continue;
                    if (ticketBuf.empty() || certIt->second.empty())
                        continue;

                    const Result rc = esImportTicket(
                        ticketBuf.data(), ticketBuf.size(),
                        certIt->second.data(), certIt->second.size());
                    if (R_FAILED(rc))
                    {
                        ok = false;
                        if (!onImportError(baseName, rc))
                            return false;
                    }
                }

                return ok;
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
