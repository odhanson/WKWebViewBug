//
//  AppDelegate.h
//  WKWebViewBug
//
//  Created by Oded Hanson on 8/10/20.
//  Copyright © 2020 Microsoft. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

@interface AppDelegate : NSObject <NSApplicationDelegate, WKScriptMessageHandler>
- (IBAction)accessViolationAction:(id)sender;

@property (weak) IBOutlet WKWebView *webView;


@end

