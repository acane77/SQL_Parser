#pragma once
/*
by Miyuki, Licensed under GPL v3
*/

#ifndef __MIYUKI_JSON_HPP__
#define __MIYUKI_JSON_HPP__

#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <exception>
#include <functional>
#pragma warning (disable:4996)

namespace Json {
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <exception>
#include <functional>

#pragma warning (disable:4996)

	//#define TEST

	using namespace std;

	class Array;
	class Json;
	class Object;

	class Entry {
	public:
		enum { JSON_ENTRY = 0, ARRAY_ENTRY };
		int    type;
		void * entry;
		Entry(int _type);
	};

	class Array {
	public:
		vector<Object*>* t;
		Array();
		Object& operator[](int i);
		int size();
		void insert(Object * o);
		Array * copy();
	};

	class Json {
	public:
		map<string, Object *>* t;
		Json();

		Object& operator[](const char * key) {
			auto it = t->find(key);
			if (it == t->end()) {
				throw "Index not found.";
				//return *(t->begin()->second);
			}
			return *(it->second);
		}
		int size() {
			return t->size();
		}
		void insert(string name, Object * o);
		Json * copy();
	};

	class Object {
	public:
		enum {
			STRING = 0, NUMBER, BOOL, JSON_OBJ, ARRAY_OBJ, _NULL,
			SIGN
		};
		union obj_value {
			string*value_string = nullptr;
			double value_number;
			bool   value_bool;
			Json * jsonObject;
			Array* arrayObject;
			char   value_sign;
			obj_value() {}
			~obj_value() {}
		};
		int       type;
		obj_value val;
		Object(int _type, void * lexval);
		Object(char sign);
		Object(const Object& obj);
		bool operator ==(Object obj);
		bool operator ==(char sign);

		bool isPointerValues() { return type == JSON_OBJ || type == ARRAY_OBJ || type == STRING; }
		bool hasValue() { return val.value_string; }
		void _ass_p() { if (isPointerValues() && hasValue()) delete (void *)val.value_string; }

		string& asString() { return *val.value_string; }
		double  asNumber() { return val.value_number; }
		bool    asBool() { return val.value_bool; }
		Json&   asJson() { return *val.jsonObject; }
		Array&  asArray() { return *val.arrayObject; }

		Object& operator[](int i) {
			return asArray()[i];
		}
		Object& operator[](const char * key) {
			return asJson()[key];
		}

		operator bool() { return asBool(); }
		operator int() { return (int)asNumber(); }
		operator float() { return (float)asNumber(); }
		operator double() { return (double)asNumber(); }
		operator const char*() { return asString().c_str(); }
		operator Json() { return asJson(); }
		operator Array() { return asArray(); }

		Object& setNumber(double d) { val.value_number = d; type = NUMBER; return *this; }
		Object& setString(string s) { *val.value_string = s; type = STRING; return *this; }
		Object& setString(const char* s) { _ass_p(); val.value_string = new string(s); type = STRING; return *this; }
		Object& setBool(bool b) { val.value_bool = b; type = BOOL; return *this; }
		Object& setJson(Json* json) { _ass_p(); val.jsonObject = json;  type = JSON_OBJ; return *this; }
		Object& setArray(Array* array) { _ass_p(); val.arrayObject = array;  type = ARRAY_OBJ; return *this; }
		Object& setNull() { type = _NULL; return *this; }

		friend ostream& operator<<(ostream& os, Object& obj);

		Object * copy();
	};

	Entry::Entry(int _type) {
		type = _type;
		if (_type == JSON_ENTRY)
			entry = new Json();
		else if (_type == ARRAY_ENTRY)
			entry = new Array();
	}

