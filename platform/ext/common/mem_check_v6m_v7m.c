/*
 * Copyright (c) 2019-2024, Arm Limited. All rights reserved.
 * Copyright (c) 2022 Cypress Semiconductor Corporation (an Infineon
 * company) or an affiliate of Cypress Semiconductor Corporation. All rights
 * reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mem_check_v6m_v7m.h"
#include "mem_check_v6m_v7m_hal.h"
#include "internal_status_code.h"
#include "region.h"
#include "region_defs.h"
#include "tfm_arch.h"
#include "utilities.h"

#ifndef TFM_ISOLATION_LEVEL
#error TFM_ISOLATION_LEVEL is not defined!
#endif

void tfm_get_mem_region_security_attr(const void *p, size_t s,
                                      struct security_attr_info_t *p_attr)
{
    p_attr->is_valid = true;

    if (check_address_range(p, s, NS_DATA_START,
                            NS_DATA_LIMIT) == SPM_SUCCESS) {
        p_attr->is_secure = false;
        return;
    }

    if (check_address_range(p, s, NS_CODE_START,
                            NS_CODE_LIMIT) == SPM_SUCCESS) {
        p_attr->is_secure = false;
        return;
    }

    if (check_address_range(p, s, S_DATA_START, S_DATA_LIMIT) == SPM_SUCCESS) {
        p_attr->is_secure = true;
        return;
    }

    if (check_address_range(p, s, S_CODE_START, S_CODE_LIMIT) == SPM_SUCCESS) {
        p_attr->is_secure = true;
        return;
    }

    p_attr->is_valid = false;
}

#if TFM_ISOLATION_LEVEL == 2
REGION_DECLARE(Image$$, PT_UNPRIV_CODE_START, $$Base);
REGION_DECLARE(Image$$, PT_APP_ROT_CODE_END, $$Base);
REGION_DECLARE(Image$$, PT_RO_DATA_START, $$Base);
REGION_DECLARE(Image$$, PT_RO_DATA_END, $$Base);
REGION_DECLARE(Image$$, PT_APP_ROT_DATA_START, $$Base);
REGION_DECLARE(Image$$, PT_APP_ROT_DATA_END, $$Base);
#endif

void tfm_get_secure_mem_region_attr(const void *p, size_t s,
                                    struct mem_attr_info_t *p_attr)
{
#if TFM_ISOLATION_LEVEL == 1
    p_attr->is_mpu_enabled = false;
    p_attr->is_valid = true;

    if (check_address_range(p, s, S_DATA_START, S_DATA_LIMIT) == SPM_SUCCESS) {
        p_attr->is_priv_rd_allow = true;
        p_attr->is_priv_wr_allow = true;
        p_attr->is_unpriv_rd_allow = true;
        p_attr->is_unpriv_wr_allow = true;
        p_attr->is_xn = true;
        return;
    }

    if (check_address_range(p, s, S_CODE_START, S_CODE_LIMIT) == SPM_SUCCESS) {
        p_attr->is_priv_rd_allow = true;
        p_attr->is_priv_wr_allow = false;
        p_attr->is_unpriv_rd_allow = true;
        p_attr->is_unpriv_wr_allow = false;
        p_attr->is_xn = false;
        return;
    }

    p_attr->is_valid = false;
#elif TFM_ISOLATION_LEVEL == 2
    uintptr_t base, limit;

    p_attr->is_mpu_enabled = false;
    p_attr->is_valid = true;

    /*
     * Combined unprivileged shared code and Application-RoT partition code
     * regions, which have the same attributes as the privileged execute-never
     * (PXN) feature is not present.
     */
    base = (uintptr_t)&REGION_NAME(Image$$, PT_UNPRIV_CODE_START, $$Base);
    limit = (uintptr_t)&REGION_NAME(Image$$, PT_APP_ROT_CODE_END, $$Base) - 1;
    if (check_address_range(p, s, base, limit) == SPM_SUCCESS) {
        p_attr->is_priv_rd_allow = true;
        p_attr->is_priv_wr_allow = false;
        p_attr->is_unpriv_rd_allow = true;
        p_attr->is_unpriv_wr_allow = false;
        p_attr->is_xn = false;
        return;
    }

    /* RO-data region that is unprivileged, read-only and execute-never */
    base = (uintptr_t)&REGION_NAME(Image$$, PT_RO_DATA_START, $$Base);
    limit = (uintptr_t)&REGION_NAME(Image$$, PT_RO_DATA_END, $$Base) - 1;
    if (check_address_range(p, s, base, limit) == SPM_SUCCESS) {
        p_attr->is_priv_rd_allow = true;
        p_attr->is_priv_wr_allow = false;
        p_attr->is_unpriv_rd_allow = true;
        p_attr->is_unpriv_wr_allow = false;
        p_attr->is_xn = true;
        return;
    }

    /*
     * Application-RoT data region containing the metadata pointer and partition
     * RW data, ZI data and stacks.
     */
    base = (uintptr_t)&REGION_NAME(Image$$, PT_APP_ROT_DATA_START, $$Base);
    limit = (uintptr_t)&REGION_NAME(Image$$, PT_APP_ROT_DATA_END, $$Base) - 1;
    if (check_address_range(p, s, base, limit) == SPM_SUCCESS) {
        p_attr->is_priv_rd_allow = true;
        p_attr->is_priv_wr_allow = true;
        p_attr->is_unpriv_rd_allow = true;
        p_attr->is_unpriv_wr_allow = true;
        p_attr->is_xn = true;
        return;
    }

    /*
     * Treat the remaining parts in secure data section and secure code section
     * as privileged regions
     */
    base = (uintptr_t)S_DATA_START;
    limit = (uintptr_t)S_DATA_LIMIT;
    if (check_address_range(p, s, base, limit) == SPM_SUCCESS) {
        p_attr->is_priv_rd_allow = true;
        p_attr->is_priv_wr_allow = true;
        p_attr->is_unpriv_rd_allow = false;
        p_attr->is_unpriv_wr_allow = false;
        p_attr->is_xn = true;
        return;
    }

    base = (uintptr_t)S_CODE_START;
    limit = (uintptr_t)S_CODE_LIMIT;
    if (check_address_range(p, s, base, limit) == SPM_SUCCESS) {
        p_attr->is_priv_rd_allow = true;
        p_attr->is_priv_wr_allow = false;
        p_attr->is_unpriv_rd_allow = false;
        p_attr->is_unpriv_wr_allow = false;
        p_attr->is_xn = false;
        return;
    }

    p_attr->is_valid = false;
