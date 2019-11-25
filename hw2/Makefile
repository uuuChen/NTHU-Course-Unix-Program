all:
	g++ main.cpp -o sandbox
	g++ test.cpp -o testfile
	g++ -fPIC -shared -o sandbox.so sandbox.cpp -ldl

clean:
	rm test_rename.txt testfile sandbox sandbox.so