	void deleteObj(Object * o) {
		if (!o || !o->val.value_string) //检查val_string地址是否为nullptr其实就等于检查整个val是不是为0，如果为0就直接返回避免下面去遍历
			return;
		if (o->type == Object::JSON_OBJ) {
			for (auto e : *(o->val.jsonObject->t))
			{
				deleteObj(e.second);
				delete e.second;
			}
		}
		else if (o->type == Object::ARRAY_OBJ) {
			for (auto e : *(o->val.arrayObject->t))
			{
				deleteObj(e);
				delete e;
			}
		}
	}

	class Exception : public exception {
	public:
		void * entry_addr = nullptr;
		int entry_type = 0;
		char msg[128];
		Exception() {

		}
		Exception(void * _entry_addr, int _entry_type, const char * what) :entry_addr(_entry_addr), entry_type(_entry_type) {
			strcpy(msg, what);
		}
		const char * what() {
			return msg;
		}
		~Exception() {
			if (!entry_addr)
				return;
			if (entry_type == Object::ARRAY_OBJ)
				deleteObj(new Object(Object::ARRAY_OBJ, entry_addr));
			else if (entry_type == Object::JSON_OBJ)
				deleteObj(new Object(Object::JSON_OBJ, entry_addr));
			delete entry_addr;
		}
	};

	Object::Object(int _type, void * lexval = nullptr) {
		type = _type;
		if (!lexval)
			return;
#ifdef TEST
		printf("Entering Object::Object\ntype=%d, lexval=%d\n", _type, lexval);
#endif
		if (_type == STRING)
			val.value_string = (string *)lexval;
		else if (_type == NUMBER)
			val.value_number = *(double *)lexval;
		else if (_type == BOOL)
			val.value_bool = *(bool *)lexval;
		else if (_type == JSON_OBJ)
			val.jsonObject = (Json *)lexval;
		else if (_type == ARRAY_OBJ)
			val.arrayObject = (Array *)lexval;
		else if (_type == _NULL);
		else if (_type == SIGN)
			val.value_sign = *(char *)lexval;
	}

	Object::Object(char sign) {
		type = SIGN;
		val.value_sign = sign;
	}



	Object::Object(const Object& obj) {
#ifdef TEST
		puts("Object(&)");
#endif
		type = obj.type;
		switch (type) {
		case STRING:
			val.value_string = obj.val.value_string;
			break;
		case NUMBER:
			val.value_number = obj.val.value_number;
			break;
		case BOOL:
			val.value_bool = obj.val.value_bool;
			break;
		case JSON_OBJ:
			val.jsonObject = obj.val.jsonObject;
			break;
		case ARRAY_OBJ:
			val.arrayObject = obj.val.arrayObject;
			break;
		case SIGN:
			val.value_sign = obj.val.value_sign;
			break;
		}
	}

	bool Object::operator ==(Object obj) {
		if (type != obj.type)
			return false;
		if (type == Object::SIGN)
			return obj.val.value_sign == val.value_sign;
		return true;
	}

	bool Object::operator ==(char sign) {
		return sign == val.value_sign;
	}

	Object * Object::copy() {
		Object * obj = nullptr;
		if (type == STRING)
			obj = new Object(STRING, new string(*val.value_string));
		else if (type == NUMBER)
			obj = new Object(NUMBER, new double(val.value_number));
		else if (type == BOOL)
			obj = new Object(BOOL, new bool(val.value_bool));
		else if (type == ARRAY_OBJ)
			obj = new Object(ARRAY_OBJ, val.arrayObject->copy());
		else if (type == JSON_OBJ)
			obj = new Object(JSON_OBJ, val.jsonObject->copy());
		return obj;
	}

	Array * Array::copy() {
		Array * tmp = new Array();
		for (auto e : *t) {
			tmp->t->push_back(e->copy());
		}
		return tmp;
	}

	Json * Json::copy() {
		Json * tmp = new Json();
		for (auto it : *t) {
			tmp->t->insert({ it.first, it.second->copy() });
		}
		return tmp;
	}

	void Array::insert(Object* o) {
		t->push_back(o->copy());
	}

