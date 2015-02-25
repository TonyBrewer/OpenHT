kmc-hammr -- High-capacity Automated Memcached Measurement Resource

Functionally, it is designed to throw large amounts of traffic at a memcached instance and measure various performance metrics -- namely transaction latency, transaction throughput, and network bandwidth.  In doing so, it can be very brutal to the server in question, even moreso if specifically configure to be.

The program itself has many warts and not a few bugs.  Almost no error checking is done and there are a number of edge cases, especially in the thread synchronization code, that can cause the program to run one second longer than specified.  It is still a work in progress and should be expected to break at any time with no warning and no explanation why or what you did wrong.

invocation:  ./mc-hammr <config file>

The config file defines a set of sessions, one per line.  Each session sets up one or more client connections to the server and runs set of transactions, then reports the stats for that session.

====== session options =======

send:  controlls the type of request sent to the memcached server.  Options are SET, SETQ, GET, GETQ, MGET, and MGETQ.  The 'q' versions of the get/set commands are "quiet".  They do not require a response unless there is an error.  These two are not well tested at this time and should not be used. Unless otherwise specified all notes apply to both standard and quiet versions of the operations.

*In binary mode, the GET and MGET commands are effectively equivelent.

*For SET, any non error response is considered a cache hit.  For the GET command a response indicating the key exists and returning data is considered a hit.  A non response or a "key does not exist" response is considered a miss.

*For GET hits, the data is not validated for correctness.  Each binary request to the memcached server includes a sequence number, similar to the TCP sequence number, in the header opaque field.  This sequence number is checked on each server response and if it matches the sequence number of an outstanding request, then the data is considered valid is the packet indicates the key was found.  For ASCII mode operations, the key is returned in the response and validated against the key requested.  If the key matches and the data is the correct length (number of bytes) then the response is considered a hit.


recv:  Sets the recieve mode.  Options are sync or async.  In synchronous mode, a single thread handles each client connection.  A request is sent to the server and the thread waits for the response before sending the next request.  There will never be more than one request outstanding on each client connection.  This mode provides the most accurate measure of request/response latency at the cost of per connection throughput.  In asynchronous mode there is a thread handling the sending of requests and a seperate thread handling receipt for each client connection.  This can provide much higher throughput, though under some conditions it can artifically inflate the reported latency.

conns_per_thread:  Sets the number of simultaneous connections to the client handled by each thread.  Total client connections will be conns_per_thread*threads.  Each connection will run a full set of transaction as defined in the rest of the configuration options.  For example, if loop=5 is set, each connection will loop through all keys five times and if rate=1000 is set, each connection will send transactions at a rate of 1000 per sec so the cumulative rate will be (conns*rate) transactions per second.  Each connection requires a TCP socket and a file handle for that socket so running a high connection count may require special considerations for open file limits.  

threads:  Set the number of client threads.  Total client connections will be conns_per_thread*threads.  In async recv mode, the actual number of program threads will be twice the specified value as their will be one thread handling sending and one thread handling receipt.  Linux counts threads against a users process limit so running with a very high connection count may require special considerations for process and stack limits

key_prefix:  A string that is prepended to each key to allow for unique keys for each client session definition.  This prefix is shared by all client connections spawned by the session.

key_len:  Sets the length of the key used in each request.  This key is made up of the key_prefix value and a key index value that is 0 extended to fill the defined key length.  The key index value is actually put in the key as a hex string.

key_count:  Sets the number of keys available to be used in a particular session.  If the loop setting is used, this is the number of transactions set to the server for each loop.  In default sequential key mode, after one loop, keys 0 through (n-1) will be sent in transactions where n is the key_count specified.  To ensure all operations are hits when they should be, the key count should be evenly divisible by the number of keys in a single send().

value_size:  defines the amount of data to be stored in the cache in a put command.  This value cannot exceed 65504 bytes at this time as the application level send and recieve buffers are a fixed 64k.  No real data is placed in the buffer, the actual data sent to the server is a bunch of zero bytes.  This value can be defined for get sessions however it has no realy effect.

host:  defines the memcached server.  This value must be an IP address.

src: defines the source address for the client bind. This value must be an IP address if specified.  If not specified, the client will bind to INADDR_ANY.  

port: defines the port on which to contact the memcached server.  This is the destination port.  The client source port is dynamically assigned.

