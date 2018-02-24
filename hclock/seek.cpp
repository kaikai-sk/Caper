#include <cassert>
#include "seek.h"

//get the classifier.a object of python
PyObject getModel(PyObject* pMod,const char* modelFileName,const char* function)
{
	PyObject* pFunc = NULL;
	PyObject* pArgs = NULL;
	PyObject* pRetVal = NULL;

	//查找函数
	if (!(pFunc = PyObject_GetAttrString(pMod, function))) 
	{
		cout<<"function can't be loaded"<<endl;
	}

	//创建参数
	pArgs = PyTuple_New(1);
	PyTuple_SetItem(pArgs, 0, Py_BuildValue("s", modelFileName));
	
	//函数调用
	pRetVal = PyEval_CallObject(pFunc, pArgs);
	PyObject ret;
	PyArg_Parse(pRetVal, "O", &ret);
	return ret;
}

char* predict(PyObject* pFunc,PyObject classifier,vector<double> params)
{
        //Py_Initialize();
        PyObject* pArgs = NULL;
        PyObject* pRetVal = NULL;

        pArgs = PyTuple_New(1+params.size());
        PyTuple_SetItem(pArgs, 0, Py_BuildValue("O", classifier));
        for (int i = 0; i < params.size(); i++)
          {
                PyTuple_SetItem(pArgs, i+1, Py_BuildValue("i", params[i]));
        }

        pRetVal = PyEval_CallObject(pFunc, pArgs);

        //int ret;
        char* ret;
        PyArg_Parse(pRetVal, "s", &ret);
        //Py_Finalize();
      //cout<<ret<<endl;
		return ret;
}

double stringToDouble(string& str)  
{  
    istringstream iss(str);  
    double num;  
    iss >> num;  
    return num;      
}  

/*
	parameter:as same as the function predict
*/
double fastHddSeek(PyObject* pFunc,PyObject classifier,vector<double> params )
{
		string res=predict(pFunc,classifier,params);
		//printf("%s\n",res);
		//cout<<res<<endl;
		return stringToDouble(res);
}
/*
	parameter:as same as the function predict
*/
double slowHddSeek(PyObject* pFunc,PyObject classifier,vector<double> params )
{
		string res=predict(pFunc,classifier,params);
		//printf("%s\n",res);
		//cout<<res<<endl;
		return stringToDouble(res);
}

double SsdSeek(PyObject* pFunc,PyObject classifier,vector<double> params )
{
		string res=predict(pFunc,classifier,params);
		//printf("%s\n",res);
		//cout<<res<<endl;
		return stringToDouble(res);
}


