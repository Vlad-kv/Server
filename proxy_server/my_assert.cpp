#include "my_assert.h"

void my_assert(bool result, std::string mess) {
	if (result) {
		std::cout << mess << "\n";
		throw new std::runtime_error(mess);
	}
}
