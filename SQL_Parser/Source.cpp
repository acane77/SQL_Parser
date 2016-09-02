#include <iostream>
#include <map>
#include <unordered_map>
#include <string>
#include <set>
#include <exception>
#include <vector>
#include <sstream>
#include <cstdio>
#include "Json.hpp"

//#define DEBUG

using namespace std;

class SyntaxError : public std::exception {
public:
	std::string err_msg;
	SyntaxError(const char * msg);
	const char * what();
};

SyntaxError::SyntaxError(const char * msg):err_msg(msg) {
	
}

const char * SyntaxError::what() {
	return err_msg.c_str();
}

class Tags {
public:
	enum {
		NUMBERS = 256, STRING, WORD,
		//Keys
		SELECT, FROM, WHERE, AND, OR, NOT, INSERT, INTO, VALUES, VALUE,
		DELETE, UPDATE, SET, MIYUKI, LIKE, MATCHES, CREATE, TABLE, 
		K_NUMBER, K_STRING, DROP, VIEW,
		//Operators
		ANY, 
		__RELOP_START__, 
			LT, LTE, GT, GTE, EQ, NEQ,
		__RELOP_END__,
		LB, RB, ASSIGN, COMMA, COLON,
		//END
		__END_OF_STRING__,
		//USE FOR 2ND PHARSE OF PAESE
		__PROP_NAME__NUMBER__,
		__PROP_NAME__STRING__
	};
};

class String;
class Number;

class token {
public:
	int  tag;

	token(int _tag);
	string toString();
	bool operator==(token& t);
	bool operator==(char * s);
};

token::token(int _tag):tag(_tag) {
}

string token::toString() {
	switch (tag) {
	case Tags::NUMBERS:
		return "Number";
	case Tags::STRING:
		return "String";
	case Tags::AND:
		return "AND";
	case Tags::ANY:
		return "Any";
	case Tags::OR:
		return "Or";
	case Tags::LT:
		return "<";
	case Tags::LTE:
		return "<=";
	case Tags::GT:
		return ">";
	case Tags::GTE:
		return ">=";
	case Tags::EQ:
		return "=";
	default:
		return "(token)";
	}
}

class word :public token {
public:
	std::string lexeme;

	word(std::string _lexeme, int tag);
};

word::word(std::string _lexeme, int tag):token(tag), lexeme(_lexeme) {

}

namespace Words {
	word any("*", Tags::ANY), lt("<", Tags::LT), lte("lte", Tags::LTE), gt(">", Tags::GT), gte(">=", Tags::GTE), eq("=", Tags::EQ), neq("!=", Tags::NEQ),
		lb("(", Tags::LB), rb(")", Tags::RB), assign("=", Tags::ASSIGN), comma(",", Tags::COMMA), __END_OF_STRING__("", Tags::__END_OF_STRING__),
		colon(":", Tags::COLON);
}

class number :public token {
public:
	double val;

	number(double _n);
};

number::number(double _n):val(_n), token(Tags::NUMBERS) {

}

class String :public token {
public:
	std::string lexval;

	String(std::string _str);
	int length() { return lexval.size(); }
};

String::String(std::string _str):lexval(_str), token(Tags::STRING) {

}

bool token::operator==(token& t) {
	if (t.tag != tag)
		return false;
	if (t.tag == Tags::STRING) {
		return ((String *)this)->lexval == ((String *)&t)->lexval;
	}
	else if (t.tag == Tags::NUMBERS) {
		return ((number*)this)->val == ((number *)&t)->val;
	}
	return tag == t.tag;
}

bool token::operator==(char * s) {
	if (tag != Tags::STRING)
		return false;
	else return ((String *)this)->lexval == s;
}

class lexer {
	string sql_str;
	char peak = ' ';
	int index = -1;
	unordered_map<string, word *> symbols;
	inline void readch();
	inline void retract();
	inline bool reach_end();
	void reserve(string id, word * _word);
	word * get(string id);
	bool work_complete = false;
	inline bool is_keyword(string str) { return symbols.find(str) != symbols.end(); }
public:
	lexer(char* sql);
	~lexer();
	token * scan();
	void getAllTokens();
	int getColumn() { return index + 1; }
};

