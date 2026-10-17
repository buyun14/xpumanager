/**
 * Copyright (C) 2026 Intel Corporation
 * SPDX-License-Identifier: MIT
 */

/**
 * @file level_zero_compat.c
 * @brief SYSMAN API compatibility shim for older Level Zero loaders.
 *
 * Background
 * ----------
 * xpum is written against Level Zero @c 1.33.1 and calls a few SYSMAN entry
 * points that were introduced after @c 1.24.1:
 *
 *   - @c zesDevicePciLinkSpeedUpdateExt
 *   - @c zesRasGetSupportedCategoriesExp
 *   - @c zesRasGetStateExp2
 *
 * Newer loaders export them, but such a loader cannot necessarily initialise
 * an older vendor driver: the loader and the driver must agree on the DDI
 * version, and a loader that is too new for the installed
 * @c libze_intel_gpu driver fails @c zeInit() with
 * @c ZE_RESULT_ERROR_UNSUPPORTED_VERSION before any driver is available.
 *
 * The shim lets xpum link and run against such an older loader. Each stub
 * reports @c ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, which is the semantically
 * correct answer: the feature genuinely is not provided by this driver/loader
 * pair, and every xpum caller already treats a non-success result as
 * "unsupported" and degrades gracefully.
 *
 * Only these three optional diagnostic entry points are affected. Core
 * functionality (device discovery, telemetry, power/fan control, firmware
 * update, ECC, RAS via the stable @c zesRasGetStateExp API) is untouched.
 *
 * Enable with @c -Dlevel_zero_sysman_compat=true. It should stay disabled when
 * the installed Level Zero loader already exports these symbols, so that the
 * real implementations are used instead of the stubs.
 */

/*
 * zes_ddi.h declares the function-pointer typedefs against the exact C ABI
 * that the loader exports, and it resolves with the @c include/level_zero
 * include directory that Meson already adds for this target.
 */
#include <zes_ddi.h>

ze_result_t zesDevicePciLinkSpeedUpdateExt(zes_device_handle_t hDevice,
                                           ze_bool_t shouldDowngrade,
                                           zes_device_action_t *pendingAction);

ze_result_t zesRasGetSupportedCategoriesExp(zes_ras_handle_t hRas,
                                            uint32_t *pCount,
                                            zes_ras_error_category_exp_t *pCategories);

ze_result_t zesRasGetStateExp2(zes_ras_handle_t hRas,
                               const uint32_t count,
                               const zes_ras_error_category_exp_t *pCategories,
                               zes_ras_state_exp2_t *pState);

/* Keep the parameters referenced so that -Wunused-parameter stays quiet. */
#define XPUM_L0_COMPAT_UNUSED(x) ((void)(x))

ze_result_t zesDevicePciLinkSpeedUpdateExt(zes_device_handle_t hDevice,
                                           ze_bool_t shouldDowngrade,
                                           zes_device_action_t *pendingAction) {
	XPUM_L0_COMPAT_UNUSED(hDevice);
	XPUM_L0_COMPAT_UNUSED(shouldDowngrade);

	/* Leave the out-parameter in a defined state for the caller. */
	if (pendingAction != NULL) {
		*pendingAction = ZES_DEVICE_ACTION_NONE;
	}

	return ZE_RESULT_ERROR_UNSUPPORTED_FEATURE;
}

ze_result_t zesRasGetSupportedCategoriesExp(zes_ras_handle_t hRas,
                                            uint32_t *pCount,
                                            zes_ras_error_category_exp_t *pCategories) {
	XPUM_L0_COMPAT_UNUSED(hRas);
	XPUM_L0_COMPAT_UNUSED(pCategories);

	/* Report "no categories" so callers that size an array stay consistent. */
	if (pCount != NULL) {
		*pCount = 0;
	}

	return ZE_RESULT_ERROR_UNSUPPORTED_FEATURE;
}

ze_result_t zesRasGetStateExp2(zes_ras_handle_t hRas,
                               const uint32_t count,
                               const zes_ras_error_category_exp_t *pCategories,
                               zes_ras_state_exp2_t *pState) {
	XPUM_L0_COMPAT_UNUSED(hRas);
	XPUM_L0_COMPAT_UNUSED(count);
	XPUM_L0_COMPAT_UNUSED(pCategories);
	XPUM_L0_COMPAT_UNUSED(pState);

	return ZE_RESULT_ERROR_UNSUPPORTED_FEATURE;
}
