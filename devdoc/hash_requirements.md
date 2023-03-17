`hash` requirements
============

## Overview

`hash` is a module that abstracts computing a hash for the purpose of its APIs being usable from C code (The Murmur hash implementation being used in the project is C++).

## Exposed API

```c
MOCKABLE_FUNCTION(, int, hash_compute_hash, const void*, buffer, size_t, length, uint32_t*, hash);
```

### hash_compute_hash

```c
MOCKABLE_FUNCTION(, int, hash_compute_hash, const void*, buffer, size_t, length, uint32_t*, hash);
```

`hash_compute_hash` computes a hash over a buffer using Murmur hash.

**SRS_HASH_01_001: [** `hash_compute_hash` shall call `MurmurHash2`, while passing as arguments `buffer`, `length` and 0 as seed. **]**

**SRS_HASH_01_003: [** On success `hash_compute_hash` shall return 0 and fill in `hash` the computed hash value. **]**

**SRS_HASH_01_004: [** If `buffer` is NULL, `hash_compute_hash` shall fail and return a non-zero value. **]**

**SRS_HASH_01_005: [** If `length` is 0, `hash_compute_hash` shall fail and return a non-zero value. **]**

**SRS_HASH_01_006: [** If `hash` is NULL, `hash_compute_hash` shall fail and return a non-zero value. **]**

**SRS_HASH_01_002: [** If `length` is greater than or equal to INT_MAX, `hash_compute_hash` shall fail and return a non-zero value. **]**
