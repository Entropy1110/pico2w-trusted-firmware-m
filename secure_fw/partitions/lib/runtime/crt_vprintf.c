/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include "tfm_vprintf_unpriv.h"

int vprintf(const char *fmt, va_list args)
{
    return tfm_vprintf_unpriv(fmt, args);
}