	void Json::insert(string name, Object * o) {
		t->insert({ name, o->copy() });
	}

	ostream& operator<<(ostream& os, Object& obj) {
		if (obj.type == Object::STRING)
			os << obj.asString();
		else if (obj.type == Object::NUMBER)
			os << obj.asNumber();
		else if (obj.type == Object::BOOL)
			os << (obj.asBool() ? "true" : "false");
		else if (obj.type == Object::_NULL)
			os << "null";
		else if (obj.type == Object::ARRAY_OBJ)
			os << "a Array() object.";
		else if (obj.type == Object::JSON_OBJ)
			os << "a Json() object";
		return os;
	}

	Json::Json() {
		t = new map<string, Object *>;
	}

	Array::Array() {
		t = new vector<Object *>;
	}

	Object& Array::operator[](int i) {
		return **(t->begin() + i);
	}

	int Array::size() {
		return t->size();
	}

	class lexer {
		char   peek;
		//char * str;
		int   line;
		int   column;
		ifstream file;
		bool   isFile;
		string  srcstr;
		int     str_iterator = -1;

		int retract();
		int getNextChar();
		bool isDigit(int ch);
		bool isLetter(int ch);
		bool isDigitOrLetter(int ch);
	public:
		lexer(char * filename, bool isFileName = true);
		~lexer();
		Object * scan();
		int getLine();
		int getColumn();
	};

	lexer::lexer(char* filename, bool isFileName) :isFile(isFileName) {
		if (isFileName) {
			file.open(filename, std::ios_base::in);
			if (!file) {
				throw "Error while opening file.";
			}
		}
		else
			srcstr = filename;
		peek = ' ';
		line = 0;
		column = 0;
		str_iterator = -1;
	}

	Object * lexer::scan() {
#ifdef TEST
		puts("lexer::scan()");
#endif
		peek = getNextChar();
	restart:
		//忽略空格
		for (; ; peek = getNextChar()) {
			if (peek == ' ' || peek == '\t' || peek == '\r')
				continue;
			else if (peek == '\n') {
				line++;
				column = 0;
			}
			else
				break;
		}
		//识别字符串
		if (peek == '"' || peek == '\'') {
			std::string buf;
			char end_char = peek;
			while ((peek = getNextChar()) != end_char) {
				buf += peek;
				if (peek == '\n' || file.eof())
					throw "Syntax error: missing terminal character.";
			}
			//printf("\nin scan(), string=%s", buf.c_str());
			return new Object(Object::STRING, new string(buf));
		}
		if (isLetter(peek)) {
			std::string buf = "";
			for (; isDigitOrLetter(peek); peek = getNextChar()) {
				buf += peek;
			}
			retract();
			if (buf == "true")
				return new Object(Object::BOOL, new bool(true));
			else if (buf == "false")
				return new Object(Object::BOOL, new bool(false));
			else if (buf == "null")
				return new Object(Object::_NULL, new int(0));
			return new Object(Object::STRING, new string(buf));
		}
		//忽略注释
		if (peek == '/') {
			peek = getNextChar();
			if (peek == '/') {
				for (; ; peek = getNextChar()) {
					if (peek == '\n') {
						line++;
						column = 0;
						peek = getNextChar();
						goto restart;
						break;
					}
				}
			}
			else if (peek == '*') {
				for (; ; peek = getNextChar()) {
					if (file.eof())
						throw "Syntax error: Unterminated comment.";
					if (peek == '\n') {
						line++;
						column = 0;
					}
					if (peek == '*') {
						peek = getNextChar();
						if (peek == '/') {
							peek = getNextChar();
							goto restart;
							break;
						}
						else
							peek = retract();
					}
				}
			}
			else peek = retract();
		}
		//Digit
		if (isDigit(peek) || (peek == '.')) {
			int dot = 0;
			double val = 0;
			for (; isDigit(peek) || peek == '.'; peek = getNextChar()) {
				if (dot) {
					dot++;
					if (peek == '.')
						throw "Too many decimal points in number.";
				}
				if (peek == '.') {
					dot++;
					continue;
				}
				val = val * 10 + peek - '0';
			}
			retract();
			for (int i = 0; i<dot - 1; i++)
				val /= 10;
			return new Object(Object::NUMBER, new double(val));
		}
		//Others
		return new Object(Object::SIGN, new char(peek));
	}

