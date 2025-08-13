break main
break server_create
break server_start
break accept_connections
break io_thread_run
break handle_read
break handle_write
break worker_thread

set print pretty on
set print array on
set pagination off

define print_conn
    print *(connection_t*)$arg0
end

define print_server
    print *(reactor_server_t*)$arg0
end

define print_stats
    print "Total connections:", server->total_connections
    print "Worker tasks completed:", server->worker_pool->tasks_completed
end
