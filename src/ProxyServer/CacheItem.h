#ifndef __CacheItem_hpp__
#define __CacheItem_hpp__

#include <iostream>
#include <string>
#include <ctime>
#include<WinSock2.h>
#include<stdlib.h>
#include<stdio.h>
#include<string>
#include <stack>

// Tham khảo 
//https://github.com/zziccardi/proxy?files=1&fbclid=IwAR2mmCd8pkCiODTqTzAftSWybZZ2pKP3nCjk3LKE_NXu5IzNRUAAmKtueIg

using namespace std;

class CacheItem {
public:
	string url;
	string response;	  // file Rep form server
	int    responseSize;  // size of the entire response in bytes
//	time_t TimeLife;

	CacheItem(const string url, const string response, const int responseSize);
	~CacheItem() { url.clear(); response.clear(); responseSize = 0; }
};


#endif