lexer::lexer(char * sql) {
	sql_str = sql;

	//Reserve key words
	reserve("select", new word("select", Tags::SELECT));
	reserve("from", new word("select", Tags::FROM));
	reserve("where", new word("where", Tags::WHERE));
	reserve("and", new word("and", Tags::AND));
	reserve("or", new word("or", Tags::OR));
	reserve("not", new word("not", Tags::NOT));
	reserve("insert", new word("insert", Tags::INSERT));
	reserve("into", new word("into", Tags::INTO));
	reserve("value", new word("value", Tags::VALUE));
	reserve("values", new word("values", Tags::VALUES));
	reserve("delete", new word("delete", Tags::DELETE));
	reserve("update", new word("update", Tags::UPDATE));
	reserve("set", new word("set", Tags::SET));
	reserve("miyuki", new word("miyuki", Tags::MIYUKI));
	reserve("like", new word("like", Tags::LIKE));
	reserve("matches", new word("matches", Tags::MATCHES));
	reserve("create", new word("create", Tags::CREATE));
	reserve("table", new word("table", Tags::TABLE));
	reserve("number", new word("number", Tags::K_NUMBER));
	reserve("string", new word("string", Tags::K_STRING));
	reserve("drop", new word("drop", Tags::DROP));
	reserve("view", new word("view", Tags::VIEW));
}

void lexer::readch() {
	peak = sql_str[++index];
}

void lexer::retract() {
	peak =sql_str[--index];
}

bool lexer::reach_end() {
	return sql_str.size() <= index;
}

void lexer::reserve(string id, word* w) {
	symbols.insert({ id , w });
}

word * lexer::get(string id) {
	unordered_map<string, word*>::iterator it = symbols.find(id);
	if (it != symbols.end())
		return it->second;
	return nullptr;
}

lexer::~lexer() {
	//Release memory
	for (auto it : symbols) {
		delete it.second;
	}
}

token * lexer::scan() {
	if (work_complete)
		return nullptr;
	// Process spaces, tabs, newline and comments
	readch();
	for (; ; readch()) {
		if (reach_end()) {
			work_complete = true;
			return &Words::__END_OF_STRING__;
		}
		if (peak == ' ' || peak == '\t')
			continue;
		else if (peak == '\n') {
			//Do nothing
		}
		else
			break;
	}
	//Digits
	if (isdigit(peak) || (peak == '.')) {
		int dot = 0;
		double val = 0;
		for (; isdigit(peak) || peak == '.'; readch()) {
			if (dot) {
				dot++;
				if (peak == '.')
					throw SyntaxError("在一个数字中存在过多的小数点。");
			}
			if (peak == '.') {
				dot++;
				continue;
			}
			val = val * 10 + peak - '0';
		}
		for (int i = 0; i<dot - 1; i++)
			val /= 10;
		retract();
		return new number(val);
	}
	//String without "
	if (isalpha(peak)) {
		string id = "";
		for (; isalnum(peak) && !reach_end(); readch()) {
			id += peak;
		}
		retract();
		if (is_keyword(id)) {
			return get(id);
		}
		return new String(id);
	}
	//Strings
	if (peak == '"' || peak == '\'') {
		std::string buf;
		char end_char = peak;
		for (readch(); peak != end_char; readch()) {
			buf += peak;
			if (peak == '\n' || reach_end())
				throw SyntaxError("字符串没有关闭");
		}
		return new String(buf);
	}
	//Operators
	if (peak == '=') {
		return &Words::eq;
	}
	if (peak == '!') {
		readch();
		if (peak == '=')
			return &Words::neq;
		retract();
		return new token(peak);
	}
	if (peak == '<') {
		readch();
		if (peak == '=')
			return &Words::lte;
		retract();
		return &Words::lt;
	}
	if (peak == '>') {
		readch();
		if (peak == '=')
			return &Words::gte;
		retract();
		return &Words::gt;
	}
	if (peak == '(')
		return &Words::lb;
	if (peak == ')')
		return &Words::rb;
	if (peak == '*')
		return &Words::any;
	if (peak == ',')
		return &Words::comma;
	if (peak == ':')
		return &Words::colon;
	return new token(peak);
}

void lexer::getAllTokens() {
	token * t;
	while ((t = scan())->tag != Tags::__END_OF_STRING__) {
		cout << t->tag << endl;
	}
}

