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

clean:
	rm jobExecutor
	rm *.o
	rm tmp/*
	rm log/*
