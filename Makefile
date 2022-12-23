build:
	@gcc socket.c -o socket
	@gcc sharedmem.c -lpthread -o sharedmem

clean:
	@rm socket sharedmem -f
	@rm received-files/* -f

run-socket:
	./socket 5 SourceFile.png && sleep 0.5

run-sharedmem:
	./sharedmem 5 SourceFile.png && sleep 0.5

help:
	@echo make clean
	@echo make build
	@echo make run-socket
	@echo make run-sharedmem
	@echo ""
	@echo ALT-
	@echo "./socket <number-of-peers> <filename>"
	@echo "./sharedmem <number-of-peers> <filename>"