#else
#error "Cannot support current TF-M isolation level"
#endif
}

void tfm_get_ns_mem_region_attr(const void *p, size_t s,
                                struct mem_attr_info_t *p_attr)
{
    p_attr->is_mpu_enabled = false;
    p_attr->is_valid = true;

    if (check_address_range(p, s, NS_DATA_START,
                            NS_DATA_LIMIT) == SPM_SUCCESS) {
        p_attr->is_priv_rd_allow = true;
        p_attr->is_priv_wr_allow = true;
        p_attr->is_unpriv_rd_allow = true;
        p_attr->is_unpriv_wr_allow = true;
        p_attr->is_xn = true;
        return;
    }

    if (check_address_range(p, s, NS_CODE_START,
                            NS_CODE_LIMIT) == SPM_SUCCESS) {
        p_attr->is_priv_rd_allow = true;
        p_attr->is_priv_wr_allow = false;
        p_attr->is_unpriv_rd_allow = true;
        p_attr->is_unpriv_wr_allow = false;
        p_attr->is_xn = false;
        return;
    }

    p_attr->is_valid = false;
}

static void security_attr_init(struct security_attr_info_t *p_attr)
{
    /* No check if p_attr is valid */

    /*
     * The initial values may be not a valid combination.
     * The value in each filed just guarantees the least access permission in
     * case that the field is incorrectly set later.
     */
    p_attr->is_valid = false;
    p_attr->is_secure = true;
}

