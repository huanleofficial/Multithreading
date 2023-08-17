#include<iostream>
#include<string>
#include<vector>
#include<fstream>
#include<memory>
#include<regex>
#include<stack>
#include<bitset>
#include<iomanip>
#include<thread>
#include<sstream>
#include<queue>
#include<mutex>
#include<functional>
#include<chrono>

using namespace std;

class HashTable;

class Parser {
	vector<string> priceData;
	vector<vector<string>> cartData;
	vector<string> keyData;
public:
	Parser() {}
	vector<string>& getPriceData() { return priceData; }
	vector<vector<string>>& getCartData() { return cartData; }
	vector<string>& getKeyData() { return keyData; }
	void readPriceData(const string&);
	void readCartData(const string&);
	void readKeyData(const string&);
};

//Reads the price data into priceData
void Parser::readPriceData(const string& path) {
	ifstream file(path);
	if (!file.is_open()) {
		cout << "Price file not found. Make sure it is named " << path << "\n\n\n\n\n";
		return;
	}
	string temp;
	while (getline(file, temp)) {
		priceData.push_back(temp);
	}
	if (!priceData.size()) {
		cout << "Price file read failed.\n\n\n\n\n";
	}
}

//Reads the cart data file, first string in vector is the cart number the rest are barcodes
void Parser::readCartData(const string& path) {
	ifstream file(path);
	if (!file.is_open()) {
		cout << "Cart file not found. Make sure it is named " << path << "\n\n\n\n\n";
		return;
	}
	string temp;
	bool isCart = true;
	vector<string> tempCart;
	while (getline(file, temp)) {
		if (isCart) {
			tempCart.push_back(temp);
			isCart = false;
		}
		else {
			regex comma(",");
			sregex_token_iterator iter(temp.begin(), temp.end(), comma, -1);
			for (; iter != sregex_token_iterator(); iter++) {
				if (iter->str() != "") tempCart.push_back(iter->str());
			}
			cartData.push_back(tempCart);
			tempCart.clear();
			isCart = true;
		}
	}
	if (!cartData.size()) {
		cout << "Cart file read failed.\n\n\n\n\n";
	}
}

//Reads the char - barcode keys into keyData
void Parser::readKeyData(const string& path) {
	ifstream file(path);
	if (!file.is_open()) {
		cout << "Key file not found. Make sure it is named " << path << "\n\n\n\n\n";
		return;
	}
	string temp;
	while (getline(file, temp)) {
		keyData.push_back(temp);
	}
	if (!keyData.size()) {
		cout << "Key file read failed.\n\n\n\n\n";
	}
}

class BaseNode {
public:
	virtual ~BaseNode() {}
};

//Has a basenode class so can easily make arrays of different types of nodes and dynamically check which they are
template<typename T>
class XMLNode : public BaseNode {
	string tag;
	vector<T> data;
	shared_ptr<BaseNode> parent;
public:
	XMLNode(const string& s, shared_ptr<BaseNode> p = nullptr) : tag(s), parent(p) {}
	string& getTag() { return tag; }
	vector<T>& getData() { return data; }
	shared_ptr<BaseNode>& getParent() { return parent; }
	void add(const T& x) { data.push_back(x); };
	void setParent(shared_ptr<BaseNode> p) { parent = p; }
};


class XMLProcessor {
	vector<shared_ptr<BaseNode>> data;
public:
	void process(const vector<string>&);
	vector<shared_ptr<BaseNode>>& getData() { return data; }
	void clear() { data.clear(); }
	void print(vector<shared_ptr<BaseNode>>&, int = 0);
};

