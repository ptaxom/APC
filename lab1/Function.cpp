#include "Function.h"

namespace Util {
	std::map<std::string, Node*> Function::operators = {
			{"+", new Node("fadd", 1)},
			{"-", new Node("fsub", 1)},
			{"*", new Node("fmul", 2)},
			{"/", new Node("fdiv", 2)},
			{"^", new Node("fxch st(1) \nfyl2x \nfld st(0) \nfrndint \nfsub st(1), st \nfxch st(1) \nf2xm1 \nfld1 \nfadd \nfscale \nfstp st(1)", 3)},
			{"sin", new Node("fsin", 3)},
			{"cos", new Node("fcos", 3)},
			{"sqrt", new Node("fsqrt", 3)},
			{"tg", new Node("fptan", 3)},
			{"ctg", new Node("fsincos\nfdivr", 3)},
			{"log2", new Node("fld1 \nfxch st(1) \nfyl2x", 3)},

	};		

	Util::Node* getOp(std::string &s)
	{
		auto val = Function::operators[s];
		if (!val)
			std::cout << "NULL Here " << s << std::endl;
		return val;
	}

	Util::Node* getOp(char c)
	{
		std::string s(1, c);
		return getOp(s);
	}

	bool isBasicOp(char c) {
		return c == '+' || c == '-' ||
			c == '*' || c == '/' ||
			c == '^';
	}

	bool isBasicFunction(std::string &s)
	{
		return  s == "sin"  || s == "cos" ||
				s == "sqrt" || s == "tg"  ||
				s == "ctg"  || s =="log2";
	}

	bool isLog(std::string &s) {
		return s.substr(0, 3) == "log";
	}

	Node* getValue(std::string &buffer) {
		if (buffer == "1" || buffer == "1.0")
			return new Value("fld1");
		if (buffer == "0" || buffer == "0.0")
			return new Value("fldz");
		if (buffer == "pi")
			return new Value("fldpi");
		if (buffer == "x")
			return new Value("fld x");
		try
		{
			double v = std::stod(buffer);
			if ((int)v - v == 0)
				return new Value((int)v);
			return new Value(v);
		}
		catch (const std::exception &ex)
		{
		}
		return nullptr;
	}


	int getNextClosingBracket(std::string &s, int i)
	{
		std::stack<char> brackets;
		for (; i < s.length(); i++)
		{
			if (s[i] == '(')
			{
				brackets.push('(');
				continue;
			}
			if (s[i] == ')')
			{
				if (brackets.empty())
					throw std::logic_error("Incorrect bracket sequance");
				brackets.pop();
				if (brackets.empty())
					return i;
			}
		}
		if (!brackets.empty())
			throw std::logic_error("Incorrect bracket sequance");
	}

	int getNextClosingAbs(std::string &s, int i)
	{
		int absCounter = 0;
		int lastEvenPos = i;
		for (; i < s.length(); i++)
		{
			if (s[i] == '|')
			{
				++absCounter;
				if (absCounter % 2 == 0)
					lastEvenPos = i;
			}
		}
		if (absCounter % 2 == 1)
			throw std::exception("Incorrect abs");
		return lastEvenPos;
	}



	std::string eraseExponent(std::string str, int start)
	{
		start++;
		if (str[start] == '(')
		{
			int end_substr = getNextClosingBracket(str, start + 1);
			auto debug =  str.substr(start, end_substr - start + 1);
			return debug;
		}
		else
		{
			std::string debug = "";
			for (; start < str.length() && !isBasicOp(str[start]); start++)
				debug += str[start];
			return debug;
		}
	}

	void push_back(std::vector<Node*> &to, std::vector<Node*> &from)
	{
		for (int i = 0; i < from.size(); i++)
			to.push_back(from[i]);
	}

	std::string createPowFunc(std::string base, std::string exponent)
	{
		Value* exp = dynamic_cast<Value*>(getValue(exponent));
		int sz;
		if (exp && (sz = exp->isIntValue()) >= 0)
		{
			if (sz == 0)
				return "1";
			std::string buffer = "";
			for (int i = 0; i < sz - 1; i++)
				buffer += "(" + base + ")*";
			buffer += "(" + base + ")";
			return buffer;
		}
		std::string ans = "pow(" + base;
		ans += ",";
		ans += exponent + ")";
		return ans;
	}