/*
Term   -> select What from string where Cond
		  { list=GetList(string.lexval);
			Term.val=GenResult(query(Condggtttt.val,list),list);
		  }
	    | insert into string values ( List )
		  { list=GetList(string.lexval);
			Term.val=Insert(string.val,list);
		  }
		| delete * from string where Cond
		  { List=GetList(string.lexval);
		    Term.val=Delete(query(Cond.val,list),list);
		  }
		| update string set Assign where Cond
		  {  List=GetList(string.lexval);
			 Term.val=Update(query(Cond.val,list),list,assignList);
		  }
		| create table string ( Columns )
		  {  CreateList(string.lexval);
		  }
		| drop table string 
		  {  DeleteTable(string lexval); }
		| create view string 
What   -> * { pListAll=true }
		| Plist { pListAll=false }
Cond   -> string Relop object and Cond1
		  {  Cond.val=Gen(string.lexval, Relop.val, object.lexval)+'and'+Cond.val; }
		| string Relop object or Cond1
		  {  Cond.val=Gen(string.lexval, Relop.val, object.lexval)+'or'+Cond.val; }
		| not Cond1 { Cond.val='not' + Cond1.val; }
		| ( Cond1 ) { Cond.val='(' + Cond1.val + ')'; }
		| e
List   -> object , List { InsertList.add(object.lexval); }
		| object { InrtList.add(object.lexval); } }
Assign -> string = string = object, Assign { assignList.add(string, string.lexval) }
		| string { assignList.add(string, string.lexval) }
Plist  -> string, Plist { plist.add(string.lexval) }
		| string { plist.add(string.lexval) }
Relop  -> relop { relop.lexval }
Columns-> string : k_string , Columns 
		  {  AddColumn(string.lexval, k_string.lexval); }
		| string : k_number , Columns 
		  {  AddColumn(string.lexval, k_number.lexval); }
		| e
*/

class tmp_data_field {
public:
	typedef Json::JsonScanner * Table;
	bool select_all_items = false;
	bool list_all_columns = false;
	vector<string> select_colunm_list;
	vector<token *> insertion_list;
	map<string, token *> assign_list;
	vector<token *> condition_list;
	vector<int> index_list;
	vector<pair<token *, token *>> create_table_columns;
	Table table = nullptr;
	string cond_str;
	string table_name;
	string homePath = "k:/";

	vector<int> result_index_array;

	Table GetTable(string str) {
		//cout << "opening " << str;
		table_name = str;
		Json::JsonScanner * t=new Json::JsonScanner(const_cast<char *>((homePath+str+".json").c_str()));
		if (!t)
			int i = 0;
		return t;
	}

	string Gen(token * prop, token * relop, token * object) {
		if (object->tag == Tags::STRING)
			prop->tag = Tags::__PROP_NAME__STRING__;
		else
			prop->tag = Tags::__PROP_NAME__NUMBER__;
		condition_list.push_back(prop);
		condition_list.push_back(relop);
		condition_list.push_back(object);
		return "";
	}

	void Query();

	void writeBack() {
		ofstream file(homePath + table_name + ".json");
		if (!file) {
			cout << "打开文件失败";
			return;
		}
		file << " ";
		table->writeBack(file);
		cout << table->size();
		file.close();
	}

	string getResult() {
		Query();
		if (table->size() == 0 || index_list.size() == 0) {
			cout << "No item matches the condition.";
			return "";
		}
		cout << endl << "Search result: " << endl;
		if (list_all_columns) {
			for (auto e : *(*table)[0].asJson().t)
				cout << e.first << "\t";
			cout << endl << "--------------------------------------------------" <<endl;
			for (auto i : index_list) {
				for (auto e : *(*table)[i].asJson().t)
					cout << *e.second << "\t";
				cout << endl;
			}
			return "";
		}
		for (auto e : select_colunm_list)
			cout << e << "\t";
		cout << endl << "--------------------------------------------------" << endl;
		for (auto i : index_list) {
			for (auto e : select_colunm_list)
				if ((*(*table)[i].asJson().t).find(e) != (*(*table)[i].asJson().t).end())
					cout << (*table)[i][e.c_str()] << "\t";
			cout << endl;
		}
		return "";
	}

