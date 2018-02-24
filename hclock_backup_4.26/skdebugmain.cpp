#include "seek.h"
#include <vector>
using namespace std;

int main()
{
	double res;
	vector<double>vec;
	vec.push_back(1111);	
	vec.push_back(1111);
	vec.push_back(1111);
	vec.push_back(1111);
	res=slowHddSeek("myPredict","predict2","exectrace_ts0disk_ch9_policy3_pre_s.csv_cdei.csv20_train_model.m",
			vec);
	res=slowHddSeek("myPredict","predict2","exectrace_ts0disk_ch9_policy3_pre_s.csv_cdei.csv20_train_model.m",
                        vec);

	return 0;
}
