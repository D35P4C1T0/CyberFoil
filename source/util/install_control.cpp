#include <atomic>
#include <mutex>
#include <switch.h>
#include "util/install_control.hpp"

namespace {
    enum class HomeBlockMode {
        None = 0,
        ShortAndLong,
        ShortOnly,
    };

    struct InstallSessionState {
        std::mutex mutex;
        std::atomic<bool> active{false};
        int depth = 0;
        HomeBlockMode home_block_mode = HomeBlockMode::None;
    };

    InstallSessionState g_session_state;

    void BeginHomeBlockLocked(InstallSessionState& state)
    {
        if (state.home_block_mode != HomeBlockMode::None)
            return;

        Result rc = appletBeginBlockingHomeButtonShortAndLongPressed(0);
        if (R_SUCCEEDED(rc)) {
            state.home_block_mode = HomeBlockMode::ShortAndLong;
            return;
        }

        rc = appletBeginBlockingHomeButton(0);
        if (R_SUCCEEDED(rc)) {
            state.home_block_mode = HomeBlockMode::ShortOnly;
        }
    }

    void EndHomeBlockLocked(InstallSessionState& state)
    {
        switch (state.home_block_mode) {
            case HomeBlockMode::ShortAndLong: {
                const Result rc = appletEndBlockingHomeButtonShortAndLongPressed();
                (void)rc;
                break;
            }
            case HomeBlockMode::ShortOnly: {
                const Result rc = appletEndBlockingHomeButton();
                (void)rc;
                break;
            }
            case HomeBlockMode::None:
            default:
                break;
        }

        state.home_block_mode = HomeBlockMode::None;
    }
}

namespace inst::install_control {

void BeginSession()
{
    std::lock_guard<std::mutex> lock(g_session_state.mutex);
    BeginHomeBlockLocked(g_session_state);
    g_session_state.depth++;
    g_session_state.active.store(true, std::memory_order_relaxed);
}

void EndSession()
{
    std::lock_guard<std::mutex> lock(g_session_state.mutex);
    if (g_session_state.depth <= 0) {
        EndHomeBlockLocked(g_session_state);
        g_session_state.depth = 0;
        g_session_state.active.store(false, std::memory_order_relaxed);
        return;
    }

    g_session_state.depth--;
    if (g_session_state.depth == 0) {
        EndHomeBlockLocked(g_session_state);
        g_session_state.active.store(false, std::memory_order_relaxed);
    }
}

bool IsSessionActive()
{
    return g_session_state.active.load(std::memory_order_relaxed);
}

}
