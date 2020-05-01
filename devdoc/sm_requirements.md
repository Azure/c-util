# State manager requirements
================

## Overview

`State manager` (short:`sm`) is a module the manages the call state for the APIs of a module under the following sematics:

1. the module has 2 step initialization (that is, the module has a _create and a _open APIs)

2. the module's APIs are callable after _open has executed (_open being the exception - it can only be called after _create)

3. the module has a _close function that reverts the effects of _open and allows calling _open again

4. the APIs that can be called in the _open state can be further divided into 2 categories

   a) APIs that can be called in parallel (for example a _read API).

   b) APIs that are barrier. (for example _set_content, _flush_all etc). Barrier APIs have the following semantics
      
       i) there can only be 1 barrier API at any given time

       ii) the barrier API can only execute when all the previous APIs have stopped executing

       iii) no other APIs (including other barriers) can start to execute after the current barrier (that means those APIs will fail)

5. there's no "parking", "queueing", "sleeping", "waiting", "buffering", "saving" or otherwise postponing or delaying the execution of an API: it either starts to execute (for barries execution includes waiting for all the previous APIs to finish executing) or it fails. Retrying is a user-land mechanism.

