#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <bitset>
#include <cstdint>
#include <algorithm>
#include <cassert>
#include <tuple>
#include <span>
#include <ranges>
#include <bit>
#include <stdlib.h>

using namespace std;


class Cache {
private:
	unsigned blockSize;
	unsigned associativity;
	unsigned capacity;
	unsigned setOffset;
	unsigned tagOffset;
	unsigned setFind;
	vector<uint64_t> tags;
	vector<char> invalid;
	vector<char> valid;
	vector<int> priority;


	int64_t memAccess = 0;
	int64_t misses = 0;
	int64_t invalidWB = 0;

	//Read input file
	ifstream infile;

public:


	//Constructor
	Cache(string input, unsigned bs, unsigned a, unsigned c) {
		infile.open(input);

		blockSize = bs;
		associativity = a;
		capacity = c;

		auto blocks = capacity / blockSize;

		tags.resize(blocks);
		invalid.resize(blocks);
		valid.resize(blocks);
		priority.resize(blocks);

		//Calculate num of offset bits
		int blockBits = popcount(blockSize - 1);

		//Calculate num of set bits
		setOffset = blockBits;
		auto sets = capacity / (blockSize * associativity);
		setFind = sets - 1;
		int setBits = popcount(setFind);


		//Calculate tag offset
		tagOffset = blockBits + setBits;
	}
	//Destructor
	~Cache() {
		infile.close();
		stats();
	}


	void run() {
		string currLine;

		while (getline(infile, currLine)) {
			//Get access data
			auto [type, address] = readLine(currLine);


			//check for hits in cache
			auto hit = hitCheck(type, address);


			updateStats(hit);
		}
	}

	tuple<bool, uint64_t> readLine(string currLine) {
		string intype;
		uint64_t address;
		int type;

		string tempAddress;
		string other;

		stringstream ss(currLine);
		getline(ss, intype, ' ');
		ss << hex << currLine;
		ss >> address;
		getline(ss, tempAddress, ' ');
		getline(ss, other, ' ');


		if (intype == "l")
			type = 0;

		else
			type = 1;

		return { type, address };
	}

	int getSet(uint64_t address) {
		auto shiftedAdress = address >> setOffset;
		return shiftedAdress & setFind;
	}

	uint64_t getTag(uint64_t address) {
		return address >> tagOffset;
	}

	bool hitCheck(bool type, uint64_t address) {
		auto set = getSet(address);
		auto tag = getTag(address);
		auto base = set * associativity;

		if (invalid.size() > 0) {
			span localTags{ tags.data() + base, associativity };
			span localInvalid{ invalid.data() + base, associativity };
			span localValid{ valid.data() + base, associativity };
			span localPriority{ priority.data() + base, associativity };


			auto hit = false;
			int invalidIndex = -1;
			int index;
			for (int i = 0u; i < localValid.size(); i++) {
				if (!localValid[i]) {
					invalidIndex = i;
					continue;
				}

				if (tag != localTags[i])
					continue;

				hit = true;
				index = i;

				localInvalid[index] |= type;

				break;
			}

			auto invalidWb = false;
			if (!hit) {
				//Try invalid line
				if (invalidIndex >= 0) {
					index = invalidIndex;
					localValid[index] = 1;
				}

				//Evict lowest priority cache block
				else {
					auto maxElement = ranges::max_element(localPriority);
					index = distance(begin(localPriority), maxElement);
					invalidWb = localInvalid[index];
				}

				//update tag and invalid state
				localTags[index] = tag;
				localInvalid[index] = type;
			}

			transform(begin(localPriority), end(localPriority), begin(localPriority), [&](int p) {
				if (p <= localPriority[index] && p < associativity)
					return p + 1;
				else
					return p;
				});

			localPriority[index] = 0;

			return { hit };
		}
	}


	void updateStats(bool hit) {
		memAccess++;
		misses += !hit;
	}

	void stats() {
		cout << "CACHE INFO" << endl;
		double numBlocks = capacity / blockSize;
		double numSets = numBlocks / (blockSize * associativity);
		cout << "Number of sets: " << numSets << endl;

		cout << "Number of blocks in each set: " << associativity << endl;
		cout << "Block size in Bytes: " << blockSize << endl;

		if ((numSets == 1) && (associativity >= 1))
			cout << "Associativity: Fully Associative" << endl;

		else if ((numSets >= 1) && (associativity == 1))
			cout << "Associativity: Direct-Mapped " << endl;

		else
			cout << "Associativiy: " << associativity << "-Way Set Associative" << endl;

		cout << "Cache Size in Bytes: " << capacity << endl;


		cout << "\nCACHE HIT RATE " << endl;
		auto hits = memAccess - misses;
		double hitRate = (double)hits / (double)memAccess * 100.0;
		cout << "Hit rate: " << hitRate << "%" << endl;
		cout << "Misses: " << misses << endl;
		cout << "Hits: " << hits << endl;

	}
};



int main() {
	bool program = true;
	int blockIn;
	int associativeIn;
	int capIn;

	cout << "Enter the power of the block size (e.g. 2^x): " << endl;
	cin >> blockIn;
	unsigned blockSize = pow(2, blockIn);

	cout << "Enter the power of the associativity (e.g. 2^x): " << endl;
	cin >> associativeIn;
	unsigned associativity = pow(2, associativeIn);

	cout << "Enter the power of the capacity of the cache (e.g. 2^x): " << endl;
	cin >> capIn;
	unsigned capacity = pow(2, capIn);


	while (program) {
		string trace;
		cout << "\nInput the name of the trace file you would like to read with the extension (ex: [name].trace)" << endl;
		cin >> trace;

		//Default values for cache
		//Default values for cache
	
		Cache analysis(trace, blockSize, associativity, capacity);

		analysis.run();

	}

	return 0;
}
 