rate: specifies a rate, in requests per second, at which to send transactions to the server.  This rate is only a guide due to a number of reasons.  This value is used to calculate a delay between sending transactions.  In synchronous mode, this delay is applied after receipt of a response and before transmission of the next request.  However, the latency of the previous request is not taken into account when calculating the delay rate so the actual delay between sending to transactions will be t_latency+(1/rate).  In asynchronous mode the delay will be reasonably accurate.  Accurracy of the actual delay will be subject to timer accurracy for usleep as well as things like kernel scheduling delays in high thread count situations.  If specified in the same session line as a delay parameter, the last one encountered when parsing the session options from left to right.

delay: specify a delay in microseconds between the sending of transactions to the server.  See the description of the rate setting for limitations and details.

loop:  Specify a number of loops through the key set to run before exiting.  This value is overriden by the time option.  Setting loop=0 will run the session indefinately.

jitter:  Add a randomness to the delay imposed by the rate or delay options.  Value is in micro seconds.  Setting the jitter option cause the actual delay between sending transactions to vary psuedo randomly +/-(jitter/2) from the actual delay.  In the case where the delay-(jitter/2)<0 then the actual delay will be [0,jitter) usec.  Jitter can be specified without specifying delay, in which case delay is assumed to be 0.

out: defines the number of transactions allowed outstanding in asynchronous mode.  This option has no effect in synchronous mode.  Special care should be used in setting this option when measuring latency as it will artificially increase the measured latency when out>2.  Above 2, the latency will increase by lat*(n-1), where latency is the measure latency of a single packet in sequential mode.  Under certain circumstances, higher throughput , both in transactions per second and network bandwidth, may be achievable with a higher outstanding transaction count at the cost of high individual packet latency.

*Note:  When ops_per_send is greater than one, the application will ensure that out is an integer multiple of ops_per_send by increasing it to the next integer multiple of ops_per_send if the specified value does not divide evenly by ops_per_send.

fork: setting to a non zero value causes the program to fork a new process to handle the session.  This allows multiple sessions to run in parallel.  If fork=0 then the session is run to completion prior to starting the next session in the configuration.  Sessions with fork=0 will complete before the next session begins.  Sessions with fork=1 will start in parallel and the following sessions is immediately started.

time:  Sets the number of seconds to run a particular session.  This overrides the loop value if specified.  If the time specified is greater than the time to complete one loop through the key set then multiple loops will be run.  If the time specified is less than one loop, then not all keys will be used.  This is important when running a session of PUT commands to pre-populate the cache as using a time value that is less than a full pass through all the keys will not fully populate the cache and can cause misses in GET sessions that follow.

tx_res:  sets the number of outstanding request allow before sending to the server is resumed once the value specified by out is hit.  This is the hysteresis point for sending requests.  By default, if not specied, it is calculated as (out/2) requests. 

rnd: set to a non zero value to use random key values.  By default the key value is the hex string representation of an index that starts at 0 and increments to (key_count-1), prefixed with the value of key_prefix.  Setting rnd=1 (or non-zero) causes some randomness in the selected key value.  Warning, the randomness sucks!  But, the keys are no longer sequential and there is no guarantee that all keys [0,key_count) will be used in a loop, only that key_count transactions will be sent.  The rnd option should not be set for session designed to pre-populate the cache.

ops_per_send:  Specify the number of operations batched together in a single call to send().  This effectively sets the number of keys requested in a multiget operation.  There is not a different operation command for GET vs MGET.  In ASCII mode, the value sets the number of keys appended to a GET request.  As there is technically no binary multiget, in binary mode this sets the number of complete GET request packets that are bundled together for the call to send().

*The code will ensure that the number of keys used is an integer multiple of the ops_per_send value.  It takes the value of key_count and if it is not an even multiple, it will increase it to the next integer multiple of ops_per_send.  In the case of a GET this means it is possible for the client to send a key that will result in a miss if the SET was done with a different ops_per_send value.

ops_per_conn:  By default, the client opens a connection to the server and uses that connection for the lifetime of the session.  This may or may not mirror behavior of a real world client.  Ops_per_conn sets the number of operations processesed before shutting down the connection and re-establishing it.  The client will scale this value up to the next integer multiple of ops_per_send if the value specified does not divide evenly into ops_per_send.

proto: Sets binary or ASCII protocol mode.  In the case of operations not technically defined under a given mode (MGET under binary protocol), the operation will be mapped to the best available operation.  ASCII MGET is mapped to multiple binary GETs packed in a single send().

mcast_wait:  This is a special configuration option used for coordinating multple mc-hammr instances across multiple client machines.  When set to a value of 1, the program will initialize all internal data structures and then wait for a message on a multicast socket.  Currently the multicast group is hardcoded as 225.0.0.37 and the port is 12346.  The program waits for the simple message "go" on this group/port.
