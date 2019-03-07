#include <iostream>
#include "Function.h"
#include <Windows.h>

#include <math.h>

void check()
{
	double x = 2, y = 1,
		f = 0,
		realF = 1 * log2f(x);
	int _i2 = 2;
	_asm {
		finit


		fld x
		fld y


		fstp f
		fwait
	}
	std::cout << "res     = " << f << std::endl;
	std::cout << "realres = " << realF << std::endl;
}

void foo() {

}

int main()
{
	std::string expression, file;
	std::cout << "Enter expression to test: " << std::endl;
	std::cin >> expression;
	std::cout << "Enter file to generate(write \"cmd\" to print): " << std::endl;
	std::cin >> file;
	if (file == "cmd")
		file = "";
	try {
		Util::Function func(expression);
		func.generateAsmFunction(file);
	}
	catch (const std::exception &ex)
	{
		std::cout << "Exception!!" << std::endl;
		std::cout << ex.what() << std::endl;
	}
	
	system("pause");
	return 0;
}