//Processes the XML into nodes, delete "prevCode" related stuff to generalize it
void XMLProcessor::process(const vector<string>& strs) {
	regex openTag("<\\w*>");
	regex closeTag("<\\/.+>");
	regex tagWData("<\\w*>\\w+<\\/.+>");

	bool bad = false;
	stack<shared_ptr<BaseNode>> prev;
	string prevCode;
	for (string str : strs) {
		string tag = str.substr(str.find('<') + 1, str.find('>') - str.find('<') - 1);
		if (regex_search(str, openTag) && regex_search(str, closeTag)) {
			if (!bad) {
				string inside = str.substr(str.find('>') + 1, str.find('<', str.find('<') + 1) - str.find('>') - 1);
				if (inside != "" && inside != prevCode) {

					shared_ptr<BaseNode> pNode = shared_ptr<BaseNode>(dynamic_cast<BaseNode*>(new XMLNode<string>(tag, prev.top())));
					dynamic_pointer_cast<XMLNode<string>>(pNode)->add(inside);
					dynamic_pointer_cast<XMLNode<shared_ptr<BaseNode>>>(prev.top())->add(pNode);
					if (tag == "Barcode") prevCode = inside;
				}
				else {
					bad = true;
					prev.pop();
				}
			}
		}
		else if (regex_search(str, openTag)) {
			shared_ptr<BaseNode> pNode = shared_ptr<BaseNode>(dynamic_cast<BaseNode*>(new XMLNode<shared_ptr<BaseNode>>(tag)));
			if (prev.size()) {
				dynamic_pointer_cast<XMLNode<shared_ptr<BaseNode>>>(prev.top())->add(pNode);
				dynamic_pointer_cast<XMLNode<shared_ptr<BaseNode>>>(pNode)->setParent(prev.top());
			}
			prev.push(pNode);
		}
		else if (regex_search(str, closeTag)) {
			if (prev.size()) {
				if (prev.size() == 1 && !bad) data.push_back(prev.top());
				prev.pop();
				bad = false;
			}
		}
	}
}

//Prints out all the tags and their data
void XMLProcessor::print(vector<shared_ptr<BaseNode>>& arr, int layer) {
	for (auto i : arr) {
		if (dynamic_pointer_cast<XMLNode<shared_ptr<BaseNode>>>(i)) {
			auto iCon = dynamic_pointer_cast<XMLNode<shared_ptr<BaseNode>>>(i);
			for (int j = 0; j < layer; j++) cout << '\t';
			cout << iCon->getTag() << '\n';
			print(iCon->getData(), layer + 1);
		}
		else if (dynamic_pointer_cast<XMLNode<string>>(i)) {
			auto iCon = dynamic_pointer_cast<XMLNode<string>>(i);
			for (int j = 0; j < layer; j++) cout << '\t';
			cout << iCon->getTag() << '\n';
			for (auto d : iCon->getData()) {
				for (int j = 0; j < layer + 1; j++) cout << '\t';
				cout << d << '\n';
			}
		}
	}
}

class HashTable {
	vector<char> chars;
	void findProd(vector<shared_ptr<BaseNode>>&, string&, shared_ptr<BaseNode>&, shared_ptr<BaseNode> = nullptr);
	shared_ptr<BaseNode> errorNode();
public:
	HashTable(vector<string>&);
	char operator[](int x) {
		if (x > 511 || x < 0) throw string("Index out of bounds");
		else return chars[x];
	}
	void addNames(vector<shared_ptr<BaseNode>>&, shared_ptr<BaseNode> = nullptr);
	shared_ptr<BaseNode> lookup(vector<shared_ptr<BaseNode>>&, const string&);
	void printAllCarts(vector<vector<string>>&, vector<shared_ptr<BaseNode>>&);
};


class Lane {
	queue<vector<string>> carts;
	mutex qMutex;
	bool filling;
public:
	Lane() : filling(true) {}
	void addCarts(vector<vector<string>>&, mutex&);
	void processCarts(stringstream&, HashTable*, mutex&, vector<shared_ptr<BaseNode>>&);
};

//Add the carts to the queue
void Lane::addCarts(vector<vector<string>>& cartArr, mutex& cartArrMutex) {
	while (cartArr.size()) {
		qMutex.lock();
		cartArrMutex.lock();
		if (cartArr.size()) {
			carts.push(cartArr.back());
			cartArr.pop_back();
		}
		cartArrMutex.unlock();
		qMutex.unlock();

		//Makes lanes pretty much equal, otherwise one lane hogs it
		this_thread::sleep_for(chrono::milliseconds(1));
	}
	filling = false;
}

