all:mycrawler jobExecutor

mycrawler: mycrawler.o
	g++ -pthread -o mycrawler mycrawler.cpp strlist.cpp ../myhttpd/my_read_write.cpp -g

mycrawler.o: mycrawler.cpp
	g++ -c mycrawler.cpp

strlist.o: strlist.cpp
	g++ -c strlist.cpp

jobExecutor: jobExecutor.o trie.o list.o commands.o searchlist.o intlist.o
	g++ -o jobExecutor jobExecutor.cpp trie.cpp list.cpp commands.cpp searchlist.cpp intlist.cpp -g

jobExecutor.o: jobExecutor.cpp
	g++ -c jobExecutor.cpp

trie.o: trie.cpp
	g++ -c trie.cpp

list.o: list.cpp
	g++ -c list.cpp

commands.o: commands.cpp
	g++ -c commands.cpp

searchlist.o: searchlist.cpp
	g++ -c searchlist.cpp

intlist.o: intlist.cpp
	g++ -c intlist.cpp

my_read_write.o: ../myhttpd/my_read_write.cpp
	g++ -c ../myhttpd/my_read_write.cpp

clean:
	-rm jobExecutor
	-rm mycrawler
	-rm -r save_dir
	-rm *.o
	-rm docfile
	-rm tmp/*
	-rm log/*