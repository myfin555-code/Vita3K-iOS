# iOS 16–26, JIT and game text input

## Build and deployment

Build the upstream core with **Xcode 26 / iOS 26 SDK**, targeting **iOS 16.0**.
`VITA3K_IOS_DEPLOYMENT_TARGET` is set before CMake initializes its toolchain and
is shared by the application and core. The `ios/triplets/arm64-ios.cmake`
vcpkg overlay builds dependencies for iOS 16.0 as well. Configure with
`-DVCPKG_OVERLAY_TRIPLETS="$PWD/ios/triplets"` and
`-DVITA3K_IOS_DEPLOYMENT_TARGET=16.0`. The GitHub workflows target iOS 16.0.
The SDK version is deliberately newer than the minimum deployment version.

The frontend uses `ObservableObject`/`Published` on all supported systems.
Liquid Glass is availability guarded and falls back to system material/buttons
on iOS 16, 17 and 18. The scroll-target carousel requires iOS 17; iOS 16 uses
the existing grid/list, including controller navigation. Sheet materials fall
back to the system sheet before iOS 16.4.

These are source-level compatibility changes, **not device certification**.
Building and installing on each OS/device remains necessary. OS compatibility
also does not guarantee that a particular Vita game runs.

## JIT methods

Open **Settings → JIT**, or tap the JIT banner. Choose one acquisition method:

- **External debugger / JIT tool**: enable JIT for the displayed bundle ID/PID
  with a compatible external tool and return to Tsubomi.
- **TrollStore** (2.0.12+): install this app through TrollStore, select
  **TrollStore**, and tap **Prepare JIT**. This opens the official
  `apple-magnifier://enable-jit?bundle-id=…` URL. TrollStore's privileged helper
  attaches to the running app and detaches; return to Tsubomi for its memory
  probe to confirm readiness. Within this app's iOS 16+ range, TrollStore supports
  16.0–16.6.1, 16.7 RC (20H18), and 17.0, not 16.7 releases or 17.0.1/18/26.
  Without TrollStore, the same URL may open Apple's Magnifier; this is not JIT
  success. Re-enable JIT after each process restart.
- **StikDebug** (iOS 17.4+): **Prepare JIT** opens the documented
  `stikdebug://enable-jit` request with the running PID and bundle ID. On iOS 26
  the request includes the allocator's fixed `universal.js` script. Pairing,
  VPN and Developer Mode must already be configured in StikDebug.

The app checks its `get-task-allow` code-signing flag for external debugger
and StikDebug requests. Sideloading must preserve this entitlement; changing an
unsigned IPA's plist does not grant it. TrollStore uses its own privileged helper
and bypasses this local entitlement check. Opening any helper URL never marks
JIT ready. No additional private entitlements are added to the IPA.

| System / installation | Memory path and applicable acquisition tools |
| --- | --- |
| iOS 16 | Conventional executable mappings: compatible AltJIT, Xcode/LLDB, Jitterbug/JitStreamer, or existing executable-memory permission |
| iOS 17.0–17.3 | Conventional mappings; use an external tool supporting these releases, such as a compatible AltJIT/SideJITServer setup |
| iOS 17.4–18 | Conventional mappings; StikDebug or a compatible external debugger/enabler |
| iOS 26 | Conventional mappings when actually permitted; otherwise StikDebug (or an equivalent universal-protocol debugger) prepares the RX/RW region pool before detaching |
| Compatible TrollStore / jailbreak installation | Automatically use conventional JIT only when the process can allocate executable memory and perform the required protection transitions; no assumption based on installer name |

No single acquisition tool works on every release. Tools above grant the same
process capabilities, so the app does not need a separate allocator for each.
This change integrates external acquisition, TrollStore and StikDebug URL launching; it
**does not embed StikJIT**, import pairing secrets, install a VPN, or add an
app extension. Built-in StikJIT requires a separately signed helper process.
LiveContainer users must enable **Use LiveContainer's Bundle ID** in its settings.