	string Insert() {
		stringstream ss;
		ss << "[{ ";
		Json::JsonScanner insertionConfig((char *)(homePath+table_name+".config.json").c_str());
		if (!insertionConfig.success()) {
			cout << "\n读取数据文件配置文件失败，无法判定各列的名称，插入失败。";
			return "";
		}
		if (insertionConfig.size() != insertion_list.size()) {
			cout << "数据量不匹配。";
			return "";
		}
		for (int i = 0; i < insertion_list.size(); i++) {
			if (insertion_list[i]->tag != insertionConfig[i]["type"].asNumber()) {
				cout << "数据类型不匹配。";
				return "";
			}
			ss << "\"" << insertionConfig[i]["name"].asString() << "\" :";
			if (insertion_list[i]->tag == Tags::NUMBERS)
				ss << ((number *)insertion_list[i])->val << ", ";
			else if (insertion_list[i]->tag == Tags::STRING)
				ss << ((String *)insertion_list[i])->lexval << ", ";
			else {
				cout << "这是什么鬼";
				return "";
			}
		}
		ss << " }]";
		char str[512];
		ss.getline(str, 512);
		Json::JsonScanner insertContent(str, false);
		if (!insertContent.success()) {
			cout << "Internal error";
			return "";
		}
		table->insert(insertContent[0].copy());
		cout << "1 rows affected" << endl;
		cout << table->size() << " rows left";
		writeBack();
		return "";
	}

	string Update() {
		Query();
		for (auto i : index_list) {
			for (auto e : assign_list)
				if ((*(*table)[i].asJson().t).find(e.first) != (*(*table)[i].asJson().t).end()) {
					if (!((e.second->tag == Tags::NUMBERS && (*table)[i][e.first.c_str()].type == Json::Object::NUMBER) || (e.second->tag == Tags::STRING && (*table)[i][e.first.c_str()].type == Json::Object::STRING))){
						cout << "type do not match";
						return "";
					}
					if (e.second->tag == Tags::NUMBERS)
						(*table)[i][e.first.c_str()].setNumber(((number *)e.second)->val);
					else if (e.second->tag == Tags::STRING)
						(*table)[i][e.first.c_str()].setNumber(((number *)e.second)->val);
				}
		}
		cout << index_list.size() << " rows affected." << endl;
		cout << table->size() << " rows left";
		writeBack();
		return "";
	}

	string Delete() {
		enum { __DELELED__ = -1 };
		if (select_all_items) {
			table->A().t->clear();
		}
		else {
			Query();
			int i = 0;
			for (auto it : index_list) {
				Json::deleteObj(&(*table)[it]);
				(*table)[it].type = __DELELED__;
			}
			for (int i = 0; i < table->A().t->size(); i++) {
				if ((*(table->A().t))[i]->type == __DELELED__) {
					table->A().t->erase(table->A().t->begin() + i);
					i--;
				}
			}
		}
		cout << index_list.size() << " rows affected." << endl;
		cout << table->size() << " rows left";
		writeBack();
		return "";
	}

	string CreateTable(token * name) {
		cout << create_table_columns.size() << " columns of this table\nTable name: " << ((String *)name)->lexval;
		ofstream config_file((homePath + ((String *)name)->lexval + ".config.json").c_str());
		if (!config_file) {
			cout << "Create configure file failed. Table creation failed.";
			return "";
		}
		ofstream file((homePath + ((String *)name)->lexval + ".json").c_str());
		if (!file) {
			cout << "Create data file failed. Table creation failed.";
			return "";
		}
		file << "[]";
		file.close();
		config_file << "//This file is generated by Miyuki SQL Parser, in order to record table information.\n[  ";
		for (auto item : create_table_columns) {
			config_file << "{ \"name\": \"" << ((String *)item.first)->lexval << "\", \"type\":" << (item.second->tag==Tags::K_NUMBER?Tags::NUMBERS:Tags::STRING) << "},";
			delete item.first;
		}
		config_file.seekp(-1, ios_base::cur);
		config_file << "]";
		config_file.close();
		cout << "\nSuccessfully created file";
		return "";
	}

	string DropTable(token * name) {
		if (!remove((homePath + ((String *)name)->lexval + ".json").c_str()) && !remove((homePath + ((String *)name)->lexval + ".config.json").c_str()))
			cout << "Successfully drop table " << ((String *)name)->lexval;
		else
			cout << "Drop table failed";
		return "";
	}
};

