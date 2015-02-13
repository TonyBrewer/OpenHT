Tests:

dsn1 - Simple PersSuInc. Port 0 reads and writes with one outstanding request.
dsn2 - Streaming data simple test. PersSuInc read data with Mif0.
dsn3 - Same as dsn1 but with PersQuXyz
dsn4 - Same as dsn1 but with reads to shared variables (scalar, memory and queue) plus indexing.
dsn5 - PersSuInc - 1 outstanding rd/wr, four DstId's
dsn6 - PersSuInc - 4 outstanding rd/wr, four DstId's
dsn7 - CXR test fairly simple
dsn8 - CXR test more interesting
dsn9 - Dsn8 with a different GroupName per module
dsn10 - dsn9 with multiple entry points
