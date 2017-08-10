#ifndef MY_ASSERT_H
#define MY_ASSERT_H

void my_assert(bool result, std::string mess) {
	if (result) {
		std::cout << mess << "\n";
		throw new std::runtime_error(mess);
	}
}

#endif // MY_ASSERT_H
