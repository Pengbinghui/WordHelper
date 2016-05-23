// 作者：王润基

#include "kernel.h"
#include "../word/json/json.h"
#include <algorithm>	// min
#include <fstream>

Kernel::Kernel ()
{
	dataBase = new DataBaseImpl;
	loadConfig();
	loadDictionary();
	wordSelectStrategyList.push_back(new WordSelectStrategy_Random(dataBase));
	/*
	auto wordList = dataBase->getWordList([](const WordInfo&){return true;});
	for(auto word: wordList)
		std::cerr << (Json::Value)*word;
	*/
}
Kernel::~Kernel ()
{
	//saveDictionary();
	writeConfig();
	for(auto strategy: wordSelectStrategyList)
		delete strategy;
	delete dataBase;
}

void Kernel::printLog (string const& str) const
{
	std::clog << "Kernel: " << str << endl;
}
void Kernel::setConfigDefault ()
{
	defaultTestSize = 50;
	dataFileName = "dictionary.txt";
	wordSelectStrategyId = 0;
	defaultTestModeName = "recall";
}
void Kernel::loadConfig ()
{
	std::ifstream configFileIn(configFileName);
	if(!configFileIn.is_open())
	{
		printLog("Failed to open config file. Use default settings.");
		setConfigDefault();
		return;
	}
	Json::Reader reader;
	Json::Value json;
	reader.parse(configFileIn, json);

	defaultTestSize = json["defaultTestSize"].asInt();
	dataFileName = json["dataFileName"].asString();
	wordSelectStrategyId = json["wordSelectStrategyId"].asInt();
	defaultTestModeName = json["defaultTestModeName"].asString();

	printLog("Success to load config.");
}
void Kernel::writeConfig()
{
	std::ofstream configFileOut(configFileName);
	if(!configFileOut.is_open())
	{
		printLog("Failed to open config file. Saving config failed.");
		throw std::runtime_error("Failed to open config file. Saving config failed.");
	}

	Json::Value json;
	json["defaultTestSize"] = defaultTestSize;
	json["dataFileName"] = dataFileName;
	json["wordSelectStrategyId"] = wordSelectStrategyId;
	json["defaultTestModeName"] = defaultTestModeName;
	configFileOut << json;

	printLog("Success to save config.");
}
void Kernel::loadDictionary ()
{
	std::ifstream wordFileIn(dataFileName);
	if(!wordFileIn.is_open())
	{
		string errorString = "Failed to open dictionary file: " + dataFileName;
		printLog(errorString);
		throw std::runtime_error(errorString);
	}
	dataBase->loadDictInfo(wordFileIn);
	printLog("Success to load dictionary.");
}
void Kernel::saveDictionary ()
{
	std::ofstream wordFileOut(dataFileName);
	if(!wordFileOut.is_open())
	{
		string errorString = "Failed to open dictionary file: " + dataFileName;
		printLog(errorString);
		throw std::runtime_error(errorString);
	}
	dataBase->saveDictInfo(wordFileOut);
	printLog("Success to save dictionary.");
}

string Kernel::getVersion () const
{
	return version;
}
int Kernel::getDefaultTestSize () const
{
	return defaultTestSize;
}
void Kernel::setDefaultTestSize (int v)
{
	if(v <= 0)
	{
		printLog("Failed to set default test size. It must be >0.");
		return;
	}
	defaultTestSize = v;
}
string Kernel::getDataFileName () const
{
	return dataFileName;
}
void Kernel::setDataFileName (string const& fileName)
{
	dataFileName = fileName;
	printLog("Success to set data file name: " + fileName);
}
string Kernel::getConfigFileName () const
{
	return configFileName;
}
void Kernel::setConfigFileName (string const& fileName)
{
	configFileName = fileName;
	printLog("Success to set config file name: " + fileName);
}

vector<string> Kernel::getTestModeList () const
{
	return TEST_MODE_NAME;
}
string Kernel::getDefaultTestMode () const
{
	return defaultTestModeName;
}
void Kernel::setDefaultTestMode (string const& testModeName)
{
	if(std::find(TEST_MODE_NAME.begin(), TEST_MODE_NAME.end(), testModeName) == TEST_MODE_NAME.end())
		printLog("Failed to set default test mode.  No such name: " + testModeName);
	else
		defaultTestModeName = testModeName;
}
WordSelectStrategy* Kernel::getWordSelectStrategy() const
{
	return wordSelectStrategyList.at(wordSelectStrategyId);
}
WordKernel* Kernel::getNewWordKernel (const WordInfo* word) const
{
	return new WordKernel(const_cast<WordInfo*>(word));
}

#define TEST_KERNEL_PARAMETER (dataBase, getWordSelectStrategy(), size == 0? defaultTestSize: size)
TestKernel_SpellMode* Kernel::getNewSpellTestKernel (int size) const
{
	return new TestKernel_SpellMode TEST_KERNEL_PARAMETER;
}
TestKernel_RecallMode* Kernel::getNewRecallTestKernel (int size) const
{
	return new TestKernel_RecallMode TEST_KERNEL_PARAMETER;
}
TestKernel_ChoiceMode* Kernel::getNewChoiceTestKernel (int size, bool choiceEnglish) const
{
	if(choiceEnglish)
		return new TestKernel_ChoiceEnglish TEST_KERNEL_PARAMETER;
	else
		return new TestKernel_ChoiceChinese TEST_KERNEL_PARAMETER;
}
TextAnalyzeKernel* Kernel::getNewTextAnalyzeKernel () const
{
	return new TextAnalyzeKernelImpl(dataBase);
}

vector<string> Kernel::getSearchHistory (int size) const
{
	vector<string> list(searchHistoryList.end() - std::min((int)searchHistoryList.size(), size), searchHistoryList.end());
	std::reverse(list.begin(), list.end());
	return list;
}

vector<const WordInfo*> Kernel::fuzzySearchByEnglish (string const& str) 
{
	searchHistoryList.push_back(str);
	auto filter = [&](WordInfo const& word) -> bool
	{
		return word.word.substr(0, str.length()) == str;
	};
	return dataBase->getWordListConst(filter);
}
vector<const WordInfo*> Kernel::fuzzySearchByChinese (string const& str) 
{
	searchHistoryList.push_back(str);
	auto filter = [&](WordInfo const& word) -> bool
	{
		for(auto const& meaning: word.meaningList)
			if(meaning.explain.find(str) != string::npos)
				return true;
		return false;
	};
	//printLog((string)"Not finished function:  " + "Kernel::fuzzySearchByChinese  " + str);
	return dataBase->getWordListConst(filter);
}
const WordInfo* Kernel::strictSearchByEnglish (string const& word) 
{
	searchHistoryList.push_back(word);
	return dataBase->getWord(word);
}