class secParse {
	vector<token *>* cond_t; //原有cond
	vector<token *> cond;
	int it = 0;
	tmp_data_field * t;
	tmp_data_field::Table table;

	token * lookahead = nullptr;

	bool evalRelop(token * relop, token * t1, token * t2=nullptr) {
		if (relop->tag == Tags::AND)
			return ((number *)t1)->val && ((number *)t2)->val;
		if (relop->tag == Tags::OR)
			return ((number *)t1)->val || ((number *)t2)->val;
		if (relop->tag == Tags::NOT)
			return !((number *)t1)->val;
		if (relop->tag == Tags::EQ)
			return *t1 == *t2;
		if (relop->tag == Tags::NEQ)
			return !(*t1 == *t2);
		if (t1->tag == Tags::STRING || t2->tag == Tags::STRING)
			return false;
			//throw "error"
		if (relop->tag == Tags::LT)
			return ((number *)t1)->val < ((number *)t2)->val;
		if (relop->tag == Tags::LTE)
			return ((number *)t1)->val <= ((number *)t2)->val;
		if (relop->tag == Tags::GT)
			return ((number *)t1)->val > ((number *)t2)->val;
		if (relop->tag == Tags::GTE)
			return ((number *)t1)->val >= ((number *)t2)->val;
		return false;
	}
	number * T() {
		if (lookahead->tag == Tags::STRING || lookahead->tag == Tags::NUMBERS) {
			token * t1 = lookahead;
			matchObject();
			token * relop = lookahead;
			matchRelop();
			token * t2 = lookahead;
			matchObject();
			number * res = new number(evalRelop(relop, t1, t2));
			
			if (lookahead->tag == Tags::AND) {
				token * _relop = lookahead;
				match(Tags::AND);
				token * t = T();
				return new number(evalRelop(_relop, t, res));
			}
			else if (lookahead->tag == Tags::OR) {
				token * _relop = lookahead;
				match(Tags::OR);
				token * t = T();
				return new number(evalRelop(_relop, t, res));
			}
			else return res;
		}
		else if (lookahead->tag == Tags::NOT) {
			token * _relop = lookahead;
			match(Tags::NOT);
			token * t = T();
			return new number(evalRelop(_relop, t));
		}
		else if (lookahead->tag == Tags::LB) {
			match(Tags::LB);
			number * res = T();
			match(Tags::RB);
			if (lookahead->tag == Tags::AND) {
				token * _relop = lookahead;
				match(Tags::AND);
				token * t = T();
				return new number(evalRelop(_relop, t, res));
			}
			else if (lookahead->tag == Tags::OR) {
				token * _relop = lookahead;
				match(Tags::OR);
				token * t = T();
				return new number(evalRelop(_relop, t, res));
			}
			else return res;
		}
		return nullptr;
	}
public:
	secParse(vector<token *>* c, tmp_data_field::Table t) :cond_t(c), table(t) {
		
	}
	void replaceVal(int index) {
		for (auto e : cond) {
			if (e)
				delete e;
		}
		cond.clear();
		//将名称替换为实际的值计算
		try {
			for (auto e : *cond_t) {
				if (e->tag == Tags::__PROP_NAME__STRING__) {
					cond.push_back(new String((*table)[index][((String *)e)->lexval.c_str()].asString()));
				}
				else if (e->tag == Tags::__PROP_NAME__NUMBER__) {
					//cout << ((new number((*table)[index][((String *)e)->lexval.c_str()].asNumber()))->val);
					cond.push_back(new number((*table)[index][((String *)e)->lexval.c_str()].asNumber()));
				}
				else if (e->tag == Tags::NUMBERS) {
					//cout << ((number *)e)->val;
					cond.push_back(new number(((number *)e)->val));
				}
				else if (e->tag == Tags::STRING)
					cond.push_back(new String(((String *)e)->lexval));
				else cond.push_back(new token(e->tag));
			}
		}
		catch (const char * ex) {
			throw new SyntaxError(ex);
		}
	}
	bool eval(int index) {
		try {
			replaceVal(index);
			it = 0;
			lookahead = cond[it];
			cond.push_back(new token(Tags::__END_OF_STRING__));
			return T()->val;
		}
		catch(SyntaxError* e){
			throw e;
		} 
		return false;
	}
	token * getToken() { 
		if (it == cond.size())
			throw "Out of range";
		return cond[++it];
	}

