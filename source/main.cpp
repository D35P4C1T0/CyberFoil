#include <thread>
#include "switch.h"
#include "util/debug.h"
#include "util/error.hpp"
#include "ui/MainApplication.hpp"
#include "util/util.hpp"
#include "util/config.hpp"
#include "util/offline_db_update.hpp"

using namespace pu::ui::render;
int main(int argc, char* argv[])
{
    bool appInitialized = false;
    try {
        debugLogReset();
        inst::util::initApp();
        appInitialized = true;
        auto rendererOptions = RendererInitOptions::RendererNoSound;
        rendererOptions.InitRomFs = false;
        auto renderer = Renderer::New(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER,
            rendererOptions, RendererHardwareFlags);
        auto main = inst::ui::MainApplication::New(renderer);
        std::thread updateThread;
        std::thread offlineDbUpdateCheckThread;
        if (inst::config::autoUpdate && inst::util::getIPAddress() != "1.0.0.127") updateThread = std::thread(inst::util::checkForAppUpdate);
        inst::offline::dbupdate::ResetStartupCheckState();
        if (inst::config::offlineDbAutoCheckOnStartup && inst::util::getIPAddress() != "1.0.0.127") {
            offlineDbUpdateCheckThread = std::thread([]() {
                const auto result = inst::offline::dbupdate::CheckForUpdate(inst::config::offlineDbManifestUrl);
                inst::offline::dbupdate::SetStartupCheckResult(result);
            });
        }
        main->Prepare();
        inst::util::updateExitLog("main prepared; show begin");
        main->ShowWithFadeIn();
        inst::util::updateExitLog("navigation audio release before renderer close");
        inst::util::releaseNavigationClickAudio();
        inst::util::updateExitLog("main loop returned; renderer close begin");
        main->Close();
        inst::util::updateExitLog("renderer close done");
        if (updateThread.joinable()) {
            inst::util::updateExitLog("joining update thread");
            updateThread.join();
            inst::util::updateExitLog("update thread joined");
        }
        if (offlineDbUpdateCheckThread.joinable()) {
            inst::util::updateExitLog("joining offline db update thread");
            offlineDbUpdateCheckThread.join();
            inst::util::updateExitLog("offline db update thread joined");
        }
    } catch (std::exception& e) {
        LOG_DEBUG("An error occurred:\n%s", e.what());
    } catch (...) {
        LOG_DEBUG("An unknown error occurred during startup.");
    }
    if (appInitialized) {
        inst::util::updateExitLog("main cleanup deinit requested");
        inst::util::deinitApp();
    }
    inst::util::updateExitLog("main returning rc=0");
    return 0;
}
