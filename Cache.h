#pragma once
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
	unsigned setMask;
	vector<uint64_t> tags;
	vector<char> invalid;
	vector<char> valid;
	vector<int> priority;


	int64_t writes = 0;
	int64_t memAccess = 0;
	int64_t misses = 0;
	int64_t invalidWB = 0;
	int64_t instructions_ = 0;

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
		setMask = sets - 1;
		int setBits = popcount(setMask);


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
			auto [type, address, instruction] = parseLine(currLine);


			//Probe cache
			auto [hit, invalidWb] = hitCheck(type, address);
			

			updateStats(instruction, type, hit);
		}
	}

	tuple<bool, uint64_t, int> parseLine(string currLine) {
		string intype;
		uint64_t address;
		int instruction;
		int type;

		string tempAddress;
		string tempInstruction;

		stringstream ss(currLine);
		getline(ss, intype, ' ');
		ss << hex << currLine;
		ss >> address;
		getline(ss, tempAddress, ' ');
		getline(ss, tempInstruction, ' ');
		instruction = stoi(tempInstruction);


		if (intype == "l") 
			type = 0;
		
		else
			type = 1;	
		
		return { type, address, instruction };
	}

	int getSet(uint64_t address) {
		auto shiftedAdress = address >> setOffset;
		return shiftedAdress & setMask;
	}

	uint64_t getTag(uint64_t address) {
		return address >> tagOffset;
	}

	tuple<bool, bool> hitCheck(bool type, uint64_t address) {
		auto set = getSet(address);
		auto tag = getTag(address);
		auto base = set * associativity;

		span localTags{tags.data() + base, associativity };
		if (invalid.size() > 0) {
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

		return { hit, invalidWb };
		}
	}


	void updateStats(int instructions, bool type, bool hit) {
		memAccess++;
		writes += type;
		misses += !hit;
		instructions_ += instructions;
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

