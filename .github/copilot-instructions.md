# c-util Copilot Instructions

## Project Overview

c-util is a Microsoft Azure C utility library providing fundamental data structures and asynchronous operation primitives. This is a C99/C11 library with extensive macro-based code generation patterns for type-safe generic programming.

## Core Architecture Patterns

### THANDLE Reference Counting System
- **THANDLE(T)** is the foundation of memory management - a thread-safe reference-counted smart pointer system
- Every data structure uses THANDLE for memory safety: `THANDLE(ASYNC_OP)`, `THANDLE(TARRAY_TYPE(int))`, etc.
- Key operations: `THANDLE_ASSIGN()`, `THANDLE_INITIALIZE()`, `THANDLE_CREATE()`, `THANDLE_INC_REF()`, `THANDLE_DEC_REF()`
- Memory is freed automatically when reference count reaches zero
- Always use `THANDLE_ASSIGN(&handle, NULL)` to release references, never direct assignment

### Macro-Based Generic Programming
- **TARRAY(T)** generates type-safe dynamic arrays: `TARRAY_TYPE_DECLARE(int)` in headers, `TARRAY_TYPE_DEFINE(int)` in source
- **THANDLE_TUPLE_ARRAY** for arrays of THANDLE tuples with `DECLARE_THANDLE_TUPLE_ARRAY(name, type1, field1, type2, field2)`
- **RC_STRING** for reference-counted strings with copy-on-write semantics
- All generic types follow the pattern: `DECLARE` in headers, `DEFINE` in implementation files

### Asynchronous Operations Framework
- **async_op.h** provides the base for all async operations with built-in cancellation support
- `ASYNC_OP_STATE` enum tracks operation lifecycle: `ASYNC_RUNNING` → `ASYNC_CANCELLING`
- Always provide both `cancel` and `dispose` function pointers to `async_op_create`
- Use `async_op_from_context()` to get THANDLE from context pointer

### Dependency Chain
Key dependencies in order: `macro-utils-c` → `c-logging` → `c-pal` → `c-util`
- **c-pal**: Platform abstraction layer providing `interlocked.h`, `gballoc_hl.h`, `thandle.h`
- **umock-c**: Mocking framework used extensively in tests
- **c-logging**: Structured logging with `LogError()`, `LogInfo()` macros

## Development Workflows

### Building and Testing
```bash
# Recommended: Use out-of-source build in cmake folder
mkdir cmake
cd cmake
cmake .. -Drun_unittests=ON -Drun_int_tests=ON -DCMAKE_BUILD_TYPE=Debug

# Alternative: In-source build (not recommended for development)
cmake . -Drun_unittests=ON -Drun_int_tests=ON -DCMAKE_BUILD_TYPE=Debug

# Build from cmake folder
cmake --build .

# Run tests from cmake folder
ctest -C Debug --output-on-failure
```

### Memory Allocator Selection
- Set `GBALLOC_LL_TYPE` to control allocator: `JEMALLOC`, `MIMALLOC`, or default `HEAP`
- Use `vcpkg.json` dependencies for jemalloc on Windows
- Custom allocators integrate via `THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS`

### Test Patterns
- Unit tests use `umock_c` with `STRICT_EXPECTED_CALL()` for call verification
- Integration tests suffix with `_int`, unit tests with `_ut`
- Always use `real_gballoc_hl_init()` in test setup and `real_gballoc_hl_deinit()` in cleanup
- Test file structure: `BEGIN_TEST_SUITE()`, `TEST_SUITE_INITIALIZE()`, `TEST_FUNCTION()`, `TEST_SUITE_CLEANUP()`
- **API Section Comments**: Mark the beginning of tests for each API with comments like `/* API_FUNCTION_NAME */`
- **SRS Requirements**: Tag each test with SRS requirements using format `/*Tests_SRS_MODULE_NN_NNN: [requirement description]*/`
  - **AI-Generated Code**: Use spec author ID `88` for all GitHub Copilot AI-generated modules and requirements
- **AAA Pattern**: All test functions must follow the Arrange-Act-Assert pattern with clear comment sections:
  - `///arrange` - Setup test data and mock expectations
  - `///act` - Execute the function under test
  - `///assert` - Verify results and mock call expectations
  - `///cleanup` - Release resources and reset handles to NULL (when applicable)

## Code Conventions

### Error Handling
- Functions return error codes or NULL for failure
- Use `LogError()` for all error conditions with descriptive messages
- Check all allocations and handle failures gracefully
- Never ignore return values from THANDLE operations

