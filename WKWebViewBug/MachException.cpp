//
//  MachException.cpp
//  WKWebViewBug
//
//  Created by Oded Hanson on 8/11/20.
//  Copyright © 2020 Microsoft. All rights reserved.
//

#include "MachException.hpp"
#include "CThreadMachExceptionHandlers.hpp"
#include "MachMessage.hpp"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <mach-o/loader.h>

// The port we use to handle exceptions and to set the thread context
mach_port_t s_ExceptionPort;

const char *
GetExceptionString(
   exception_type_t exception
)
{
    switch(exception)
    {
    case EXC_BAD_ACCESS:
        return "EXC_BAD_ACCESS";

    case EXC_BAD_INSTRUCTION:
        return "EXC_BAD_INSTRUCTION";

    case EXC_ARITHMETIC:
        return "EXC_ARITHMETIC";

    case EXC_SOFTWARE:
        return "EXC_SOFTWARE";

    case EXC_BREAKPOINT:
        return "EXC_BREAKPOINT";

    case EXC_SYSCALL:
        return "EXC_SYSCALL";

    case EXC_MACH_SYSCALL:
        return "EXC_MACH_SYSCALL";

    default:
        printf("Got unknown trap code %d\n", exception);
        break;
    }
    return "INVALID CODE";
}

void *
SEHExceptionThread(void *args)
{
    MachMessage sReplyOrForward;
    MachMessage sMessage;
    kern_return_t machret;
    thread_act_t thread;

    // Loop processing incoming messages forever.
    while (true)
    {
        // Receive the next message.
        sMessage.Receive(s_ExceptionPort);

        printf("Received message %s (%08x) from (remote) %08x to (local) %08x\n",
            sMessage.GetMessageTypeName(),
            sMessage.GetMessageType(),
            sMessage.GetRemotePort(),
            sMessage.GetLocalPort());
        
        if (sMessage.IsExceptionNotification())
        {
            // This is a notification of an exception occurring on another thread.
            exception_type_t exceptionType = sMessage.GetException();
            thread = sMessage.GetThread();

            printf("ExceptionNotification %s (%u) thread %08x flavor %u\n",
                GetExceptionString(exceptionType),
                exceptionType,
                thread,
                sMessage.GetThreadStateFlavor());

            int subcode_count = sMessage.GetExceptionCodeCount();
            for (int i = 0; i < subcode_count; i++)
                printf("ExceptionNotification subcode[%d] = %llx\n", i, sMessage.GetExceptionCode(i));

            x86_thread_state64_t threadStateActual;
            unsigned int count = sizeof(threadStateActual) / sizeof(unsigned);
            machret = thread_get_state(thread, x86_THREAD_STATE64, (thread_state_t)&threadStateActual, &count);
            CHECK_MACH("thread_get_state", machret);

            printf("ExceptionNotification actual  rip %016llx rsp %016llx rbp %016llx rax %016llx r15 %016llx eflags %08llx\n",
                threadStateActual.__rip,
                threadStateActual.__rsp,
                threadStateActual.__rbp,
                threadStateActual.__rax,
                threadStateActual.__r15,
                threadStateActual.__rflags);

            x86_exception_state64_t threadExceptionState;
            unsigned int ehStateCount = sizeof(threadExceptionState) / sizeof(unsigned);
            machret = thread_get_state(thread, x86_EXCEPTION_STATE64, (thread_state_t)&threadExceptionState, &ehStateCount);
            CHECK_MACH("thread_get_state", machret);

            printf("ExceptionNotification trapno %04x cpu %04x err %08x faultAddr %016llx\n",
                threadExceptionState.__trapno,
                threadExceptionState.__cpu,
                threadExceptionState.__err,
                threadExceptionState.__faultvaddr);
            
            // Recover from exception
            threadStateActual.__rip += 2;
            
            machret = thread_set_state(thread, x86_THREAD_STATE64, (thread_state_t)&threadStateActual, x86_THREAD_STATE64_COUNT);
            CHECK_MACH("thread_set_state", machret);
            
            
            // Send the result of handling the exception back in a reply.
            printf("ReplyToNotification KERN_SUCCESS thread %08x port %08x\n", thread, sMessage.GetRemotePort());
            sReplyOrForward.ReplyToNotification(sMessage, KERN_SUCCESS);
        }
        else
        {
            printf("Unknown message type: %u", sMessage.GetMessageType());
        }
    }
}

/*++
Function :
    SEHInitializeMachExceptions

    Initialize all SEH-related stuff related to mach exceptions

    flags - PAL_INITIALIZE flags

Return value :
    TRUE  if SEH support initialization succeeded
    FALSE otherwise
--*/
bool SEHInitializeMachExceptions()
{
    pthread_t exception_thread;
    kern_return_t machret;

    // Allocate a mach port that will listen in on exceptions
    machret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &s_ExceptionPort);
    if (machret != KERN_SUCCESS)
    {
        printf("mach_port_allocate failed: %d\n", machret);
        return FALSE;
    }

    // Insert the send right into the task
    machret = mach_port_insert_right(mach_task_self(), s_ExceptionPort, s_ExceptionPort, MACH_MSG_TYPE_MAKE_SEND);
    if (machret != KERN_SUCCESS)
    {
        printf("mach_port_insert_right failed: %d\n", machret);
        return FALSE;
    }

    // Create the thread that will listen to the exception for all threads
    int createret = pthread_create(&exception_thread, NULL, SEHExceptionThread, NULL);
    if (createret != 0)
    {
        printf("pthread_create failed, error is %d (%s)\n", createret, strerror(createret));
        return FALSE;
    }
    
    exception_mask_t machExceptionMask = PAL_EXC_MANAGED_MASK;
    machret = thread_set_exception_ports(mach_thread_self(), machExceptionMask, s_ExceptionPort, EXCEPTION_DEFAULT, MACHINE_THREAD_STATE);
    if (machret != KERN_SUCCESS)
    {
        printf("task_set_exception_ports failed: %d\n", machret);
        return FALSE;
    }

    // We're done
    return TRUE;
}