	void match(int terminal_tag) {
		if (lookahead->tag == terminal_tag)
			lookahead = getToken();
		else throw SyntaxError("Internal error.");
	}
	void matchObject() {
		if (lookahead->tag == Tags::STRING || lookahead->tag == Tags::NUMBERS)
			lookahead = getToken();
		else throw SyntaxError("Internal error.");
	}
	void matchRelop() {
		if (lookahead->tag > Tags::__RELOP_START__ && lookahead->tag < Tags::__RELOP_END__)
			lookahead = getToken();
		else throw SyntaxError("Internal error");
	}
};

void tmp_data_field::Query() {
	index_list.clear();
	secParse p(&condition_list, table);
	int table_size = table->size();
	if (table->isJson())
		throw "Error data format";
	for (int i = 0; i < table_size; i++) {
		if (select_all_items || p.eval(i))
			index_list.push_back(i);
	}
	cout << endl << "Result: " << index_list.size() << endl;
}

class SQL_Parser;

class parser {
#ifdef DEBUG
public:
#endif
	friend class SQL_Parser;
	token * lookahead = nullptr;
	lexer * lex = nullptr;
	tmp_data_field t;

	void match(token * terminal, SyntaxError * e = nullptr, bool dispose=true);
	void match(int terminal_tag, SyntaxError * e = nullptr, bool dispose = true);
	void matchObject(SyntaxError * e, bool dispose = true);
	void matchRelop(SyntaxError * e, bool dispose = true);
	void matchType(SyntaxError * e, bool dispose = false);

	string T();
	void _WHERE();
	void W();
	string C(int inh_resCall = false);
	void L();
	void A();
	void P();
	void CO();
public:
	parser(char * sql);
	~parser();
	string analyze() { return T(); }
	int getColumn() { return lex->getColumn(); }
};

void parser::_WHERE() {
	if (!lookahead) return;
	if (lookahead->tag == Tags::WHERE) {
		t.select_all_items = false;
		match(Tags::WHERE);
		if (lookahead->tag == Tags::__END_OF_STRING__) {
			throw new SyntaxError("'where' subsentense cannot be empty.");
		}
		string Cval = C();
		t.cond_str = Cval;
	}
	else t.select_all_items = true;
}

string parser::T() {
	token * _t_table_name = nullptr;
	if (lookahead->tag == Tags::SELECT) {
		match(Tags::SELECT);
		W();
		match(Tags::FROM, new SyntaxError("'from' expected."));
		_t_table_name = lookahead;
		match(Tags::STRING, new SyntaxError("Table name expected."), false);
		t.table = t.GetTable(((String*)_t_table_name)->lexval);
		_WHERE();
		match(Tags::__END_OF_STRING__, new SyntaxError("此处应该是结尾"));
		return t.getResult();
	}
	else if (lookahead->tag == Tags::INSERT) {
		match(Tags::INSERT);
		match(Tags::INTO, new SyntaxError("'into' keyword required here"));
		_t_table_name = lookahead;
		match(Tags::STRING, new SyntaxError("Table name expected."), false);
		t.table = t.GetTable(((String*)_t_table_name)->lexval);
		match(Tags::VALUES, new SyntaxError("'values' expected."));
		match(Tags::LB, new SyntaxError("'(' expected."));
		L();
		match(Tags::RB, new SyntaxError("')' expected."));
		match(Tags::__END_OF_STRING__, nullptr);
		return t.Insert();;
	}
	else if (lookahead->tag == Tags::DELETE) {
		match(Tags::DELETE);
		match(Tags::ANY, new SyntaxError("'*' as a constantly structure is required."));
		t.select_all_items = true;
		match(Tags::FROM, new SyntaxError("'from' expected."));
		_t_table_name = lookahead;
		match(Tags::STRING, new SyntaxError("Table name expected."), false);
		t.table = t.GetTable(((String*)_t_table_name)->lexval);
		_WHERE();
		match(Tags::__END_OF_STRING__, nullptr);
		return t.Delete();
	}
	else if (lookahead->tag == Tags::UPDATE) {
		match(Tags::UPDATE);
		_t_table_name = lookahead;
		match(Tags::STRING, new SyntaxError("Table name expected."), false);
		t.table = t.GetTable(((String*)_t_table_name)->lexval);
		match(Tags::SET, new SyntaxError("'set' expected"));
		A();
		_WHERE();
		match(Tags::__END_OF_STRING__, nullptr);
		return t.Update();
	}
	else if (lookahead->tag == Tags::CREATE) {
		match(Tags::CREATE);
		match(Tags::TABLE, new SyntaxError("This field must be 'table'."));
		_t_table_name = lookahead;
		match(Tags::STRING, new SyntaxError("Table name required"), false);
		match(Tags::LB, new SyntaxError("Column list for the table is required."));
		CO();
		match(Tags::RB, new SyntaxError("')' is required."));
		match(Tags::__END_OF_STRING__, nullptr);
		return t.CreateTable(_t_table_name);
	}
	else if (lookahead->tag == Tags::DROP) {
		match(Tags::DROP);
		match(Tags::TABLE, new SyntaxError("This field must be 'table'."));
		_t_table_name = lookahead;
		match(Tags::STRING, new SyntaxError("Table name required"), false);
		match(Tags::__END_OF_STRING__, nullptr);
		return t.DropTable(_t_table_name);
	}
	throw new SyntaxError("Only 'select', 'delete', 'update', 'insert', 'create', 'drop' can be the first token.");
}

