//
//  CThreadMachExceptionHandlers.hpp
//  WKWebViewBug
//
//  Created by Oded Hanson on 8/11/20.
//  Copyright © 2020 Microsoft. All rights reserved.
//

#ifndef CThreadMachExceptionHandlers_hpp
#define CThreadMachExceptionHandlers_hpp

#include <mach/mach.h>

// Structure used to return data about a single handler to a caller.
struct MachExceptionHandler
{
    exception_mask_t m_mask;
    exception_handler_t m_handler;
    exception_behavior_t m_behavior;
    thread_state_flavor_t m_flavor;
};

// Class abstracting previously registered Mach exception handlers for a thread.
struct CThreadMachExceptionHandlers
{
public:
    // Maximum number of exception ports we hook.  Must be the count
    // of all bits set in the exception masks defined in machexception.h.
    static const int s_nPortsMax = 6;

    // Saved exception ports, exactly as returned by
    // thread_swap_exception_ports.
    mach_msg_type_number_t m_nPorts;
    exception_mask_t m_masks[s_nPortsMax];
    exception_handler_t m_handlers[s_nPortsMax];
    exception_behavior_t m_behaviors[s_nPortsMax];
    thread_state_flavor_t m_flavors[s_nPortsMax];
    
    CThreadMachExceptionHandlers() :
        m_nPorts(-1)
    {
    }
    
    kern_return_t LoadThreadSelf();
    void Print();

    // Get handler details for a given type of exception. If successful the structure pointed at by
    // pHandler is filled in and true is returned. Otherwise false is returned.
    bool GetHandler(exception_type_t eException, MachExceptionHandler *pHandler);

private:
    // Look for a handler for the given exception within the given handler node. Return its index if
    // successful or -1 otherwise.
    int GetIndexOfHandler(exception_mask_t bmExceptionMask);
};


#endif /* CThreadMachExceptionHandlers_hpp */
