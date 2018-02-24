#-*-coding:utf-8-*-
from sklearn.externals import joblib
from numpy import *

def getClf(modelFileName):
    clf = joblib.load(modelFileName)
    return clf

def predict1(modelFileName,eid,count):
    print 'debug information：',modelFileName,eid,count
    test_X=[]
    test_X.append(int(eid))
    test_X.append(int(count))
    X=[]
    X.append(test_X)
    clf = joblib.load(modelFileName)
    test_Y=clf.predict(X)
    print test_Y[0]
    tempStr=str(test_Y[0])
    return tempStr

def predict2(clf,eid,count,distance,iotype):
    test_X=[]
    test_X.append(int(eid))
    test_X.append(int(count))
    test_X.append(int(distance))
    test_X.append(int(iotype))
    X=[]
    X.append(test_X)
    test_Y=clf.predict(X)
    #print test_Y[0]
    tempStr=str(test_Y[0])
    return tempStr

def predict4(modelFileName,count):
    test_X=[]
    test_X.append(int(count))
    X=[]
    X.append(test_X)
    clf = joblib.load(modelFileName)
    test_Y=clf.predict(X)
    print test_Y[0] 
    tempStr=str(test_Y[0])
    return tempStr

def predict5(modelFileName,count,distance):
    test_X=[]
    test_X.append(int(count))
    test_X.append(int(distance))
    X=[]
    X.append(test_X)
    clf = joblib.load(modelFileName)
    test_Y=clf.predict(X)
    print test_Y[0]  
    tempStr=str(test_Y[0])
    return tempStr

def predict6(modelFileName,count,distance,eid,iotype):
    test_X=[]
    test_X.append(int(count))
    test_X.append(int(distance))
    test_X.append(int(eid))
    test_X.append(int(iotype))
    X=[]
    X.append(test_X)
    clf = joblib.load(modelFileName)
    test_Y=clf.predict(X)
    print test_Y[0] 
    tempStr=str(test_Y[0])
    return tempStr

def predict3(modelFileName,count,distance,eid,iotype,lastdelay):
    test_X=[]
    test_X.append(int(count))
    test_X.append(int(distance))
    test_X.append(int(eid))
    test_X.append(int(iotype))
    test_X.append(float(lastdelay))
    X=[]
    X.append(test_X)
    clf = joblib.load(modelFileName)
    test_Y=clf.predict(X)
    print test_Y[0]
    tempStr=str(test_Y[0])
    return tempStr
        
def predict7(modelFileName,lastdelay):
    test_X=[]
    test_X.append(float(lastdelay))
    X=[]
    X.append(test_X)
    clf = joblib.load(modelFileName)
    test_Y=clf.predict(X)
    print test_Y[0]
    tempStr=str(test_Y[0])
    return tempStr

def predict8(modelFileName,count,distance,reuse_dis,lastdelay):
    test_X=[]
    test_X.append(int(count))
    test_X.append(int(distance))
    test_X.append(int(reuse_dis))
    test_X.append(float(lastdelay))
    X=[]
    X.append(test_X)
    clf = joblib.load(modelFileName)
    test_Y=clf.predict(X)
    print test_Y[0]
    tempStr=str(test_Y[0])
    return tempStr

def predict9(modelFileName,count,distance,reuse_dis):
    test_X=[]
    test_X.append(int(count))
    test_X.append(int(distance))
    test_X.append(int(reuse_dis))
    X=[]
    X.append(test_X)
    clf = joblib.load(modelFileName)
    test_Y=clf.predict(X)
    print test_Y[0]
    tempStr=str(test_Y[0])
    return tempStr  

def predict(modelFileName,count,iotype):
    test_X=[]
    test_X.append(int(count))
    test_X.append(int(iotype))
    X=[]
    X.append(test_X)
    clf = joblib.load(modelFileName)
    test_Y=clf.predict(X)
    print test_Y[0]
    tempStr=str(test_Y[0])
    return tempStr

def add(a,b):
    return a+b

if __name__=="__main__":
    print predict2('exectrace_ts0disk_ch9_policy3_pre_s.csv_cdei.csv20_train_model.m',
                   7621600,8,1,1)
    #exectrace_ts0disk_ch9_policy3_pre_s.csv_cdei.csv20_train_model.m
