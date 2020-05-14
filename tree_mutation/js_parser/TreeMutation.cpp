#include <Python.h>
#include <iostream>
#include <cstring>
#include "antlr4-runtime.h"
#include "ECMAScriptLexer.h"
#include "ECMAScriptParser.h"
#include "ECMAScriptBaseVisitor.h"
#include "ECMAScriptSecondVisitor.h"

using namespace antlr4;
using namespace std;


extern "C" int parse(char* target,size_t len,char* second,size_t lenS);
extern "C" void fuzz(int index, char** ret, size_t* retlen);
extern "C" void get_reward(int r_from_afl);

#define MAXSAMPLES 10000
#define MAXTEXT 200

string ret[MAXSAMPLES+2]; // 存储语法树中的节点
string ifStatement[MAXSAMPLES+2];
string funcDeclaration[MAXSAMPLES+2];

int arr_s0[12] = {0};  // 状态s0
int arr_s1[12] = {0};  // 状态s1
int action = 0;
int reward = 0;

int errors = 0;


class DRL_Agent{
    public:
        PyObject *pModule; // python module
        PyObject* pDict;  // python method dictionary in pModule
        PyObject* pAgent; // python class"Agent" declared in pModule
        PyObject* pIns;   // the instance of class "Agent"
        DRL_Agent(){
            Py_Initialize();
            PyRun_SimpleString("import sys");
            PyRun_SimpleString("sys.path.append('/home/tangsong/Superion-modified/tree_mutation/js_parser')");
            this->pModule = PyImport_ImportModule("dqn"); //导入.py模块
            this->pDict= PyModule_GetDict(pModule); // 导入dqn.py中的方法字典
            this->pAgent = PyDict_GetItemString(pDict,"Agent"); // 导入Agent类
            if(!pAgent)
            {
                printf("Can't find Agent class.\n");
            }
            // 构造Agent对象
            PyObject* pConstruct = PyInstanceMethod_New(pAgent);
            this->pIns= PyObject_CallObject(pConstruct,NULL);
        }

        int act(){
            int action;
            PyObject* pA0 = PyObject_CallMethod(pIns,"act","[i,i,i,i,i,i,i,i,i,i,i,i]",Py_BuildValue("i",arr_s0[0]),
                                                                Py_BuildValue("i",arr_s0[1]),
                                                                Py_BuildValue("i",arr_s0[2]),
                                                                Py_BuildValue("i",arr_s0[3]),
                                                                Py_BuildValue("i",arr_s0[4]),
                                                                Py_BuildValue("i",arr_s0[5]),
                                                                Py_BuildValue("i",arr_s0[6]),
                                                                Py_BuildValue("i",arr_s0[7]),
                                                                Py_BuildValue("i",arr_s0[8]),
                                                                Py_BuildValue("i",arr_s0[9]),
                                                                Py_BuildValue("i",arr_s0[10]),
                                                                Py_BuildValue("i",arr_s0[11]));

            PyArg_Parse(pA0, "i", &action);
            return action;
        }

        void put(){
            PyObject* r = Py_BuildValue("i",&reward);
            PyObject* pA0 = Py_BuildValue("i",&action);
            PyObject_CallMethod(pIns,"put","[i,i,i,i,i,i,i,i,i,i,i,i],i,[i,i,i,i,i,i,i,i,i,i,i,i],i",Py_BuildValue("i",arr_s0[0]),
                                                                Py_BuildValue("i",arr_s0[1]),
                                                                Py_BuildValue("i",arr_s0[2]),
                                                                Py_BuildValue("i",arr_s0[3]),
                                                                Py_BuildValue("i",arr_s0[4]),
                                                                Py_BuildValue("i",arr_s0[5]),
                                                                Py_BuildValue("i",arr_s0[6]),
                                                                Py_BuildValue("i",arr_s0[7]),
                                                                Py_BuildValue("i",arr_s0[8]),
                                                                Py_BuildValue("i",arr_s0[9]),
                                                                Py_BuildValue("i",arr_s0[10]),
                                                                Py_BuildValue("i",arr_s0[11]),
                                                                pA0,
                                                                Py_BuildValue("i",arr_s1[0]),
                                                                Py_BuildValue("i",arr_s1[1]),
                                                                Py_BuildValue("i",arr_s1[2]),
                                                                Py_BuildValue("i",arr_s1[3]),
                                                                Py_BuildValue("i",arr_s1[4]),
                                                                Py_BuildValue("i",arr_s1[5]),
                                                                Py_BuildValue("i",arr_s1[6]),
                                                                Py_BuildValue("i",arr_s1[7]),
                                                                Py_BuildValue("i",arr_s1[8]),
                                                                Py_BuildValue("i",arr_s1[9]),
                                                                Py_BuildValue("i",arr_s1[10]),
                                                                Py_BuildValue("i",arr_s1[11]),
                                                                r);
        }

        void learn(){
            PyObject_CallMethod(pIns,"learn",NULL);
        }
};

class Test{
public:
	int a;
	Test(){
		this->a = 1;
	}
};

DRL_Agent agent = DRL_Agent();
Test t = Test();
bool cmp(const string &x, const string &y){return x<y;}


