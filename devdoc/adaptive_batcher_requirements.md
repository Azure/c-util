`adaptive_batcher` requirements
============

## Overview

`adaptive_batcher` is a module that batches payloads together while adapting to changes observed in the system.

A naive batcher would take all payloads available every x ms and batch them.
This technique implies specifying the interval of time used for batching, which implies that any batch can have up to x ms increased latency.

A better solution is to create batches that are small enough (not waiting for more payloads and thus not introducing artificial delays), while not exceeding the bounds of the system with regards to how many batches/s can be processed (when system bounds are exceeded latency typically suffers).

Thus the inputs we have to the module are:
- payload sizes
- measured latency of each payload

The adaptive batching module would have to perform the following actions:
- each time a payload needs to be processed, the module will decide based on the known desired batch size whether the payload gets batched or not.
- evaluate if desired batch size should be increased or decreased. The rules should be:
  - if no change in latency is perceived, decrease the batch size by a given percent/amount.
  - if an increase in latency is perceived, increase the batch size (up to an upper bound)

## Exposed API

```c

```