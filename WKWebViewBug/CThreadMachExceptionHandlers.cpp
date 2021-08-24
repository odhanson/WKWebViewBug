//
//  CThreadMachExceptionHandlers.cpp
//  WKWebViewBug
//
//  Created by Oded Hanson on 8/11/20.
//  Copyright © 2020 Microsoft. All rights reserved.
//

#include "CThreadMachExceptionHandlers.hpp"
#include "MachException.hpp"
#include <assert.h>
#include <stdio.h>

extern mach_port_t s_ExceptionPort;

// Get handler details for a given type of exception. If successful the structure pointed at by pHandler is
// filled in and true is returned. Otherwise false is returned.
bool CThreadMachExceptionHandlers::GetHandler(exception_type_t eException, MachExceptionHandler *pHandler)
{
    exception_mask_t bmExceptionMask = (1 << eException);
    int idxHandler = GetIndexOfHandler(bmExceptionMask);

    // Did we find a handler?
    if (idxHandler == -1)
        return false;

    // Found one, so initialize the output structure with the details.
    pHandler->m_mask = m_masks[idxHandler];
    pHandler->m_handler = m_handlers[idxHandler];
    pHandler->m_behavior = m_behaviors[idxHandler];
    pHandler->m_flavor = m_flavors[idxHandler];

    return true;
}

// Look for a handler for the given exception within the given handler node. Return its index if successful or
// -1 otherwise.
int CThreadMachExceptionHandlers::GetIndexOfHandler(exception_mask_t bmExceptionMask)
{
    // Check all handler entries for one handling the exception mask.
    for (mach_msg_type_number_t i = 0; i < m_nPorts; i++)
    {
        // Entry covers this exception type and the handler isn't null
        if (m_masks[i] & bmExceptionMask && m_handlers[i] != MACH_PORT_NULL)
        {
            assert(m_handlers[i] != s_ExceptionPort);

            // One more check; has the target handler port become dead?
            mach_port_type_t ePortType;
            if (mach_port_type(mach_task_self(), m_handlers[i], &ePortType) == KERN_SUCCESS && !(ePortType & MACH_PORT_TYPE_DEAD_NAME))
            {
                // Got a matching entry.
                return i;
            }
        }
    }

    // Didn't find a handler.
    return -1;
}

kern_return_t CThreadMachExceptionHandlers::LoadThreadSelf()
{
    return thread_get_exception_ports(mach_thread_self(),
        PAL_EXC_ALL_MASK,
        m_masks,
        &m_nPorts,
        m_handlers,
        m_behaviors,
        m_flavors);
}

void CThreadMachExceptionHandlers::Print()
{
    printf("SEHInitializeMachExceptions: TASK PORT count %d\n", m_nPorts);
    for (mach_msg_type_number_t i = 0; i < m_nPorts; i++)
    {
        printf("SEHInitializeMachExceptions: TASK PORT mask %08x handler: %08x behavior %08x flavor %u\n",
            m_masks[i],
            m_handlers[i],
            m_behaviors[i],
            m_flavors[i]);
    }
}
