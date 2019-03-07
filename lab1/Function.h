#pragma once
#include <fstream>
#include <cctype>
#include <algorithm>
#include <functional>
#include <stack>
#include <vector>
#include <map>
#include <iostream>
#include <string>

namespace Util {


	class Node {
	public:
		Node(std::string asmView_, double priority) : asmView(asmView_)
		{}

		Node(const char* asmView_, double priority_) : asmView(std::string(asmView_)), priority(priority_)
		{}

		double getPriority() const {
			return priority;
		}

		virtual std::string getAsmView() const {
			return asmView;
		}

	private:
		std::string asmView;
		double priority;
	};

	class Value : public Node {
	public:
		Value(int val) : Node(std::string(""), 0), iValue(val), isVariable(false), isInt(true), isAsmConstant(false)
		{
			std::string iRepres = std::to_string(iValue);
			if (val < 0)
				iRepres.replace(0, 1, "n");
			strRepres = "_i" + iRepres;
		}


		Value(double val) : Node(std::string(""), 0), dValue(val), isVariable(false), isInt(false),  isAsmConstant(false)
		{
			std::string dRepres = std::to_string(dValue);
			auto pos = dRepres.find('.');
			dRepres.replace(pos, 1, "p");
			if (val < 0)
				dRepres.replace(0, 1, "n");
			strRepres = "_d" + dRepres;
		}

		Value() : Node(std::string(""), 0), isVariable(true)
		{}

		Value(std::string &s) : Node(s, 0), isAsmConstant(true), isInt(false), isVariable(false)
		{}

		Value(const char* str) : Node(str, 0), isAsmConstant(true), isInt(false), isVariable(false)
		{}

		bool isIntType() const {
			return isInt;
		}

		std::string getStrRepres() const {
			return strRepres;
		}

		template<class T>
		std::string getGeneratedValue() const
		{
			if (typeid(T) == typeid(int))
				return std::to_string(iValue);
			if (typeid(T) == typeid(double))
				return std::to_string(dValue);
			throw std::exception("Unknown type from getGeneratedValue");
		}

		int isIntValue() const {
			if (isInt)
				return iValue;
			return -1;
		}

		virtual std::string getAsmView() const override
		{
			if (isVariable)
				return "fld x";
			if (isAsmConstant)
				return Node::getAsmView();
			if (isInt)
			{
				return "fild " + strRepres;
			}
			if (!isInt) {
				return "fld " + strRepres;
			}
		}

	private:
		bool isVariable;
		bool isInt;
		bool isAsmConstant;
		int iValue;
		double dValue;
		std::string strRepres = "";

	};

	
	class Function {
	public:
		static std::map<std::string, Node*> operators;

		Function(std::string &str)
		{
			str.erase(std::remove_if(str.begin(), str.end(), std::isspace), str.end());
			expression = minimize(str);
			notation = parse(expression);
		}

		std::string minimize(std::string buffer);

		std::vector<Node*> parse(std::string& expression);

		void print()
		{
			for (auto val : notation)
				if (val)
					std::cout << val->getAsmView() << std::endl;
				else std::cout << "null\n";

		}

		void generateAsmFunction(std::string filename = "") {
			std::vector<std::string> header = {
							"void asm_function(double a, double b, double step, const char *label){\n"
							"double x = a, f = 0.0;",
							"_asm{",
							"finit",
							"fld b",
							"fld x",
							"loop_compute :",
							"fcom",
							"fstsw ax",
							"and ah, 01000101b",
							"jz loop_end"
			};
			
			std::map<std::string, Value*> values;
			for (auto val : notation)
			{
				if (typeid(*val) == typeid(Value))
				{
					Value *value = dynamic_cast<Value*>(val);
					std::string str = value->getStrRepres();
					if (str != "")
						values[str] = value;
				}
			}
			for (auto val : values)
			{
				auto x = val.second;
				if (x->isIntType())
					header.insert(header.begin() + 1, "int " + x->getStrRepres() + " = " + x->getGeneratedValue<int>() + ";");
				else
					header.insert(header.begin() + 1, "double " + x->getStrRepres() + " = " + x->getGeneratedValue<double>() + ";");
			}
			std::vector<std::string> footer = {
							"fadd f",
							"fstp f",
							"fadd step",
							"fst x",
							"jmp loop_compute",
							"loop_end :",
							"fwait",
							"}",
							"printf(\"ASM function value: %lf\\n\", f);",
							"}"
			};
			for (auto x : notation)
				header.push_back(x->getAsmView());
			for (auto x : footer)
				header.push_back(x);
			std::string preheader = "#include <math.h>\n #include <stdio.h>\n #include <stdlib.h>\n #include <time.h>\n #include <Windows.h>\n #include <conio.h>\n #include <iostream>\n#define pi 3.1415926 \n \n \n void c_function(double a, double b, double step, const char *label)\n {\n double x = a,\n f = 0;\n while(x <= b){\n ";
			preheader += "f+= " + (expression) + ";\n";

			preheader += "x += step;\n }\n std::cout << label << \" value: \"<< f << std::endl;\n  }\n \n \n ";

			header.insert(header.begin(), preheader);
			std::string postheader = "\n \n  void time_measure(void(*fun)(double, double, double, const char*), double startR, double end, double step, const char *label)\n  {\n  LARGE_INTEGER frequency, start, finish;\n  QueryPerformanceFrequency(&frequency);\n  QueryPerformanceCounter(&start);\n  fun(startR, end, step, label);\n  QueryPerformanceCounter(&finish);\n  float delay = (finish.QuadPart - start.QuadPart) * 1000.0f / frequency.QuadPart;\n  printf(\"%s time elapsed : %f ms\\n\", label, delay);\n  }\n \n  int main()\n  {\n  double start, end, step;\n  std::cout << \"Enter start of range : \";\n  std::cin >> start;\n  std::cout << \"Enter end of range : \";\n  std::cin >> end;\n  std::cout << \"Enter step of range : \";\n  std::cin >> step;\n  if ((end - start) / step <= 0)\n  {\n  std::cout << \"Incorrect input!\" << std::endl;\n  goto end;\n  }\n  time_measure(&c_function, start, end, step,   \"C   function\");\n  time_measure(&asm_function, start, end, step, \"ASM function\");\n end:\n  system(\"pause\");\n  return 0;\n  }\n \n ";
			header.push_back(postheader);
			if (filename == "")
				for (auto x : header)
					std::cout << x << std::endl;
			else
			{
				std::ofstream file(filename);
				for (auto x : header)
					file << x << std::endl;
			}
		}

		std::string getExpression() const {
			return expression;
		}

	private:
		std::vector<Node*> notation;
		std::string expression;


	};

}