void parser::W() {
	if (lookahead->tag == Tags::ANY) {
		match(Tags::ANY);
		t.list_all_columns = true;
	}
	else {
		t.list_all_columns = false;
		P();
	}
}

void parser::P() {
	token * _t_table_name = nullptr;
	_t_table_name = lookahead;
	match(Tags::STRING, new SyntaxError("Property name required."), false);
	t.select_colunm_list.push_back(((String *)_t_table_name)->lexval);
	//PLIST.ADD
	if (lookahead->tag == Tags::COMMA) {
		match(Tags::COMMA);
		P();
	}
}

string parser::C(int inh_rescall) {
	string val;
	token * tmp = lookahead;
	if (inh_rescall && lookahead->tag == Tags::__END_OF_STRING__)
		throw new SyntaxError("Expression expected");
	if (lookahead->tag == Tags::STRING) {
		tmp = lookahead;
		match(Tags::STRING, new SyntaxError("Property name required."), false);
		token * tmp_relop = lookahead;
		matchRelop(new SyntaxError("Relation operators required."), false);
		token * tmp_val = lookahead;
		matchObject(new SyntaxError("Value required."), false);
		val += t.Gen(tmp, tmp_relop, tmp_val);
		if (!lookahead) //如果词法分析结束就不继续分析
			return val;
		if (lookahead->tag == Tags::AND) {
			t.condition_list.push_back(lookahead);
			match(Tags::AND, false);
			val += " and " + C(true);;
		}
		else if (lookahead->tag == Tags::OR) {
			t.condition_list.push_back(lookahead);
			match(Tags::OR);
			val += " or " + C(true);;
		}
		//else val += C();
	}
	else if (lookahead->tag == Tags::NOT) {
		t.condition_list.push_back(lookahead);
		match(Tags::NOT);
		val += " not " + C(true);
	}
	else if (lookahead->tag == Tags::LB) {
		t.condition_list.push_back(lookahead);
		match(Tags::LB);
		val += "(" + C(true);
		t.condition_list.push_back(lookahead);
		match(Tags::RB, new SyntaxError("')' required"));
		val += ")";
		if (lookahead->tag == Tags::AND) {
			t.condition_list.push_back(lookahead);
			match(Tags::AND, false);
			val += " and " + C(true);
		}
		else if (lookahead->tag == Tags::OR) {
			t.condition_list.push_back(lookahead);
			match(Tags::OR);
			val += " or " + C(true);
		}
		//else val += C();
	}
	return val;
}

void parser::L() {
	token * _t = lookahead;
	matchObject(new SyntaxError("Value must be specified."), false);
	t.insertion_list.push_back(_t);
	if (lookahead->tag == Tags::COMMA) {
		match(Tags::COMMA, new SyntaxError("',' required"));
		L();
	}
}

