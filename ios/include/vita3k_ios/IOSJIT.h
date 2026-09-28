#pragma once

// Allocation capability is independent of which external tool enabled JIT.
bool vita3k_ios_can_allocate_jit();
bool vita3k_ios_supports_universal_jit();

// Complete the universal protocol only after all executable regions exist.
void vita3k_ios_detach_jit_debugger();
