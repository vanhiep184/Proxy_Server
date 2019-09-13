#include "pch.h"
#include "Cache.h"

using namespace std;

//Khởi tạo cache với số byte đã sử dụng bởi người dùng = 0
Cache::Cache() {
	bytesUsed = 0;
}

//Mẫu singleton
Cache* Cache::getInstance()
{
	if (m_instance == NULL)
	{
		m_instance = new Cache();
	}

	return m_instance;
}

//5 mins for 1 URL
UINT FiveMinLife(LPVOID Item)
{
	string URL = (char*)Item;
	Cache* DL = Cache::getInstance();
	
	Sleep(300000);
	
	DL->Dele(URL);
	cout << "THE URL: " << URL << " has just been deleted" << endl;
	
	return 0;
}

/**
 * Insert a CacheItem into the cache. If the cache doesn't have enough room to insert the item, the least recently used (LRU) replacement policy is used to remove one or more items.
 * @param item - the item to insert
 */
void Cache::insert(CacheItem* item) {
	
	//Check xem có tồn tại URL này chưa
	int index = search(item->url);

	// If the item isn't already in the cache, add it
	if (index == -1) {
		//Nếu tối đa vùng nhớ của cache thì không lưu
		if (item->responseSize > maxSize) {
			cerr << endl << "ERROR: Response for the following URL exceeds maximum cache size (" << maxSize << "): " << item->url << endl << endl;
			return;
		}

		// If there isn't room to add the item without removing other items, keep removing the least
		// recently used item until there's enough room
		while (item->responseSize > maxSize - bytesUsed) {
			CacheItem* lastItem = cache.back();

			bytesUsed -= lastItem->responseSize;

			cache.erase(cache.end() - 1);

			delete lastItem;
		}

		// Insert the item at the front so it becomes the new most recently used item
		cache.insert(cache.begin(), item);

		//Tăng bytes được sử dụng lên
		bytesUsed += item->responseSize;
		//Chạy 1 luồng bắt đầu tính thời gian
		AfxBeginThread(FiveMinLife, (LPVOID)item->url.c_str());
		return;
	}
	//Nếu đã tồn tại thì đưa nó lên đầu cho lần tìm kiếm tiếp theo nhanh chóng hơn
	else if (index != 0) {
		cache.erase(cache.begin() + index);

		cache.insert(cache.begin(), item);
	}
}


//Xóa URL này ra khỏi cache
int Cache::Dele(string URL)
{
	int index = search(URL);
	if (index != 0) {
		cache.erase(cache.begin() + index);
		return 0;
	}
	return 1;
}
/*
 * If an item is in the cache, return a pointer to it and make it the most recently used item.
 * @param  url  - the URL to search for
 * @return item - a pointer to the CacheItem if found, otherwise nullptr
 */
CacheItem* Cache::access(const string& url) {
	CacheItem* item = nullptr;


	//Tìm kiếm xem URL này có trong cache?
	int index = search(url);

	// Nếu đã tồn tại thì trả về vị trí của nó
	if (index != -1) {
		item = cache[index];

		// Nếu không phải là vị trí đầu tiên thì nhích nó lên làm vị trí đầu tiên để tìm kiếm cho lẹ
		if (index != 0) {
			cache.erase(cache.begin() + index);

			cache.insert(cache.begin(), item);
		}
		
	}
	//Nếu không tồn tại trả về -1
	return item;
}

/**
 * Search the vector for a CacheItem with the provided URL.
 * NOTE: This does not lock!
 * @param  url   - the URL to search for
 * @return index - the index of the CacheItem, or -1 if none is found
 * @private
 */
int Cache::search(const string& url) {
	int index = -1;

	for (int i = 0; i < cache.size(); i++) {
		if (cache[i]->url == url) {
			index = i;
			break;
		}
	}

	return index;
}