#pragma once

#include <cassert>

#ifdef ReturnIf
#undef ReturnIf
#endif
#ifdef ReturnUnless
#undef ReturnUnless
#endif

// clang-format off
#define ReturnIf(x, ...)     { if(  x ){ return __VA_ARGS__; } }
#define ReturnUnless(x, ...) { if(!(x)){ return __VA_ARGS__; } }
// clang-format on

#ifdef ContinueIf
#undef ContinueIf
#endif
#ifdef ContinueUnless
#undef ContinueUnless
#endif

// clang-format off
#define ContinueIf(x)     { if(  x ){ continue; } }
#define ContinueUnless(x) { if(!(x)){ continue; } }
// clang-format on

#ifdef BreakIf
#undef BreakIf
#endif
#ifdef BreakUnless
#undef BreakUnless
#endif

// clang-format off
#define BreakIf(x)     { if(  x ){ break; } }
#define BreakUnless(x) { if(!(x)){ break; } }
// clang-format on

#ifdef AssertReturnIf
#undef AssertReturnIf
#endif
#ifdef AssertReturnUnless
#undef AssertReturnUnless
#endif

// clang-format off
#define AssertReturnIf(x, ...)     { if(  x ){ assert(!(#x)); return __VA_ARGS__; } }
#define AssertReturnUnless(x, ...) { if(!(x)){ assert(!(#x)); return __VA_ARGS__; } }
// clang-format on

#ifdef AssertContinueIf
#undef AssertContinueIf
#endif
#ifdef AssertContinueUnless
#undef AssertContinueUnless
#endif

// clang-format off
#define AssertContinueIf(x)     { if(  x ){ assert(!(#x)); continue; } }
#define AssertContinueUnless(x) { if(!(x)){ assert(!(#x)); continue; } }
// clang-format on

#ifdef AssertBreakIf
#undef AssertBreakIf
#endif
#ifdef AssertBreakUnless
#undef AssertBreakUnless
#endif

// clang-format off
#define AssertBreakIf(x)     { if(  x ){ assert(!(#x)); break; } }
#define AssertBreakUnless(x) { if(!(x)){ assert(!(#x)); break; } }
// clang-format on

#ifdef ThrowIf
#undef ThrowIf
#endif
#ifdef ThrowUnless
#undef ThrowUnless
#endif

// clang-format off
#define ThrowIf(x, e)     { if(  x ){ throw e; } }
#define ThrowUnless(x, e) { if(!(x)){ throw e; } }
// clang-format on