For conventional JIT, readiness requires a successful RWX allocation and
RX → RW → RX transitions. The app permits debugger detachment after permission
is granted. The iOS 26 universal path requires a traced process while all
32 × 16 MiB regions are prepared, then sends the universal detach request.
The pool is reused across games; exhaustion without an attached script fails
allocation rather than issuing an unhandled breakpoint. Restarting the app
requires acquiring JIT again. The banner only clears after preparation succeeds.

References checked during implementation:

- [StikDebug supported versions and setup](https://github.com/StikDebug/StikDebug)
- [StikJIT universal protocol and URL integration](https://github.com/StikDebug/StikJIT/blob/main/INTEGRATION.md)
- [SideJITServer supported platforms](https://github.com/stossy11/SideJITServer)
- [Jitterbug pairing and debugger launch](https://github.com/osy/Jitterbug)
- [AltJIT instructions](https://faq.altstore.io/altstore-classic/enabling-jit/altjit)
- [TrollStore supported versions and JIT URL](https://github.com/opa334/TrollStore#url-scheme)
- [TrollStore helper attach/detach implementation](https://github.com/opa334/TrollStore/blob/main/RootHelper/jit.m)

## Native game keyboard

Vita3K-Plus at `c44cec34b3b376d3a4698e869625236f0e28a73b` uses
`vita3k/android/jni/ime.cpp`, `android_state.cpp`, `VitaInputConnection.java`
and its native IME overlay. It synchronizes the mobile editor with
`emuenv.ime`, shared by `SceIme` and `SceImeDialog`. Confirm/cancel results are
returned through the guest APIs, not injected as simulated controller presses.

`ios/src/IOSKeyboard.mm` implements that boundary using a native `UITextView`:

- Existing text, caret, keyboard type, return label and multiline mode come
  from the game's request. The editor follows the system keyboard layout guide.
- iOS owns composition, selection, paste, deletion and hardware keyboard input.
  Marked text stays in UIKit until committed, so Japanese/Chinese composition
  is not repeatedly replaced by the game's committed text.
- Text and caret positions use UTF-16; limits do not split surrogate pairs.
- Done writes `SceImeDialog` results, or queues `SCE_IME_EVENT_PRESS_ENTER`
  after the game consumes the text update. Cancel respects dialogMode.
- The controls disappear while editing. Session generations reject callbacks
  belonging to a previous keyboard request; game exit removes the editor
  before guest state is destroyed.
- `sceImeSetText` and `sceImeSetCaret` synchronize game-initiated changes back
  to the editor. IME callbacks run without holding the editor mutex.

## Validation

Portable boundary tests:

```sh
cmake -S ios/tests -B build-native-tests
cmake --build build-native-tests
ctest --test-dir build-native-tests --output-on-failure
```

The upstream IPA workflow runs these tests before the Xcode build and targets iOS 16.
Linux cannot compile UIKit/SwiftUI or validate device JIT. Before release, run:

1. Install the IPA on iOS 16, 17, 18 and 26; inspect onboarding, library,
   settings, controls and rotation. Confirm iOS 16 grid/controller navigation.
2. On 16/17/18, enable conventional JIT, detach, launch and switch games.
   On 26, exercise universal preparation and detach, then switch games.
3. Try with no JIT, missing get-task-allow, missing StikDebug, and an interrupted
   universal script. Verify the app stays gated and reports the failure.
4. Exercise both `sceImeOpen` and `sceImeDialogInit`: initial text, paste,
   Thai/Japanese/Chinese, emoji at the limit, selection, backspace, hardware
   keyboard, numeric keyboard, multiline Return, Done and Cancel.
5. Test repeated Enter in a game that keeps SceIme open, game-driven text/caret
   changes, game abort while composing, background/foreground, and game exit
   while the keyboard is shown. Noncancelable dialogs must ignore Cancel.
6. On a supported TrollStore device, install the IPA through TrollStore 2.0.12+,
   request JIT from Settings, return, launch and switch games. Restart the app
   and request JIT again. With TrollStore absent or its request rejected, verify
   opening Magnifier does not clear the JIT banner or enable game launch.

Physical-device and Xcode results are pending; portable tests alone do not
establish that all JIT tools or games work on all four OS families.