//Process each cart in the queue
void Lane::processCarts(stringstream& allCarts, HashTable* h, mutex& strMutex, vector<shared_ptr<BaseNode>>& nodes) {
	while (filling || carts.size()) {
		vector<string> cart;
		qMutex.lock();
		if (carts.size()) {
			cart = carts.front();
			carts.pop();
		}
		qMutex.unlock();
		if (cart.size()) {
			stringstream strOut;

			strOut << cart[0] << '\n';
			double total = 0;
			for (int i = 1; i < cart.size(); i++) {
				auto prod = h->lookup(nodes, cart[i]);
				auto prodCon = dynamic_pointer_cast<XMLNode<shared_ptr<BaseNode>>>(prod);
				auto prodNameNode = dynamic_pointer_cast<XMLNode<string>>(prodCon->getData()[2]);
				string name = prodNameNode->getData()[0];
				auto prodPriceNode = dynamic_pointer_cast<XMLNode<string>>(prodCon->getData()[1]);
				string price = prodPriceNode->getData()[0];
				strOut << '\t' << name << "\t\t$" << price << '\n';
				total += stod(price);
			}
			strOut << "Total Price: $" << setprecision(2) << fixed << total << "\n\n";
			strMutex.lock();
			allCarts << strOut.str();
			strMutex.unlock();
		}
	}
}

//Makes the character table
HashTable::HashTable(vector<string>& data) {
	chars.resize(512);
	for (auto i : data) {
		string temp = "";
		for (int j = 2; j < 11; j++) {
			if (i[j] == 'w') temp += '1';
			else temp += '0';
		}
		chars[bitset<9>(temp).to_ulong()] = i[0];
	}
}

//Adds the names to all the product nodes
void HashTable::addNames(vector<shared_ptr<BaseNode>>& arr, shared_ptr<BaseNode> parent) {
	for (int k = 0; k < arr.size(); k++) {
		auto i = arr[k];
		if (dynamic_pointer_cast<XMLNode<shared_ptr<BaseNode>>>(i)) {
			auto iCon = dynamic_pointer_cast<XMLNode<shared_ptr<BaseNode>>>(i);
			addNames(iCon->getData(), arr[k]);
		}
		else if (dynamic_pointer_cast<XMLNode<string>>(i)->getTag() == "Barcode") {
			auto iCon = dynamic_pointer_cast<XMLNode<string>>(i);
			string temp = iCon->getData()[0];
			string name = "";
			while (temp != "000") {
				bitset<9> bs(temp.substr(0, 9));
				temp = temp.substr(9);
				name += chars[bs.to_ulong()];
			}
			shared_ptr<BaseNode> pNode = shared_ptr<BaseNode>(dynamic_cast<BaseNode*>(new XMLNode<string>("Name", parent)));
			dynamic_pointer_cast<XMLNode<string>>(pNode)->add(name);
			if (parent) dynamic_pointer_cast<XMLNode<shared_ptr<BaseNode>>>(parent)->add(pNode);
		}
	}
}

//Returns an error node so things don't break when the barcode is blank or doesn't match or something
shared_ptr<BaseNode> HashTable::errorNode() {
	shared_ptr<BaseNode> errBase = shared_ptr<BaseNode>(dynamic_cast<BaseNode*>(new XMLNode<shared_ptr<BaseNode>>("Product")));
	shared_ptr<BaseNode> errName = shared_ptr<BaseNode>(dynamic_cast<BaseNode*>(new XMLNode<string>("Name", errBase)));
	dynamic_pointer_cast<XMLNode<string>>(errName)->add("ERROR");
	shared_ptr<BaseNode> errPrice = shared_ptr<BaseNode>(dynamic_cast<BaseNode*>(new XMLNode<string>("Price", errBase)));
	dynamic_pointer_cast<XMLNode<string>>(errPrice)->add("0");
	shared_ptr<BaseNode> errBar = shared_ptr<BaseNode>(dynamic_cast<BaseNode*>(new XMLNode<string>("Barcode", errBase)));
	dynamic_pointer_cast<XMLNode<string>>(errBar)->add("000000000000000000000000000000000000000000000000");
	dynamic_pointer_cast<XMLNode<shared_ptr<BaseNode>>>(errBase)->add(errBar);
	dynamic_pointer_cast<XMLNode<shared_ptr<BaseNode>>>(errBase)->add(errPrice);
	dynamic_pointer_cast<XMLNode<shared_ptr<BaseNode>>>(errBase)->add(errName);
	return errBase;
}