void parser::A() {
	token * t_prop = lookahead;
	match(Tags::STRING, new SyntaxError("Property name maust be specified"), false);
	match(Tags::EQ, new SyntaxError("‘=’ required, 赋值不用等号 >_< 等号在哭好嘛，快去看SQL教程。"), false);
	token * t_val = lookahead;
	matchObject(new SyntaxError("Property value maust be specified"), false);
	t.assign_list.insert({ { ((String *) (t_prop))->lexval, t_val } });
	if (!lookahead)
		//如果词法分析已结束就不再继续分析
		return;
	if (lookahead->tag == Tags::COMMA) {
		match(Tags::COMMA);
		A();
	}
}

void parser::CO() {
	if (lookahead->tag == Tags::STRING) {
		token * name = lookahead;
		match(Tags::STRING, nullptr, false);
		match(Tags::COLON, new SyntaxError("':' required."));
		token * type_id = lookahead;
		matchType(new SyntaxError("Typename required."), false);
		t.create_table_columns.push_back({ name, type_id });
		if (lookahead->tag == Tags::COMMA) {
			match(Tags::COMMA);
			CO();
		}
	}
	else
		throw new SyntaxError("Column list for the table is required");
}

parser::parser(char * sql) {
	lex = new lexer(sql);
	if (!lex)
		throw "Failed on creating lexer";
	lookahead = lex->scan();
}

parser::~parser() {
	if (lex)
		delete lex;
}

void parser::match(token * terminal, SyntaxError * e, bool disposeLookahead) {
	if (lookahead && *lookahead == *terminal) {
		//if (disposeLookahead)
		//	delete lookahead;
		lookahead = lex->scan();
		if (e)
			delete e;
	}
	else {
		//if (lookahead)
		//	delete lookahead;
		throw e;
	}
}

void parser::match(int terminal_tag, SyntaxError * e, bool disposeLookahead) {
	if (lookahead && lookahead->tag == terminal_tag) {
		//if (disposeLookahead)
		//	delete lookahead;
		lookahead = lex->scan();
		if (e)
			delete e;
	}
	else {
		//if (lookahead)
		//	delete lookahead;
		throw e;
	}
}

void parser::matchObject(SyntaxError * e, bool disposeLookahead) {
	if (lookahead && (lookahead->tag == Tags::NUMBERS || lookahead->tag == Tags::STRING)) {
		//if (disposeLookahead)
		//	delete lookahead;
		lookahead = lex->scan();
		if (e)
			delete e;
	}
	else {
		//if (lookahead)
		//	delete lookahead;
		throw e;
	}
}

void parser::matchRelop(SyntaxError * e, bool disposeLookahead) {
	if (lookahead && (lookahead->tag > Tags::__RELOP_START__ && lookahead->tag < Tags::__RELOP_END__)) {
		//if (disposeLookahead)
		//	delete lookahead;
		lookahead = lex->scan();
		if (e)
			delete e;
	}
	else {
		//if (lookahead)
		//	delete lookahead;
		throw e;
	}
}

void parser::matchType(SyntaxError * e, bool dispose) {
	if (lookahead && (lookahead->tag == Tags::K_NUMBER || lookahead->tag == Tags::K_STRING)) {
		//if (disposeLookahead)
		//	delete lookahead;
		lookahead = lex->scan();
		if (e)
			delete e;
	}
	else {
		//if (lookahead)
		//	delete lookahead;
		throw e;
	}
}

class SQL_Parser {
public:
	string data_file_path;

	SQL_Parser(string _data_file_path):data_file_path(_data_file_path) {

	}

	string query(char * sql) {
		parser * p = nullptr;
		string str;
		try {
			p = new parser(sql);
			p->t.homePath = data_file_path;
			str = p->analyze();
		}
		catch (SyntaxError * e) {
			cout << endl << sql << endl;
			for (int i = 0; i < p->getColumn() - 1; i++)
				cout << " ";
			cout << "^\n";
			cout << "语法错误：" << (e ? e->what() : "未知错误");
			if (e)
				delete e;
		}
		catch (const char * e) {
			cout << e;
		}
		catch (...) {
			cout << "Error.";
		}
		if (p)
			delete p;
		return str;
	}
};

int main() {
	parser * p = nullptr;
	char str[256];
	SQL_Parser sp("k:/");
	cout << " ** SQL Parser **" << endl;
	while (1) {
		cout << "\n>";
		cin.getline(str, 256);
		sp.query(str);
	}
}