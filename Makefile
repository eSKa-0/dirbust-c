ifeq ($(OS),Windows_NT)
	CURL_INC=libs/windows/curl-8.13.0_5-win64-mingw/include
	CURL_LIB=libs/windows/curl-8.13.0_5-win64-mingw/lib
	CURL_FLAGS=-I$(CURL_INC) -L$(CURL_LIB) -lcurl
else
	CURL_FLAGS=-lcurl
endif

all:
	gcc dirbust.c libs/universal/tinycthreads/tinycthread.c $(CURL_FLAGS) -o dirbust