	bool lexer::isDigit(int ch) {
		return ch >= '0' && ch <= '9';
	}

	bool lexer::isLetter(int ch) {
		return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
	}

	bool lexer::isDigitOrLetter(int ch) {
		return isDigit(ch) || isLetter(ch);
	}


	int lexer::getNextChar() {
		column++;
		if (!isFile) {
			return srcstr[++str_iterator];
		}
		char ch;
		file.get(ch);
		return ch;
	}

	int lexer::retract() {
		column--;
		if (!isFile) {
			return srcstr[--str_iterator];
		}
		char ch;
		file.unget().unget();
		file.get(ch);
		return ch;
	}

	int lexer::getLine() {
		return line;
	}
	int lexer::getColumn() {
		return column;
	}

	lexer::~lexer() {
		if (file)
			file.close();
	}

	/*
	J -> '{' P '}'
	{ J.syn=new Entry(JSON); P.inh=J.syn }
	| [ E ]
	{ J.syn=new Entry(ARRAY); E.inh=J.syn;}
	L -> s : O
	{ P.inh->insertNode(s.lexval, O.syn, NULL); }
	P -> s : O , P'
	{ P.inh->insertNode(s.lexval, O.syn); }
	| ε {  }
	O -> s {O.syn=new Object(s.lexval); }
	| n {O.syn=new Object(n.lexval) }
	| t {O.syn=new Object(t.true) }
	| f {O.syn=new Object(t.false); }
	| null {O.syn=new Object(null.null); }
	| J { O.syn=J.syn }
	E -> O , E'
	{ E.inh=E'.inh;
	E.inh->insertNode(O.syn, E'.syn); }
	| ε {  }
	*/

	class parser {
		lexer *  lex;
		Object * lookahead;

		Entry * J();
		void    P(Json * inh);
		Object* O();
		void    E(Array * inh);
		template <class T>
		void match(T terminal, Exception * e = nullptr);
	public:
		parser(char * filepath, bool isFileName = true);
		Entry * GetJson();
		int     getLine();
		int     getColumn();
	};

	parser::parser(char * filepath, bool isFileName) {
		lex = new lexer(filepath, isFileName);
		if (!lex)
			throw "Create lexer failed";
		lookahead = lex->scan();
	}

	template <class T>
	void parser::match(T terminal, Exception * e) {
		if (*lookahead == terminal)
			lookahead = lex->scan();
		else
			throw e;
	}

	int parser::getLine() {
		return lex->getLine() + 1;
	}

	int parser::getColumn() {
		return lex->getColumn() + 1;
	}

	Entry * parser::J() {
		Entry * syn = nullptr;
		if (*lookahead == '{') {
			match('{');
			syn = new Entry(Entry::JSON_ENTRY);
			P((Json *)syn->entry);
			match('}', new Exception(nullptr, -1, "'}' expected."));
		}
		else if (*lookahead == '[') {
			match('[');
			syn = new Entry(Entry::ARRAY_ENTRY);
			E((Array *)syn->entry);
			match(']', new Exception(nullptr, -1, "']' expected."));
		}
		else
			throw new Exception(nullptr, -1, "A JSON object or an array object is required.");
		return syn;
	}

