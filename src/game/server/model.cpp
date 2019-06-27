#include "tensorflow/cc/saved_model/loader.h"
#include "model.h"

namespace tf = tensorflow;

CModel::CModel()
{
	tf::SavedModelBundle bundle;
	tf::LoadSavedModel(tf::SessionOptions(), tf::RunOptions(), "stuff", {}, &bundle);
}
