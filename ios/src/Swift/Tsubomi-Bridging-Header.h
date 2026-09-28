// Everything the Swift frontend is allowed to see.
//
// The facade imports only Objective-C declarations: the emulator core is C++23 and Swift's C++
// interop is not enabled for this target, so the UI reaches the core only
// through the Objective-C facade in TsubomiBridge.h.

#import "vita3k_ios/TsubomiBridge.h"

#import "vita3k_ios/JITBridge.h"
