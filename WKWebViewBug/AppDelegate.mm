//
//  AppDelegate.m
//  WKWebViewBug
//
//  Created by Oded Hanson on 8/10/20.
//  Copyright © 2020 Microsoft. All rights reserved.
//

#import "AppDelegate.h"
#include <JavaScriptCore/JavaScriptCore.h>

#include "MachException.hpp"
#include "CThreadMachExceptionHandlers.hpp"

@interface AppDelegate ()

@property (weak) IBOutlet NSWindow *window;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    
    // Workaround for JSC setting mach exception ports before our code does.
    // These options prevent JSC from crashing after we replace their mach exception ports for EXC_BAD_ACCESS
    setenv("JSC_useWebAssembly", "0", 1);
    setenv("JSC_usePollingTraps", "1", 1);

    // Get JSC to initialize itself.
    JSStringRelease(JSStringCreateWithUTF8CString("Init JSC"));

    SEHInitializeMachExceptions();
    
    [_webView.configuration.userContentController addScriptMessageHandler:self name:@"notify"];

    NSString* html = @"<html><body>"
    "<h1>Hello, World!</h1>"
    "<button type=\"button\" onclick=\"window.webkit.messageHandlers.notify.postMessage('button was clicked');\">Click Me to repro the bug!</button>"
    "</body></html>";

    [_webView loadHTMLString:html baseURL:nil];
    
    CThreadMachExceptionHandlers taskHandlers;
    taskHandlers.LoadThreadSelf();
    taskHandlers.Print();

}


- (void)applicationWillTerminate:(NSNotification *)aNotification {
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (IBAction)accessViolationAction:(id)sender {
    
    printf("Going to cause an access violation\n");
    int *x = 0;
    
    int y = *x;
    
    printf("Magically recovered from AV\n");
}

- (void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message
{
    NSLog(@"Received message: %@ with body: %@", message.name, message.body);
    
    CThreadMachExceptionHandlers taskHandlers;
    taskHandlers.LoadThreadSelf();
    taskHandlers.Print();
    
    printf("Note that the registred exception ports and masks have changed\n");

}
@end
