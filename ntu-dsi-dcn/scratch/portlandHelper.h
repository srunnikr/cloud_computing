#include <iostream>
#include <string>
#include <stdlib.h>
#include <map>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;
using namespace std;

// Class for common utility functions
class portlandHelper {

public:
	portlandHelper() {
	}

	uint8_t* ipToBytes (string ip) {

		int len = 16;
		uint8_t* arr = (uint8_t *) malloc (len * sizeof(uint8_t));
		memset(arr, 0, len);

		const char* arrTemp = ip.c_str();
		memcpy(arr, arrTemp, ip.length());
		return arr;

	}

	string bytesToIp (char* bytes) {

		string ip(bytes, 16);
		return ip;

	}

	uint8_t* macToBytes (string mac) {

		int len = 18;
		uint8_t* arr = (uint8_t *) malloc (len * sizeof(uint8_t));
		memset(arr, 0 , len);

		const char* arrTemp = mac.c_str();
		memcpy(arr, arrTemp, mac.length());
		return arr;

	}

	string bytesToMac (char* bytes) {

		string mac(bytes, 18);
		return mac;

	}

};
