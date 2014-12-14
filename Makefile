
dsmc: main.cpp irc.hpp word.hpp database.hpp reader.hpp voice.hpp gutenbergparser.hpp
	clang++ -g -std=c++11 -stdlib=libc++ main.cpp -o dsmc

release: main.cpp irc.hpp word.hpp database.hpp reader.hpp voice.hpp gutenbergparser.hpp
	clang++ -O3 -std=c++11 -stdlib=libc++ main.cpp -o dsmc