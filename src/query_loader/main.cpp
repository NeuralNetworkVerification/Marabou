#include <cstdio>

#include "Engine.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "Preprocessor.h"
#include "ReluplexError.h"

#include "QueryLoader.h"

int main(){

	InputQuery inputQuery = load_query("test_TF");
	inputQuery.saveQuery("test_TF_AFTER");
}