	std::string Function::minimize(std::string buffer)
	{
		for (int i = 0; i < buffer.length(); i++)
		{
			char c = buffer[i];
			if (c == '|')
			{
				int end_subexp = getNextClosingAbs(buffer, i);
				std::string substr = buffer.substr(i + 1, end_subexp - i - 1);
				std::string subexpr = "abs(" + minimize(substr) + ")";
				buffer.replace(i, end_subexp - i + 1, "?");
				buffer.replace(i, 1, subexpr);
				i += subexpr.length() - 1;
				continue;
			}
			if (c == '(')
			{
				if (i > 0 && !isBasicOp(buffer[i - 1]))
					continue;
				int expIndex = getNextClosingBracket(buffer, i) + 1;
				if (buffer[expIndex] == '^')
				{
					std::string base = buffer.substr(i+1, expIndex - i - 2);
					std::string exponent = eraseExponent(buffer, expIndex);
					std::string repres = minimize(createPowFunc(base, exponent));
					int len = (base + exponent).length();
					buffer.replace(i, len + 3, "?");
					buffer.replace(i, 1, repres);
					i += len + 6;
					continue;
				}
			}
			if (c == '^')
			{
				//std::cout << buffer << std::endl;
				int j = i-1;
				for (; j >= 0 && !isBasicOp(buffer[j]); j--);
				std::string base = buffer.substr(j+1, i - j - 1);
				std::string exponent = eraseExponent(buffer, i);
				std::string repres = minimize(createPowFunc(base, exponent));
				int len = (base + exponent).length();
				buffer.replace(j+1, len + 1, "?");
				//std::cout << buffer << std::endl;
				buffer.replace(j+1, 1, repres);
				//std::cout << buffer << std::endl;
				i = j + len + 7;
				continue;
			}
		}
		return buffer;
	}

	std::vector<Node*> Util::Function::parse(std::string &expression)
	{
		std::vector<Node*> arr;
		std::stack<Node*> operators;
		std::string buffer = "";
		int start = 0;

		if (expression[0] == '-')
		{
			arr.push_back(new Value(-1));
			operators.push(Function::operators["*"]);
			start = 1;
		}

		for (int i = start; i < expression.length(); i++)
		{
			char c = expression[i];
			if (c == '|')
			{
				int end_subexp = getNextClosingAbs(expression, i);
				std::string substr = expression.substr(i + 1, end_subexp - i - 1 );
				std::vector<Node*> subarr = parse(substr);
				push_back(arr, subarr);
				arr.push_back(new Node("fabs", 1));
				i = end_subexp;
				continue;

			}
			if (c == '(')
			{
				int end_subexp = getNextClosingBracket(expression, i);
				std::string substr = expression.substr(i + 1,  end_subexp - i - 1 );
				std::vector<Node*> subarr = parse(substr);
				push_back(arr, subarr);
				i = end_subexp;
				continue;
			}
			if (c == '-' && expression[i - 1] == '(')
			{
				arr.push_back(new Value(-1));
				operators.push(Function::operators["*"]);
				continue;
			}
			if (isBasicOp(c))
			{
				auto op = getOp(c);
				while (!operators.empty() && operators.top()->getPriority() >= op->getPriority())
				{
					arr.push_back(operators.top());
					operators.pop();
				}
				operators.push(op);
				continue;
			}
			buffer += c;
			if (i + 1 < expression.length() && 
				(isBasicOp(expression[i + 1]) || expression[i + 1] == '(')
				)
			{				
				if (isBasicFunction(buffer))
				{
					int end_subexp = getNextClosingBracket(expression, i+1);
					std::string substr = expression.substr(i + 2, end_subexp - i - 2);
					std::vector<Node*> subarr = parse(substr);
					push_back(arr, subarr);
					arr.push_back(getOp(buffer));
					i = end_subexp;
					buffer = "";
					continue;
				}
				Node* node = getValue(buffer);
				if (node)
					arr.push_back(node);
				else throw std::exception("Incorrect expression");
				buffer = "";
			}
		}
		if (!buffer.empty())
		{
			Node *val = getValue(buffer);
			if (val)
				arr.push_back(val);
			else throw std::exception("Incorrect expression");
		}
		while (!operators.empty())
		{
			arr.push_back(operators.top());
			operators.pop();
		}

		return arr;
	}

}