	void parser::P(Json* inh) {
		string   name;
		Object * obj = nullptr;
		if (*lookahead == Object(Object::STRING)) {
			name = *(lookahead->val.value_string);
			match(Object(Object::STRING), new Exception((void *)inh, Object::JSON_OBJ, "A property name is required here."));
			match(':', new Exception((void *)inh, Object::JSON_OBJ, "',' Expected"));
			obj = O();
			(*(inh->t))[name] = obj;
			//if (obj->type==0)
			//    printf("[string]=%s, ", obj->asString().c_str());
			if (*lookahead == ',') {
				match(',');
				P(inh);
			}
		}
	}

	void parser::E(Array* inh) {
		Object * syn = nullptr;
		Object * obj_save = lookahead;
		Entry * ent = nullptr;
		if (*lookahead == Object(Object::STRING)) {
			match(Object(Object::STRING));
			syn = obj_save;
			inh->t->push_back(syn);
			if (*lookahead == ',') {
				match(',');
				E(inh);
			}
		}
		else if (*lookahead == Object(Object::NUMBER)) {
			match(Object(Object::NUMBER));
			syn = obj_save;
			inh->t->push_back(syn);
			if (*lookahead == ',') {
				match(',');
				E(inh);
			}
		}
		else if (*lookahead == Object(Object::BOOL)) {
			match(Object(Object::BOOL));
			syn = obj_save;
			inh->t->push_back(syn);
			if (*lookahead == ',') {
				match(',');
				E(inh);
			}
		}
		else if (*lookahead == Object(Object::_NULL)) {
			match(Object(Object::_NULL));
			syn = obj_save;
			inh->t->push_back(syn);
			if (*lookahead == ',') {
				match(',');
				E(inh);
			}
		}
		else if (*lookahead == '{') {
			match('{');
			ent = new Entry(Entry::JSON_ENTRY);
			P((Json *)ent->entry);
			inh->t->push_back(new Object(Object::JSON_OBJ, (Json*)ent->entry));
			match('}', new Exception((void *)inh, Object::ARRAY_OBJ, "'}' expected"));
			if (*lookahead == ',') {
				match(',');
				E(inh);
			}
		}
		else if (*lookahead == '[') {
			match('[');
			ent = new Entry(Entry::ARRAY_ENTRY);
			E((Array *)ent->entry);
			inh->t->push_back(new Object(Object::ARRAY_OBJ, (Array*)ent->entry));
			match(']', new Exception((void *)inh, Object::ARRAY_OBJ, "']' expected"));
			if (*lookahead == ',') {
				match(',');
				E(inh);
			}
		}
	}

	Object* parser::O() {
		Object * syn = nullptr;
		Object * obj_save = lookahead;
		if (*lookahead == Object(Object::STRING)) {
			match(Object(Object::STRING));
			syn = obj_save;
		}
		else if (*lookahead == Object(Object::NUMBER)) {
			match(Object(Object::NUMBER));
			syn = obj_save;
		}
		else if (*lookahead == Object(Object::BOOL)) {
			match(Object(Object::BOOL));
			syn = obj_save;
		}
		else if (*lookahead == Object(Object::_NULL)) {
			match(Object(Object::_NULL));
			syn = obj_save;
		}
		else {
			Entry * t = J();
			if (t->type == Entry::JSON_ENTRY)
				syn = new Object(Object::JSON_OBJ, (Json*)t->entry);
			else if (t->type == Entry::ARRAY_ENTRY)
				syn = new Object(Object::ARRAY_OBJ, (Array*)t->entry);
		}
		return syn;
	}

	Entry * parser::GetJson() {
		return J();
	}

	class JsonWriter {
	public:
		Entry * e = nullptr;