int parse(char* target,size_t len,char* second,size_t lenS) {
	vector<misc::Interval> intervals;
    intervals.clear();
	vector<string> texts;
    texts.clear();
	int num_of_smaples=0;
	//parse the target
	string targetString;
	try{
		targetString=string(target,len);
		ANTLRInputStream input(targetString);
		//ANTLRInputStream input(target);
		ECMAScriptLexer lexer(&input);
		CommonTokenStream tokens(&lexer);
		ECMAScriptParser parser(&tokens);
		TokenStreamRewriter rewriter(&tokens);
		tree::ParseTree* tree = parser.program();
		if(parser.getNumberOfSyntaxErrors()>0){
			return 0;
		}else{
 			ECMAScriptBaseVisitor *visitor=new ECMAScriptBaseVisitor();
			visitor->visit(tree);
			int interval_size = visitor->intervals.size();
			for(int i=0;i<interval_size;i++){  // 采集intervals信息
				if(find(intervals.begin(),intervals.end(),visitor->intervals[i])!=intervals.end()){
				}else if(visitor->intervals[i].a<=visitor->intervals[i].b){
					intervals.push_back(visitor->intervals[i]);	
				}
			}
			// get current state s0
			for(int i=0;i<12;i++){
			    arr_s0[i] = visitor->state[i];
			}
			int texts_size = visitor->texts.size();
			for(int i=0;i<texts_size;i++){
				if(find(texts.begin(),texts.end(),visitor->texts[i])!=texts.end()){
				}else if(visitor->texts[i].length()>MAXTEXT){
				}else{
					texts.push_back(visitor->texts[i]);
            	}
			}
            int action = agent.act();
            delete visitor;
			//parse sencond
			string secondString;
			try{
				secondString=string(second,lenS);
				//cout<<targetString<<endl;
				//cout<<secondString<<endl;

				ANTLRInputStream inputS(secondString);
				ECMAScriptLexer lexerS(&inputS);
				CommonTokenStream tokensS(&lexerS);
				ECMAScriptParser parserS(&tokensS);
				tree::ParseTree* treeS = parserS.program();

				if(parserS.getNumberOfSyntaxErrors()>0){
		 		
				}else{
					ECMAScriptSecondVisitor *visitorS=new ECMAScriptSecondVisitor();
					visitorS->visit(treeS);
					if(action ==0){
					    texts_size = visitorS->texts.size();
					    for(int i=0;i<texts_size;i++){
						    if(find(texts.begin(),texts.end(),visitorS->texts[i])!=texts.end()){
                        	}else if(visitorS->texts[i].length()>MAXTEXT){
						    }else{
							    texts.push_back(visitorS->texts[i]);
						    }
					    }
					}else if(action ==1){
					texts_size = visitorS->texts.size();
					    for(int i=0;i<texts_size;i++){
						    if(find(texts.begin(),texts.end(),visitorS->texts[i])!=texts.end()){
                        	}else if(visitorS->texts[i].length()>MAXTEXT){
						    }else{
							texts.push_back(visitorS->texts[i]);
						    }
					    }
					}
                    		delete visitorS;
				}

				interval_size = intervals.size();
				sort(texts.begin(),texts.end());
				texts_size = texts.size();

				for(int i=0;i<interval_size;i++){
					for(int j=0;j<texts_size;j++){
						rewriter.replace(intervals[i].a,intervals[i].b,texts[j]);
						ret[num_of_smaples++]=rewriter.getText();
						if(num_of_smaples>=MAXSAMPLES)break;
					}
					if(num_of_smaples>=MAXSAMPLES)break;
				}
			}catch(range_error e){
				//std::cerr<<"range_error"<<second<<endl;
			}
		}
	}catch(range_error e){
		//std::cerr<<"range_error:"<<target<<endl;
	}
	return num_of_smaples;
}

void fuzz(int index, char** result, size_t* retlen){
	*retlen=ret[index].length();
	*result=strdup(ret[index].c_str());

	// get s1
    string str = *result;
    ANTLRInputStream input(str);
	ECMAScriptLexer lexer(&input);
	CommonTokenStream tokens(&lexer);
	ECMAScriptParser parser(&tokens);
	TokenStreamRewriter rewriter(&tokens);
	tree::ParseTree* tree = parser.program();
	if(parser.getNumberOfSyntaxErrors()>0){
        errors ++;
	}else{
	    ECMAScriptBaseVisitor *visitor=new ECMAScriptBaseVisitor();
	    visitor->visit(tree);
	    for(int i=0;i<12;i++){
	        arr_s1[i] = visitor->state[i];

	    }
	}
//	cout << "action:" << action << "   texts: "<< str << endl;
}

void get_reward(int r_from_afl){
    reward = r_from_afl;
   agent.put();
   agent.learn();
}


int main(){
  	ifstream in;
	char target[100*1024];
	int len=0;
  	in.open("/Users/tangsong/Superion-master/tree_mutation/js_parser/test.js");
	while(!in.eof()){
		in.read(target,102400);
	}
	len=in.gcount();
	//cout<<target<<endl;
	//cout<<len<<endl;
	in.close();

	char second[100*1024];
	int lenS=0;
  	in.open("/Users/tangsong/Superion-master/tree_mutation/js_parser/test2.js");
	while(!in.eof()){
		in.read(second,102400);
	}
	lenS=in.gcount();

  	int num_of_smaples=parse(target,len,second,lenS);
  	for(int i=0;i<num_of_smaples;i++){
     	char* retbuf=nullptr;
     	size_t retlen=0;
     	fuzz(i,&retbuf,&retlen);
     	get_reward(1);
  	}
  	cout << "num_of_smaples:" << num_of_smaples << endl;
  	cout << "errors:" << errors << endl;
    return 0;
}

