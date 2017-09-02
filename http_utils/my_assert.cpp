#include "my_assert.h"

void my_assert(bool result, std::string mess) {
	if (result) {
		std::cout << mess << "\n";
		throw std::runtime_error(mess);
	}
}