//Looks up the hex barcode in the data
shared_ptr<BaseNode> HashTable::lookup(vector<shared_ptr<BaseNode>>& arr, const string& hex) {

	//There's probably a better way to do this, but I couldn't think of any
	string bin = "";
	for (char c : hex) {
		switch (c) {
		case '0': bin += "0000"; break;
		case '1': bin += "0001"; break;
		case '2': bin += "0010"; break;
		case '3': bin += "0011"; break;
		case '4': bin += "0100"; break;
		case '5': bin += "0101"; break;
		case '6': bin += "0110"; break;
		case '7': bin += "0111"; break;
		case '8': bin += "1000"; break;
		case '9': bin += "1001"; break;
		case 'A': bin += "1010"; break;
		case 'B': bin += "1011"; break;
		case 'C': bin += "1100"; break;
		case 'D': bin += "1101"; break;
		case 'E': bin += "1110"; break;
		case 'F': bin += "1111"; break;
		default: bin += "ERROR";
		}
	}
	if (bin == "") bin = "ERROR";


	if (regex_search(bin, regex("ERROR"))) {
		return errorNode();
	}

	shared_ptr<BaseNode> ret(nullptr);
	findProd(arr, bin, ret);

	if (!ret) return errorNode();
	return ret;
}

//Finds the barcode's product
void HashTable::findProd(vector<shared_ptr<BaseNode>>& arr, string& bar, shared_ptr<BaseNode>& result, shared_ptr<BaseNode> parent) {
	if (result) return;
	for (int k = 0; k < arr.size(); k++) {
		auto i = arr[k];
		if (dynamic_pointer_cast<XMLNode<shared_ptr<BaseNode>>>(i)) {
			auto iCon = dynamic_pointer_cast<XMLNode<shared_ptr<BaseNode>>>(i);
			findProd(iCon->getData(), bar, result, arr[k]);
		}
		else if (dynamic_pointer_cast<XMLNode<string>>(i)->getTag() == "Barcode") {
			auto iCon = dynamic_pointer_cast<XMLNode<string>>(i);
			if (iCon->getData()[0] == bar) result = parent;
		}
	}
}

//Prints all the carts
void HashTable::printAllCarts(vector<vector<string>>& arrOfCarts, vector<shared_ptr<BaseNode>>& nodes) {
	stringstream strOut;
	vector<shared_ptr<Lane>> lanes;
	vector<shared_ptr<thread>> tVec;

	mutex cartArrMutex;
	mutex strMutex;

	for (int i = 0; i < 16; i++) {
		lanes.push_back(make_shared<Lane>());
		tVec.push_back(make_shared<thread>(&Lane::addCarts, lanes[i], ref(arrOfCarts), ref(cartArrMutex)));
		tVec.push_back(make_shared<thread>(&Lane::processCarts, lanes[i], ref(strOut), this, ref(strMutex), ref(nodes)));
	}

	for (auto t : tVec) t->join();

	ofstream file("Carts.txt");
	if (file.is_open()) {
		file << strOut.str();
		file.close();
	}
	cout << strOut.str();
}

int main() {
	cout << "Reading product data...\n";
	Parser f;
	f.readPriceData("ProductPrice.xml");
	f.readKeyData("BarKeys.csv");

	cout << "Processing product data...\n";
	HashTable h(f.getKeyData());
	XMLProcessor x;
	x.process(f.getPriceData());
	h.addNames(x.getData());

	cout << "Reading cart data...\n";
	f.readCartData("Carts.csv");

	cout << "Looking up products in each cart...\n";
	h.printAllCarts(f.getCartData(), x.getData());
	cin.get();
	return 0;
}