static void mem_attr_init(struct mem_attr_info_t *p_attr)
{
    /* No check if p_attr is valid */

    /*
     * The initial values may be not a valid combination.
     * The value in each filed just guarantees the least access permission in
     * case that the field is incorrectly set later.
     */
    p_attr->is_mpu_enabled = false;
    p_attr->is_valid = false;
    p_attr->is_xn = true;
    p_attr->is_priv_rd_allow = false;
    p_attr->is_priv_wr_allow = false;
    p_attr->is_unpriv_rd_allow = false;
    p_attr->is_unpriv_wr_allow = false;
}

/**
 * \brief Check whether the access permission matches the security settings of
 *        the target memory region
 *
 * \param[in] attr   The security_attr_info_t containing security settings of
 *                   memory region
 * \param[in] flags  The flags indicating the access permissions.
 *
 * \return SPM_SUCCESS if the check passes,
 *         SPM_ERROR_GENERIC otherwise.
 */
static int32_t security_attr_check(struct security_attr_info_t attr, uint8_t flags)
{
    bool secure_access;

    if (!attr.is_valid) {
        return SPM_ERROR_GENERIC;
    }

    secure_access = flags & MEM_CHECK_NONSECURE ? false : true;
    /*
     * Non-secure access should not access secure memory region.
     * Secure service should not directly access non-secure memory region.
     */
    if (secure_access ^ attr.is_secure) {
        return SPM_ERROR_GENERIC;
    }

    return SPM_SUCCESS;
}

/**
 * \brief Check whether the access permission matches the target non-secure
 *        memory region access attributes.
 *
 * \param[in] attr   The mem_attr_info_t containing attributes of memory region
 * \param[in] flags  The flags indicating the access permissions.
 *
 * \return SPM_SUCCESS if the check passes,
 *         SPM_ERROR_GENERIC otherwise.
 */
static int32_t ns_mem_attr_check(struct mem_attr_info_t attr, uint8_t flags)
{
    /*
     * Non-secure privileged/unprivileged check is skipped.
     * Non-secure software should implement the check if it enforces the
     * isolation between privileged and unprivileged regions.
     */

    if ((flags & MEM_CHECK_MPU_READWRITE) &&
        (attr.is_priv_rd_allow || attr.is_unpriv_rd_allow) &&
        (attr.is_priv_wr_allow || attr.is_unpriv_wr_allow)) {
            return SPM_SUCCESS;
    }

    if ((flags & MEM_CHECK_MPU_READ) &&
        (attr.is_priv_rd_allow || attr.is_unpriv_rd_allow)) {
            return SPM_SUCCESS;
    }

    return SPM_ERROR_GENERIC;
}

/**
 * \brief Check whether the access permission matches the target secure memory
 *        region access attributes.
 *
 * \param[in] attr   The mem_attr_info_t containing attributes of memory region
 * \param[in] flags  The flags indicating the access permissions.
 *
 * \return SPM_SUCCESS if the check passes,
 *         SPM_ERROR_GENERIC otherwise.
 */
