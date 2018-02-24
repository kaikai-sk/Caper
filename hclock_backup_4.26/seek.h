#include <Python.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
using namespace std;

/*
	module:    the name of python file
	function:  the name of predicting funtion,decisioned by the true python function
	modelFileName: the name of nodel file (.m file)
	params: vector (for example [eid,count])
	return value:the char* of predicted delay
*/

PyObject getModel(PyObject* pMod,const char* modelFileName,const char* function);

char* predict(PyObject* pFunc,PyObject classifier,vector<double> params);

/*
	convert string value to double
*/
double stringToDouble(string& str);

/*
as same as the function predict
*/
double fastHddSeek(PyObject* pFunc, PyObject classifier,vector<double> params );

double slowHddSeek(PyObject* pFunc, PyObject classifier,vector<double> params );

double SsdSeek(PyObject* pFunc,PyObject classifier,vector<double> params );
