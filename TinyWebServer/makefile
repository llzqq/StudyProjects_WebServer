server: main.cpp ./threadpool/threadpool.h ./http/http_conn.cpp ./http/http_conn.h ./lock/locker.h ./log/log.cpp ./log/log.h ./log/block_queue.h ./CGImysql/sql_connection_pool.cpp ./CGImysql/sql_connection_pool.h
	g++ -o server main.cpp ./threadpool/threadpool.h ./http/http_conn.cpp ./http/http_conn.h ./lock/locker.h ./log/log.cpp ./log/log.h ./CGImysql/sql_connection_pool.cpp ./CGImysql/sql_connection_pool.h -lpthread -lmysqlclient

lzq_in_log: test_lzq/in_log.cpp
	g++ -o lzq_in_log test_lzq/in_log.cpp

lzq_in_threadpool: test_lzq/in_threadpool.cpp
	g++ -o lzq_in_threadpool test_lzq/in_threadpool.cpp

clean:
	rm  -r server
