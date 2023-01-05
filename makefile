objects = src/httphandle.c src/queue.c src/server.c src/threadpool.c

build : $(objects)
		mkdir src/out ; gcc -o src/out/webserver $(objects)
			
clean :	 
	cd src && rm -r out