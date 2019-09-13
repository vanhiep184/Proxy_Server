#include "pch.h"
#include "CacheItem.h"

//Tạo 1 dữ kiện để bắt
CacheItem::CacheItem(const string url, const string response, const int responseSize) {
	this->url = url;
	this->response = response;
	this->responseSize = responseSize;
}