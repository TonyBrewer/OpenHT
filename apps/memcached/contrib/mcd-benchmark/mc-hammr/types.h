#ifndef __TYPES__
#define __TYPES__

typedef struct conn_stats_struct conn_stats_t;
typedef struct thread_stats_struct thread_stats_t;
typedef struct process_stats_struct process_stats_t;

typedef enum enum_req_type req_type_t;
typedef enum enum_proto proto_t;
typedef enum enum_recv_mode recv_mode_t;
typedef enum enum_thread_mode thread_mode_t;
typedef enum enum_cfg_key cfg_key_t;
typedef struct config_struct config_t;

typedef enum enum_conn_read_state conn_read_state_t;
typedef enum enum_conn_resp_state conn_resp_state_t;
typedef enum enum_conn_send_state conn_send_state_t;
typedef struct outstanding_req_struct outstanding_req_t;
typedef struct connection_struct connection_t;

typedef struct udp_frame_header_struct udp_frame_header_t;
typedef struct thread thread_t;
typedef struct frame_struct frame_t;
typedef struct request_struct request_t;

#endif
