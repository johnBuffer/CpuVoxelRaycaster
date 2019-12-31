#pragma once

#include <iostream>
#include <fstream>
#include <list>


struct ReplayElements
{
	float timestamp;
	float x;
	float y;
	float z;
	float view_x;
	float view_y;

	
	static std::list<ReplayElements> loadFromFile(const std::string& filename) {
		std::ifstream file;
		file.open(filename);

		std::list<ReplayElements> result;
		if (file) {
			ReplayElements elem;
			while (file >> elem.timestamp >> elem.x >> elem.y >> elem.z >> elem.view_x >> elem.view_y) {
				result.push_back(elem);
			}
		}

		std::cout << result.size() << " ticks loaded" << std::endl;

		return result;
	}

};