		JsonWriter(Entry * _e) :e(_e) {

		}
		void travel(ofstream& os, bool encodeJson = false, function<void(Object * o, ostream& os)> visit = [](Object * o, ostream& os) {
			if (o->type == Object::STRING)
				os << "\"" << *o << "\"";
			else
				os << *o;
		}) {
			if (!e)
				return;
			Object * o = nullptr;
			if (e->type == Entry::JSON_ENTRY) {
				o = new Object(Object::JSON_OBJ, e->entry);
			}
			else if (e->type == Entry::ARRAY_ENTRY) {
				o = new Object(Object::ARRAY_OBJ, e->entry);
			}
			if (!o)
				return;
			travel(o, visit, encodeJson, 0, os);
		}
		void travel(Object * o, function<void(Object * o, ostream& os)> visit, bool encodeJson, int height, ostream& os) {
			if (!o)
				return;
			if (o->type == Object::JSON_OBJ) {
				if (encodeJson) {
					os << "{";
				}
				for (auto e : *(o->val.jsonObject->t)) {
					os << endl;
					for (int i = 0; i < height + 1; i++)
						os << "\t";
					os << "\"" << e.first << "\": ";
					travel(e.second, visit, encodeJson, height + 1, os);
					os << ",";
				}
				os.seekp(-1, ios_base::cur);
				os << "\n";
				if (encodeJson) {
					for (int i = 0; i < height; i++)
						os << "\t";
					os << "}" << endl;
				}
			}
			else if (o->type == Object::ARRAY_OBJ) {
				if (encodeJson) {
					os.seekp(-1, ios_base::cur);
					os << "[\n";
				}
				for (auto e : *(o->val.arrayObject->t)) {
					for (int i = 0; i < height + 1; i++)
						os << "\t";
					travel(e, visit, encodeJson, height + 1, os);
					os.seekp(-2, ios_base::cur);
					os << ",\n";
				}
				os.seekp(-1, ios_base::cur);
				os << " \n";
				if (encodeJson)
					os << "]";
			}
			else
				visit(o, os);
		}
		friend ofstream& operator << (ofstream& os, JsonWriter& jw) {
			jw.travel(os, true);
			return os;
		}
	};

	class JsonScanner {
		Entry * e;
		int type;
		bool read_success;

	public:
		Array * array;
		Json *  json;

		JsonWriter * jw = nullptr;

		JsonScanner(char * filepath, bool isFileName = true);
		bool success();
		int size();
		Json& J() { return *json; }
		Array& A() { return *array; }

		bool isArray();
		bool isJson();

		Object& operator[](int i) { return A()[i]; }
		Object& operator[](const char * key) { return J()[key]; }

		void writeBack(ofstream& file) {
			file << *jw;
		}

		void insert(Object * o) {
			A().insert(o);
		}

		void insert(string name, Object * o) {
			J().insert(name, o);
		}

		Entry * getEntry() { return e; }
		~JsonScanner();
	};

	JsonScanner::JsonScanner(char * filepath, bool isFileName) {
		parser p(filepath, isFileName);
		try {
			e = p.GetJson();
			read_success = true;
		}
		catch (Exception * ex) {
			cout << (ex ? ex->what() : "Unkown error") << "\n(at line " << p.getLine() << " column " << p.getColumn() << ")\n";
			read_success = false;
			if (ex)
				delete ex;
			getchar();
			return;
		}
		type = e->type;
		if (type == Entry::JSON_ENTRY)
			json = (Json*)e->entry;
		else if (type == Entry::ARRAY_ENTRY)
			array = (Array*)e->entry;
		else throw "Invalid type";
		jw = new JsonWriter(e);
	}

	bool JsonScanner::isArray() {
		return type == Entry::ARRAY_ENTRY;
	}

	bool JsonScanner::isJson() {
		return type == Entry::JSON_ENTRY;
	}

	int JsonScanner::size() {
		return isArray() ? array->size() : json->size();
	}

	bool JsonScanner::success() {
		return read_success;
	}

	JsonScanner::~JsonScanner() {
		Object * o = nullptr;
		if (isJson())
			o = new Object(Object::JSON_OBJ, json);
		else
			o = new Object(Object::ARRAY_OBJ, array);
		deleteObj(o);
	}
}

#endif