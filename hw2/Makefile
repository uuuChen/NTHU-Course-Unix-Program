all:
	g++ main.cpp -o sandbox
	g++ sandbox_test.cpp -o testfile
	g++ -fPIC -shared -o sandbox.so sandbox.cpp -ldl

clean:
	rm test_rename.txt sandbox sandbox.so testfile test text.txt
