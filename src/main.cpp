#include <wups.h>
#include <wups/config/WUPSConfigCategory.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemIntegerRange.h>
#include <wups/config/WUPSConfigItemStub.h>
#include <wups/config_api.h>

#include <avm/drc.h>
#include <coreinit/debug.h>

#include <atomic>
#include <exception>
#include <string_view>

/**
 * Gamma-Slop (experimental, made entirely with AI)
 *
 * Overrides the gamma the system sends to the Wii U GamePad through
 * AVMSetDRCGamma (declared in wut: include/avm/drc.h).
 *
 * UNVERIFIED: the numeric scale AVMSetDRCGamma expects is not documented.
 * This plugin treats the value as a plain gamma float and lets you tune it by eye.
 */
WUPS_PLUGIN_NAME("Gamma-Slop");
WUPS_PLUGIN_DESCRIPTION("Experimental: overrides the GamePad gamma via AVMSetDRCGamma.");
WUPS_PLUGIN_VERSION("v0.1");
WUPS_PLUGIN_AUTHOR("Claude (AI)");
WUPS_PLUGIN_LICENSE("GPL-3.0");

WUPS_USE_WUT_DEVOPTAB();
WUPS_USE_STORAGE("gamma_slop");

#define CFG_ENABLED_ID "enabled"
#define CFG_STEP_ID    "gammaStep"

// Slider value / 20 = gamma. 20 = 1.00, 10 = 0.50, 60 = 3.00.
static constexpr int32_t kStepDivisor   = 20;
static constexpr int32_t kStepMin       = 10;
static constexpr int32_t kStepMax       = 60;
static constexpr int32_t kDefaultStep   = 20;
static constexpr bool kDefaultEnabled   = false;

static std::atomic<bool> sEnabled{kDefaultEnabled};
static std::atomic<int32_t> sGammaStep{kDefaultStep};

static float currentGamma() {
    return static_cast<float>(sGammaStep.load()) / static_cast<float>(kStepDivisor);
}

/**
 * Pushes our gamma to the GamePad.
 * While the hook below is active this call is routed through it, which uses our value.
 * If the hook is not active in the current process, it goes straight to the system.
 */
static void applyGamma() {
    if (!sEnabled.load()) {
        return;
    }
    float gamma = currentGamma();
    if (!AVMSetDRCGamma(&gamma)) {
        OSReport("[Gamma-Slop] AVMSetDRCGamma(%f) returned FALSE\n", gamma);
    }
}

static void enabledChanged(ConfigItemBoolean *item, bool newValue) {
    if (std::string_view(CFG_ENABLED_ID) == item->identifier) {
        sEnabled = newValue;
        WUPSStorageAPI::Store(item->identifier, newValue);
        applyGamma();
    }
}

static void stepChanged(ConfigItemIntegerRange *item, int32_t newValue) {
    if (std::string_view(CFG_STEP_ID) == item->identifier) {
        sGammaStep = newValue;
        WUPSStorageAPI::Store(item->identifier, newValue);
        applyGamma();
    }
}

WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    WUPSConfigCategory root = WUPSConfigCategory(rootHandle);
    try {
        root.add(WUPSConfigItemStub::Create("Experimental. Gamma = slider / 20 (20 = 1.00)."));
        root.add(WUPSConfigItemBoolean::Create(CFG_ENABLED_ID, "Override GamePad gamma",
                                               kDefaultEnabled, sEnabled.load(),
                                               enabledChanged));
        root.add(WUPSConfigItemIntegerRange::Create(CFG_STEP_ID, "Gamma step (10 to 60)",
                                                    kDefaultStep, sGammaStep.load(),
                                                    kStepMin, kStepMax,
                                                    &stepChanged));
        root.add(WUPSConfigItemStub::Create("Turning this off needs a reboot to restore the system value."));
    } catch (std::exception &e) {
        OSReport("[Gamma-Slop] Creating config menu failed: %s\n", e.what());
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }
    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

void ConfigMenuClosedCallback() {
    WUPSStorageAPI::SaveStorage();
}

/** Called once when the plugin is loaded. */
INITIALIZE_PLUGIN() {
    WUPSConfigAPIOptionsV1 configOptions = {.name = "Gamma-Slop"};
    if (WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback) != WUPSCONFIG_API_RESULT_SUCCESS) {
        OSReport("[Gamma-Slop] Failed to init config api\n");
    }

    bool enabled  = kDefaultEnabled;
    int32_t step  = kDefaultStep;
    WUPSStorageError res;

    if ((res = WUPSStorageAPI::GetOrStoreDefault(CFG_ENABLED_ID, enabled, kDefaultEnabled)) != WUPS_STORAGE_ERROR_SUCCESS) {
        OSReport("[Gamma-Slop] Storage (enabled) failed: %s (%d)\n", WUPSStorageAPI_GetStatusStr(res), res);
    }
    if ((res = WUPSStorageAPI::GetOrStoreDefault(CFG_STEP_ID, step, kDefaultStep)) != WUPS_STORAGE_ERROR_SUCCESS) {
        OSReport("[Gamma-Slop] Storage (step) failed: %s (%d)\n", WUPSStorageAPI_GetStatusStr(res), res);
    }
    if ((res = WUPSStorageAPI::SaveStorage()) != WUPS_STORAGE_ERROR_SUCCESS) {
        OSReport("[Gamma-Slop] SaveStorage failed: %s (%d)\n", WUPSStorageAPI_GetStatusStr(res), res);
    }

    if (step < kStepMin) {
        step = kStepMin;
    } else if (step > kStepMax) {
        step = kStepMax;
    }

    sEnabled   = enabled;
    sGammaStep = step;
}

/** Called whenever the Wii U Menu or a game starts. */
ON_APPLICATION_START() {
    applyGamma();
}

/**
 * Whoever calls AVMSetDRCGamma while the override is on (system or game) gets
 * our value instead. If nothing else ever calls it, this hook stays idle and
 * applyGamma() above does all the work.
 */
DECL_FUNCTION(BOOL, AVMSetDRCGamma, float *gamma) {
    if (sEnabled.load()) {
        float ours = currentGamma();
        return real_AVMSetDRCGamma(&ours);
    }
    return real_AVMSetDRCGamma(gamma);
}

WUPS_MUST_REPLACE(AVMSetDRCGamma, WUPS_LOADER_LIBRARY_AVM, AVMSetDRCGamma);
