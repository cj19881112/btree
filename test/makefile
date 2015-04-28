all : clean run

test.out : ../*.c
	clang++ -D_GTEST_ -o $@ $^ -lgtest_main -lgtest -lpthread -g

run : test.out
	./test.out

clean :
	rm -f test.out