### Header Structure
```c
// Standard header pattern
#include <stdint.h>  // Always first for C99 types
#include "macro_utils/macro_utils.h"
#include "c_pal/thandle.h" 
#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

// Declare types, then functions
THANDLE_TYPE_DECLARE(MY_TYPE);
MOCKABLE_FUNCTION(, THANDLE(MY_TYPE), my_type_create, int, param);

#ifdef __cplusplus
}
#endif
```

### Memory and Lifecycle
- All allocations go through `gballoc_hl.h` functions (`malloc`, `malloc_flex`, `free`)
- Structures containing THANDLE fields need proper initialization: `THANDLE_INITIALIZE(TYPE)(&field, NULL)`
- Always provide dispose functions for cleanup, even if just logging
- Use `CONTAINING_RECORD` macro to get wrapper from embedded structure pointers

## Critical Integration Points

### Windows-Specific Features
- `thread_notifications_dispatcher` and `for_each_in_folder` are Windows-only
- Thread notifications use `thread_notifications_lackey_dll` for cross-process communication
- Conditional compilation with `#if WIN32` guards these features

### External Dependencies
- **smhasher/MurmurHash2** integrated for hashing - fails build if missing, update submodules
- **vcpkg** used on Windows for package management, particularly jemalloc
- **c-build-tools** provides build macros like `build_test_folder()` and `set_default_build_options()`

### Cross-Component Communication
- Use `channel.h` for producer-consumer patterns with flow control
- `constbuffer_array` for efficient buffer management without copying
- `tcall_dispatcher` for marshaling calls across thread boundaries
- `worker_thread` and `tp_worker_thread` for background task execution

## Common Pitfalls to Avoid

- Never directly assign THANDLE variables - always use `THANDLE_ASSIGN()`
- Don't forget to call `TARRAY_TYPE_DEFINE()` in source files after declaring in headers
- Match alignment requirements when using `malloc_flex` for variable-sized allocations
- Always check if async operations support cancellation before calling `async_op_cancel()`
- Include `gballoc_hl_redirect.h` in source files to redirect malloc/free calls

## Requirements and Traceability

### SRS Documentation Pattern
- Each module has detailed requirements in `devdoc/{module}_requirements.md`
- Requirements use SRS (Software Requirements Specification) numbering: `SRS_MODULE_NN_NNN`
- Implementation code includes requirement references: `/*Tests_SRS_ASYNC_OP_02_001: [ requirement text ]*/`
- Traceability tool validates that all SRS requirements are implemented and tested

### Traceability Tool Usage
```bash
# Run traceability check (Visual Studio generator only)
cmake . -Drun_traceability=ON
cmake --build . --target c_util_traceability
```
- Automatically runs on builds when `run_traceability=ON` (default)
- Validates bidirectional traceability between requirements, code, and tests
- Reports missing implementations or tests for SRS requirements

## Dependency Management

### Git Submodules
All dependencies are managed as git submodules in `deps/`:
```bash
# Initialize submodules when cloning
git clone --recursive https://github.com/Azure/c-util.git

# Update existing submodules
git submodule update --init --recursive

# Update to latest dependency versions
git submodule update --remote
```

### Dependency Chain
Critical to understand the layered dependency structure:
1. **macro-utils-c**: Fundamental macros for generic programming
2. **c-logging**: Structured logging infrastructure 
3. **c-pal**: Platform abstraction layer (threading, memory, THANDLE)
4. **c-util**: This library (builds on all above)

### External Dependencies
- **smhasher/MurmurHash2**: Required for hash functionality - build fails if submodule not updated
- **umock-c**: Mocking framework for unit tests
- **ctest**: Test runner framework
- **vcpkg**: Package manager for Windows-specific dependencies (jemalloc)

## Code Organization

### Include Order Convention
Standard include order pattern for both headers and source files:
```c
// 1. Standard C headers (conditional for C++)
#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif

// 2. External library headers
#include "macro_utils/macro_utils.h"

// 3. Dependency headers (c-pal, c-logging)
#include "c_pal/thandle.h"
#include "c_logging/logger.h"

// 4. Test framework headers (tests only)
#include "umock_c/umock_c_prod.h"

// 5. Project headers (own module last)
#include "c_util/async_op.h"
```

### IWYU Pragma Usage
Use Include What You Use (IWYU) pragmas to control header dependencies:
- `// IWYU pragma: keep` - Force inclusion of headers that appear unused
- Common in "real" test files: `#include "real_gballoc_hl_renames.h" // IWYU pragma: keep`

## Documentation
Detailed requirements for each module are in `devdoc/*.md` with SRS (Software Requirements Specification) numbering for traceability.
