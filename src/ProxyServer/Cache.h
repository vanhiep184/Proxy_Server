#ifndef __Cache_hpp__
#define __Cache_hpp__

#include <cstdlib>
#include <vector>

#include <errno.h>
//#include <pthread.h>

#include "CacheItem.h"
#include <chrono>

// Tham khảo 
//https://github.com/zziccardi/proxy?files=1&fbclid=IwAR2mmCd8pkCiODTqTzAftSWybZZ2pKP3nCjk3LKE_NXu5IzNRUAAmKtueIg


//Tạo mẫu singleton để có duy nhất 1 cache
class Cache {
private:
	Cache();
	~Cache()
	{
		cache.clear();
	}
	static Cache* m_instance;
	int   bytesUsed;
	//	pthread_mutex_t   lock;
	int search(const string& url);
	vector<CacheItem*> cache;
	
public:
	//Tối đa 300 Megabytes (not Mebibytes)  = 30000000 bytes
	int maxSize = 30000000;
	static Cache* getInstance();
	//UINT FiveMinLife(LPVOID Item);
	void       insert(CacheItem* item);
	int Dele(string URL);
	CacheItem* access(const string& url);
};



#endif
