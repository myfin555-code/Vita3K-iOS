import SwiftUI
import Darwin

/// External tools grant the process permission. Only the core can declare its
/// executable mappings ready; successfully opening a URL never sets readiness.
@MainActor
struct JITSettingsView: View {
    @ObservedObject private var library = LibraryState.shared
    @AppStorage("tsubomi.jitMethod") private var method = "external"
    @State private var message: String?

    private var supportsStikDebug: Bool {
        if #available(iOS 17.4, *) { return true }
        return false
    }

    var body: some View {
        Form {
            Section("Status") {
                Label(library.jitAvailable ? "JIT ready" : "Waiting for JIT permission and memory preparation",
                      systemImage: library.jitAvailable ? "checkmark.circle" : "clock")
                Text("iOS \(UIDevice.current.systemVersion) · PID \(getpid())")
                    .font(.caption).textSelection(.enabled)
                Text(Bundle.main.bundleIdentifier ?? "Unknown bundle identifier")
                    .font(.caption).textSelection(.enabled)
            }
            Section("Enable JIT") {
                Picker("Method", selection: $method) {
                    Text("External debugger / JIT tool").tag("external")
                    if supportsStikDebug { Text("StikDebug").tag("stikdebug") }
                }
                Button("Prepare JIT") { prepare() }
                    .disabled(library.jitAvailable)
                if let message { Text(message).font(.footnote) }
                Text("Enable Developer Mode and sign the app with get-task-allow for debugger methods. The library automatically checks again while waiting. Re-enable JIT after the app process restarts.")
                    .font(.footnote)
            }
            Section("Compatible methods") {
                if #available(iOS 26.0, *) {
                    Text("StikDebug with universal.js prepares the executable region pool. Keep the script attached until JIT ready appears. Ordinary debugger attachment alone may be insufficient on devices with TXM/SPTM.")
                } else {
                    Text("Xcode/LLDB, AltJIT, SideJITServer and other compatible debugger tools can enable the same conventional JIT path. The tool must support your exact iOS version. It may detach once permission is granted.")
                }
                if supportsStikDebug {
                    Link("StikDebug setup (pairing file and VPN)", destination: URL(string: "https://github.com/StikDebug/StikDebug")!)
                }
                if #unavailable(iOS 17.0) {
                    Text("iOS 16 can also use Jitterbug/JitStreamer with the required pairing and developer disk image.")
                    Link("Jitterbug setup", destination: URL(string: "https://github.com/osy/Jitterbug")!)
                }
                Link("AltJIT instructions", destination: URL(string: "https://faq.altstore.io/altstore-classic/enabling-jit/altjit")!)
                Text("Installations that already permit executable memory, including compatible TrollStore or jailbreak setups, are detected automatically. No debugger is required when allocation and protection checks succeed.")
                if getenv("LC_HOME_PATH") != nil {
                    Text("LiveContainer: enable Use LiveContainer’s Bundle ID in its settings before requesting StikDebug.")
                }
            }
        }
        .navigationTitle("JIT")
        .onAppear {
            if !supportsStikDebug && method == "stikdebug" { method = "external" }
        }
    }

    private func prepare() {
        guard TsubomiJITBridge.hasDebugEntitlement() else {
            message = "This installation lacks get-task-allow. Re-sign it with a compatible sideloading tool to use a debugger."
            return
        }
        guard method == "stikdebug" else {
            message = "Enable JIT for this running process with your external tool, then return here. On iOS 26 use a tool running the universal script."
            return
        }
        guard supportsStikDebug, let bundleID = Bundle.main.bundleIdentifier else {
            message = "StikDebug requires iOS 17.4 or later and a valid bundle identifier."
            return
        }
        var components = URLComponents()
        components.scheme = "stikdebug"
        components.host = "enable-jit"
        components.queryItems = [URLQueryItem(name: "bundle-id", value: bundleID),
                                 URLQueryItem(name: "pid", value: String(getpid()))]
        // The allocator uses the universal protocol when conventional mappings
        // fail on iOS 26. Request its fixed script, including on unknown hardware.
        if #available(iOS 26.0, *) {
            components.queryItems?.append(URLQueryItem(name: "script-name", value: "universal.js"))
        }
        guard let url = components.url else { return }
        UIApplication.shared.open(url) { opened in
            message = opened ? "Return to Tsubomi to finish memory preparation."
                             : "Could not open StikDebug. Install it and complete pairing/VPN setup first."
        }
    }
}

@objc(TsubomiJITHost)
@MainActor
final class JITHost: NSObject {
    @objc static func viewController() -> UIViewController {
        UIHostingController(rootView: JITSheet())
    }
}

private struct JITSheet: View {
    @Environment(\.dismiss) private var dismiss
    var body: some View {
        NavigationStack {
            JITSettingsView()
                .toolbar { ToolbarItem(placement: .confirmationAction) { Button("Done") { dismiss() } } }
        }
    }
}
