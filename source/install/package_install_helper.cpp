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

#include "install/package_install_helper.hpp"

#include <filesystem>
#include <memory>
#include <thread>

#include "install/nca.hpp"
#include "ui/MainApplication.hpp"
#include "util/config.hpp"
#include "util/crypto.hpp"
#include "util/install_diagnostics.hpp"
#include "util/lang.hpp"
#include "util/title_util.hpp"
#include "util/util.hpp"

namespace inst::ui {
    extern MainApplication *mainApp;
}

namespace tin::install
{
    void InstallNcaFromPackage(
        const NcmContentId& ncaId,
        const PackageNcaEntryInfo& entryInfo,
        NcmStorageId destStorageId,
        bool& declinedValidation,
        const std::function<void(void* buf, u64 offset, size_t size)>& bufferData,
        const std::function<void(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId)>& streamToPlaceholder,
        const std::function<void()>& afterStream)
    {
        LOG_DEBUG("Installing %s to storage Id %u\n", entryInfo.fileName.c_str(), destStorageId);

        std::shared_ptr<nx::ncm::ContentStorage> contentStorage(new nx::ncm::ContentStorage(destStorageId));

        try {
            contentStorage->DeletePlaceholder(*(NcmPlaceHolderId*)&ncaId);
        }
        catch (...) {}

        LOG_DEBUG("Size: 0x%lx\n", entryInfo.fileSize);

        try {
            if (inst::config::validateNCAs && !declinedValidation)
            {
                inst::diag::NoteStep("NCA verify: validating signature for " + tin::util::GetNcaIdString(ncaId));
                auto header = std::make_unique<tin::install::NcaHeader>();
                bufferData(header.get(), entryInfo.dataOffset, sizeof(tin::install::NcaHeader));

                Crypto::AesXtr crypto(Crypto::Keys().headerKey, false);
                crypto.decrypt(header.get(), header.get(), sizeof(tin::install::NcaHeader), 0, 0x200);

                if (header->magic != MAGIC_NCA3)
                    THROW_FORMAT("Invalid NCA magic");

                if (!Crypto::rsa2048PssVerify(&header->magic, 0x200, header->fixed_key_sig, Crypto::NCAHeaderSignature))
                {
                    inst::diag::NoteStep("NCA verify: signature validation failed for " + tin::util::GetNcaIdString(ncaId), false);
                    std::string audioPath = "romfs:/audio/bark.wav";
                    if (!inst::config::soundEnabled) audioPath = "";
                    if (std::filesystem::exists(inst::config::appDir + "/bark.wav")) audioPath = inst::config::appDir + "/bark.wav";
                    std::thread audioThread(inst::util::playAudio, audioPath);
                    int rc = inst::ui::mainApp->CreateShowDialog("inst.nca_verify.title"_lang, "inst.nca_verify.desc"_lang, {"common.cancel"_lang, "inst.nca_verify.opt1"_lang}, false);
                    audioThread.join();
                    if (rc != 1)
                        THROW_FORMAT(("inst.nca_verify.error"_lang + tin::util::GetNcaIdString(ncaId)).c_str());

                    declinedValidation = true;
                    inst::diag::NoteStep("NCA verify: user bypass enabled for remaining NCAs", false);
                }
                else {
                    inst::diag::NoteStep("NCA verify: signature valid for " + tin::util::GetNcaIdString(ncaId));
                }
            }

            streamToPlaceholder(contentStorage, ncaId);
            if (afterStream)
                afterStream();

            LOG_DEBUG("Registering placeholder...\n");

            try
            {
                contentStorage->Register(*(NcmPlaceHolderId*)&ncaId, ncaId);
            }
            catch (...)
            {
                LOG_DEBUG(("Failed to register " + entryInfo.fileName + ". It may already exist.\n").c_str());
            }

            try
            {
                contentStorage->DeletePlaceholder(*(NcmPlaceHolderId*)&ncaId);
            }
            catch (...) {}
        }
        catch (...)
        {
            try { contentStorage->DeletePlaceholder(*(NcmPlaceHolderId*)&ncaId); } catch (...) {}
            try {
                if (contentStorage->Has(ncaId))
                    contentStorage->Delete(ncaId);
            } catch (...) {}
            throw;
        }
    }
}
