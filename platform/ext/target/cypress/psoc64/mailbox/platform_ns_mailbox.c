/*
 * Copyright (c) 2019-2021, Arm Limited. All rights reserved.
 * Copyright (c) 2019-2024 Cypress Semiconductor Corporation (an Infineon company)
 * or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* -------------------------------------- Includes ----------------------------------- */
#include <limits.h>
#include <string.h>

#include "cmsis_compiler.h"

#include "cy_ipc_drv.h"
#include "cy_sysint.h"

#include "ns_ipc_config.h"
#include "tfm_ns_mailbox.h"
#include "platform_multicore.h"

/* -------------------------------------- HAL API ------------------------------------ */

static void mailbox_ipc_init(void)
{
    Cy_IPC_Drv_SetInterruptMask(Cy_IPC_Drv_GetIntrBaseAddr(IPC_RX_INTR_STRUCT),
                                0, IPC_RX_INT_MASK);
}

static void mailbox_ipc_config(void)
{
    NVIC_SetPriority(PSA_CLIENT_REPLY_NVIC_IRQn, PSA_CLIENT_REPLY_IRQ_PRIORITY);

    NVIC_EnableIRQ(PSA_CLIENT_REPLY_NVIC_IRQn);
}

int32_t tfm_ns_mailbox_hal_notify_peer(void)
{
    cy_en_ipcdrv_status_t status;

    status = Cy_IPC_Drv_SendMsgWord(Cy_IPC_Drv_GetIpcBaseAddress(IPC_TX_CHAN),
                                    IPC_TX_NOTIFY_MASK,
                                    PSA_CLIENT_CALL_REQ_MAGIC);

    if (status == CY_IPC_DRV_SUCCESS) {
        return MAILBOX_SUCCESS;
    } else {
        return MAILBOX_CHAN_BUSY;
    }
}

int32_t tfm_ns_mailbox_hal_init(struct ns_mailbox_queue_t *queue)
{
    uint32_t stage;

    if (!queue) {
        return MAILBOX_INVAL_PARAMS;
    }

    /*
     * FIXME
     * Further verification of mailbox queue address may be required according
     * to diverse NSPE implementations.
     */

    mailbox_ipc_init();

    /*
     * Wait until SPE mailbox library is ready to receive NSPE mailbox queue
     * address.
     */
    while (1) {
        platform_mailbox_wait_for_notify();

        platform_mailbox_fetch_msg_data(&stage);
        if  (stage == NS_MAILBOX_INIT_ENABLE) {
            break;
        }
    }

    /* Send out the address */
    struct mailbox_init_t ns_init;
    ns_init.status = &queue->status;
    ns_init.slot_count = NUM_MAILBOX_QUEUE_SLOT;
    ns_init.slots = &queue->slots[0];
    platform_mailbox_send_msg_ptr(&ns_init);

    /* Wait until SPE mailbox service is ready */
    while (1) {
        platform_mailbox_wait_for_notify();

        platform_mailbox_fetch_msg_data(&stage);
        if  (stage == S_MAILBOX_READY) {
            break;
        }
    }

    mailbox_ipc_config();

    return MAILBOX_SUCCESS;
}

uint32_t tfm_ns_mailbox_hal_enter_critical(void)
{
    uint32_t state = Cy_SysLib_EnterCriticalSection();

    IPC_STRUCT_Type* ipc_struct =
        Cy_IPC_Drv_GetIpcBaseAddress(IPC_PSA_MAILBOX_LOCK_CHAN);
    while(CY_IPC_DRV_SUCCESS != Cy_IPC_Drv_LockAcquire (ipc_struct))
    {
    }

    return state;
}

void tfm_ns_mailbox_hal_exit_critical(uint32_t state)
{
    IPC_STRUCT_Type* ipc_struct =
        Cy_IPC_Drv_GetIpcBaseAddress(IPC_PSA_MAILBOX_LOCK_CHAN);
    Cy_IPC_Drv_LockRelease(ipc_struct, CY_IPC_NO_NOTIFICATION);
    Cy_SysLib_ExitCriticalSection(state);
}

uint32_t tfm_ns_mailbox_hal_enter_critical_isr(void)
{
    IPC_STRUCT_Type* ipc_struct =
        Cy_IPC_Drv_GetIpcBaseAddress(IPC_PSA_MAILBOX_LOCK_CHAN);
    while(CY_IPC_DRV_SUCCESS != Cy_IPC_Drv_LockAcquire (ipc_struct))
    {
    }

    return 0;
}

void tfm_ns_mailbox_hal_exit_critical_isr(uint32_t state)
{
    (void)state;
    IPC_STRUCT_Type* ipc_struct =
        Cy_IPC_Drv_GetIpcBaseAddress(IPC_PSA_MAILBOX_LOCK_CHAN);
    Cy_IPC_Drv_LockRelease(ipc_struct, CY_IPC_NO_NOTIFICATION);
}

static bool mailbox_clear_intr(void)
{
    uint32_t status;

    status = Cy_IPC_Drv_GetInterruptStatusMasked(
                            Cy_IPC_Drv_GetIntrBaseAddr(IPC_RX_INTR_STRUCT));
    status >>= CY_IPC_NOTIFY_SHIFT;
    if ((status & IPC_RX_INT_MASK) == 0) {
        return false;
    }

    Cy_IPC_Drv_ClearInterrupt(Cy_IPC_Drv_GetIntrBaseAddr(IPC_RX_INTR_STRUCT),
                              0, IPC_RX_INT_MASK);
    return true;
}

void cpuss_interrupts_ipc_8_IRQHandler(void)
{
    uint32_t magic;

    if (!mailbox_clear_intr())
        return;

    platform_mailbox_fetch_msg_data(&magic);
    if (magic == PSA_CLIENT_CALL_REPLY_MAGIC) {
        /* Handle all the pending replies */
        tfm_ns_mailbox_wake_reply_owner_isr();
    }
}