static int32_t secure_mem_attr_check(struct mem_attr_info_t attr, uint8_t flags)
{
#if TFM_ISOLATION_LEVEL == 1
    /* Privileged/unprivileged is ignored in TFM_ISOLATION_LEVEL == 1 */

    if ((flags & MEM_CHECK_MPU_READWRITE) &&
        (attr.is_priv_rd_allow || attr.is_unpriv_rd_allow) &&
        (attr.is_priv_wr_allow || attr.is_unpriv_wr_allow)) {
            return SPM_SUCCESS;
    }

    if ((flags & MEM_CHECK_MPU_READ) &&
        (attr.is_priv_rd_allow || attr.is_unpriv_rd_allow)) {
            return SPM_SUCCESS;
    }

    return SPM_ERROR_GENERIC;
#else
    if (flags & MEM_CHECK_MPU_UNPRIV) {
        if ((flags & MEM_CHECK_MPU_READWRITE) && attr.is_unpriv_rd_allow &&
            attr.is_unpriv_wr_allow) {
            return SPM_SUCCESS;
        }

        if ((flags & MEM_CHECK_MPU_READ) && attr.is_unpriv_rd_allow) {
            return SPM_SUCCESS;
        }
    } else {
        if ((flags & MEM_CHECK_MPU_READWRITE) && attr.is_priv_rd_allow &&
            attr.is_priv_wr_allow) {
            return SPM_SUCCESS;
        }

        if ((flags & MEM_CHECK_MPU_READ) && attr.is_priv_rd_allow) {
            return SPM_SUCCESS;
        }
    }

    return SPM_ERROR_GENERIC;
#endif
}

/**
 * \brief Check whether the access permission matches the memory attributes of
 *        the target memory region
 *
 * \param[in] attr   The mem_attr_info_t containing memory region attributes
 * \param[in] flags  The flags indicating the access permissions.
 *
 * \return SPM_SUCCESS if the check passes,
 *         SPM_ERROR_GENERIC otherwise.
 */
static int32_t mem_attr_check(struct mem_attr_info_t attr, uint8_t flags)
{
    if (!attr.is_valid) {
        return SPM_ERROR_GENERIC;
    }

    if (flags & MEM_CHECK_NONSECURE) {
        return ns_mem_attr_check(attr, flags);
    }

    return secure_mem_attr_check(attr, flags);
}

int32_t tfm_has_access_to_region(const void *p, size_t s, uint32_t flags)
{
    struct security_attr_info_t security_attr;
    struct mem_attr_info_t mem_attr;

    /* If size is zero, this indicates an empty buffer and base is ignored */
    if (s == 0) {
        return SPM_SUCCESS;
    }

    if (!p) {
        return SPM_ERROR_GENERIC;
    }

    if ((uintptr_t)p > (UINTPTR_MAX - s)) {
        return SPM_ERROR_GENERIC;
    }

    /* Abort if current check doesn't run in PSA RoT */
    if (!tfm_arch_is_priv()) {
        tfm_core_panic();
    }

    security_attr_init(&security_attr);

    /* Retrieve security attributes of target memory region */
    tfm_hal_get_mem_security_attr(p, s, &security_attr);

    if (security_attr_check(security_attr, flags) != SPM_SUCCESS) {
        return SPM_ERROR_GENERIC;
    }

    mem_attr_init(&mem_attr);

    if (security_attr.is_secure) {
        /* Retrieve access attributes of secure memory region */
        tfm_hal_get_secure_access_attr(p, s, &mem_attr);

#if TFM_ISOLATION_LEVEL != 1
        /* Secure MPU must be enabled in Isolation Level 2 and 3 */
        if (!mem_attr.is_mpu_enabled) {
            tfm_core_panic();
        }
#endif
    } else {
        /* Retrieve access attributes of non-secure memory region. */
        tfm_hal_get_ns_access_attr(p, s, &mem_attr);
    }

    return mem_attr_check(mem_attr, flags);
}

int32_t check_address_range(const void *p, size_t s,
                            uintptr_t region_start,
                            uintptr_t region_limit)
{
    int32_t range_in_region;

    /* Check for overflow in the range parameters */
    if ((uintptr_t)p > UINTPTR_MAX - s) {
        return SPM_ERROR_GENERIC;
    }

    /* We trust the region parameters, and don't check for overflow */

    /* Calculate the result */
    range_in_region = ((uintptr_t)p >= region_start) &&
                      ((uintptr_t)((char *) p + s - 1) <= region_limit);
    if (range_in_region) {
        return SPM_SUCCESS;
    } else {
        return SPM_ERROR_GENERIC;
    }
}
