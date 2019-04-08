myhttpd: myhttpd.o 
	g++ -pthread -o myhttpd myhttpd.cpp intlist.cpp my_read_write.cpp -g

myhttpd.o: myhttpd.cpp
	g++ -c myhttpd.cpp

intlist.o: intlist.cpp
	g++ -c intlist.cpp

my_read_write.o: my_read_write.cpp
	g++ -c my_read_write.cpp

clean:
	rm myhttpd
	rm *.o
