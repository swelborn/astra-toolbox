/*
-----------------------------------------------------------------------
Copyright: 2010-2015, iMinds-Vision Lab, University of Antwerp
           2014-2015, CWI, Amsterdam

Contact: astra@uantwerpen.be
Website: http://sf.net/projects/astra-toolbox

This file is part of the ASTRA Toolbox.


The ASTRA Toolbox is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

The ASTRA Toolbox is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with the ASTRA Toolbox. If not, see <http://www.gnu.org/licenses/>.

-----------------------------------------------------------------------
$Id$
*/

/** \file mexHelpFunctions.cpp
 *
 *  \brief Contains some functions for interfacing matlab with c data structures
 */
#include "mexHelpFunctions.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;
using namespace astra;


//-----------------------------------------------------------------------------------------
// get string from matlab 
string mexToString(const mxArray* pInput)
{
	// is string?
	if (mxIsChar(pInput)) {
		mwSize iLength = mxGetNumberOfElements(pInput) + 1;
		char* buf = new char[iLength]; 
		mxGetString(pInput, buf, iLength);
		std::string res = std::string(buf);
		delete[] buf;
		return res;
	}

	// is scalar?
	if (mxIsNumeric(pInput) && mxGetM(pInput)*mxGetN(pInput) == 1) {
		return boost::lexical_cast<string>(mxGetScalar(pInput));
	}

	return "";
}

//-----------------------------------------------------------------------------------------
// return true if the argument is a scalar
bool mexIsScalar(const mxArray* pInput)
{
	return (mxIsNumeric(pInput) && mxGetM(pInput)*mxGetN(pInput) == 1);
}

//-----------------------------------------------------------------------------------------
void get3DMatrixDims(const mxArray* x, mwSize *dims)
{
	const mwSize* mdims = mxGetDimensions(x);
	mwSize dimCount = mxGetNumberOfDimensions(x);
	if (dimCount == 1) {
		dims[0] = mdims[0];
		dims[1] = 1;
		dims[2] = 1;
	} else if (dimCount == 2) {
		dims[0] = mdims[0];
		dims[1] = mdims[1];
		dims[2] = 1;
	} else if (dimCount == 3) {
		dims[0] = mdims[0];
		dims[1] = mdims[1];
		dims[2] = mdims[2];
	} else {
		dims[0] = 0;
		dims[1] = 0;
		dims[2] = 0;
	}
} 






//-----------------------------------------------------------------------------------------
// turn an std vector<float32> object to an mxArray
mxArray* vectorToMxArray(std::vector<astra::float32> mInput)
{
	mxArray* res = mxCreateDoubleMatrix(1, mInput.size(), mxREAL);
	double* pdData = mxGetPr(res);
	for (unsigned int i = 0; i < mInput.size(); i++) {
		pdData[i] = mInput[i];
	}
	return res;
}

//-----------------------------------------------------------------------------------------
// turn a vector<vector<float32>> object to an mxArray
mxArray* vector2DToMxArray(std::vector<std::vector<astra::float32> > mInput)
{
	unsigned int sizex = mInput.size();
	if (sizex == 0) return mxCreateString("empty");
	unsigned int sizey = mInput[0].size();

	mxArray* res = mxCreateDoubleMatrix(sizex, sizey, mxREAL);
	double* pdData = mxGetPr(res);
	for (unsigned int i = 0; i < sizex; i++) {
		for (unsigned int j = 0; j < sizey && j < mInput[i].size(); j++) {
			pdData[j*sizex+i] = mInput[i][j];
		}
	}
	return res;
}

//-----------------------------------------------------------------------------------------
// turn a boost::any object to an mxArray
mxArray* anyToMxArray(boost::any _any) 
{
	if (_any.type() == typeid(std::string)) {
		std::string str = boost::any_cast<std::string>(_any);
		return mxCreateString(str.c_str());     
	}
	if (_any.type() == typeid(int)) {
		return mxCreateDoubleScalar(boost::any_cast<int>(_any));
	}
	if (_any.type() == typeid(float32)) {
		return mxCreateDoubleScalar(boost::any_cast<float32>(_any));
	}
	if (_any.type() == typeid(std::vector<astra::float32>)) {
		return vectorToMxArray(boost::any_cast<std::vector<float32> >(_any));
	}
	if (_any.type() == typeid(std::vector<std::vector<astra::float32> >)) {
		return vector2DToMxArray(boost::any_cast<std::vector<std::vector<float32> > >(_any));
	}
	return NULL;
}









//-----------------------------------------------------------------------------------------
// turn a MATLAB struct into a Config object
Config* structToConfig(string rootname, const mxArray* pStruct)
{
	if (!mxIsStruct(pStruct)) {
		mexErrMsgTxt("Input must be a struct.");
		return NULL;
	}

	// create the document
	Config* cfg = new Config();
	cfg->initialize(rootname);

	// read the struct
	bool ret = structToXMLNode(cfg->self, pStruct);
	if (!ret) {
		delete cfg;
		mexErrMsgTxt("Error parsing struct.");
		return NULL;		
	}
	return cfg;
}

//-----------------------------------------------------------------------------------------
bool structToXMLNode(XMLNode* node, const mxArray* pStruct) 
{
	// loop all fields
	int nfields = mxGetNumberOfFields(pStruct);
	for (int i = 0; i < nfields; i++) {

		// field and fieldname
		std::string sFieldName = std::string(mxGetFieldNameByNumber(pStruct, i));
		const mxArray* pField = mxGetFieldByNumber(pStruct, 0, i);

		// string
		if (mxIsChar(pField)) {
			string sValue = mexToString(pField);
			if (sFieldName == "type") {
				node->addAttribute("type", sValue);
			} else {
				delete node->addChildNode(sFieldName, sValue);
			}
		}

		// scalar
		else if (mxIsNumeric(pField) && mxGetM(pField)*mxGetN(pField) == 1) {
			string sValue = mexToString(pField);
			delete node->addChildNode(sFieldName, sValue);
		}

		// numerical array
		else if (mxIsNumeric(pField) && mxGetM(pField)*mxGetN(pField) > 1) {
			if (!mxIsDouble(pField)) {
				mexErrMsgTxt("Numeric input must be double.");
				return false;
			}
			XMLNode* listbase = node->addChildNode(sFieldName);
			listbase->addAttribute("listsize", mxGetM(pField)*mxGetN(pField));
			double* pdValues = mxGetPr(pField);
			int index = 0;
			for (unsigned int row = 0; row < mxGetM(pField); row++) {
				for (unsigned int col = 0; col < mxGetN(pField); col++) {
					XMLNode* item = listbase->addChildNode("ListItem");
					item->addAttribute("index", index);
					item->addAttribute("value", pdValues[col*mxGetM(pField)+row]);
					index++;
					delete item;
				}
			}
			delete listbase;
		}

		// not castable to a single string
		else if (mxIsStruct(pField)) {
			if (sFieldName == "options" || sFieldName == "option" || sFieldName == "Options" || sFieldName == "Option") {
				bool ret = optionsToXMLNode(node, pField);
				if (!ret)
					return false;
			} else {
				XMLNode* newNode = node->addChildNode(sFieldName);
				bool ret = structToXMLNode(newNode, pField);
				delete newNode;
				if (!ret)
					return false;
			}
		}

	}

	return true;
}
//-----------------------------------------------------------------------------------------
// Options struct to xml node
bool optionsToXMLNode(XMLNode* node, const mxArray* pOptionStruct)
{
	// loop all fields
	int nfields = mxGetNumberOfFields(pOptionStruct);
	for (int i = 0; i < nfields; i++) {
		std::string sFieldName = std::string(mxGetFieldNameByNumber(pOptionStruct, i));
		const mxArray* pField = mxGetFieldByNumber(pOptionStruct, 0, i);

		if (node->hasOption(sFieldName)) {
			mexErrMsgTxt("Duplicate option");
			return false;
		}
	
		// string or scalar
		if (mxIsChar(pField) || mexIsScalar(pField)) {
			string sValue = mexToString(pField);
			node->addOption(sFieldName, sValue);
		}
		// numerical array
		else if (mxIsNumeric(pField) && mxGetM(pField)*mxGetN(pField) > 1) {
			if (!mxIsDouble(pField)) {
				mexErrMsgTxt("Numeric input must be double.");
				return false;
			}

			XMLNode* listbase = node->addChildNode("Option");
			listbase->addAttribute("key", sFieldName);
			listbase->addAttribute("listsize", mxGetM(pField)*mxGetN(pField));
			double* pdValues = mxGetPr(pField);
			int index = 0;
			for (unsigned int row = 0; row < mxGetM(pField); row++) {
				for (unsigned int col = 0; col < mxGetN(pField); col++) {
					XMLNode* item = listbase->addChildNode("ListItem");
					item->addAttribute("index", index);
					item->addAttribute("value", pdValues[col*mxGetM(pField)+row]);
					index++;
					delete item;
				}
			}
			delete listbase;
		} else {
			mexErrMsgTxt("Unsupported option type");
			return false;
		}
	}
	return true;
}
//-----------------------------------------------------------------------------------------
// turn a matlab struct into a c++ map
std::map<std::string, mxArray*> parseStruct(const mxArray* pInput) 
{
	std::map<std::string, mxArray*> res;

	// check type
	if (!mxIsStruct(pInput)) {
      mexErrMsgTxt("Input must be a struct.");
	  return res;
	}

	// get field names
	int nfields = mxGetNumberOfFields(pInput);
	for (int i = 0; i < nfields; i++) {
		std::string sFieldName = std::string(mxGetFieldNameByNumber(pInput, i));
		res[sFieldName] = mxGetFieldByNumber(pInput,0,i);
	}
	return res;
}














//-----------------------------------------------------------------------------------------
// turn a Config object into a MATLAB struct
mxArray* configToStruct(astra::Config* cfg)
{
	return XMLNodeToStruct(cfg->self);
}

//-----------------------------------------------------------------------------------------
mxArray* XMLNodeToStruct(astra::XMLNode* node)
{
	std::map<std::string, mxArray*> mList;
	std::map<std::string, mxArray*> mOptions;

	// type_attribute
	if (node->hasAttribute("type")) {
		mList["type"] = mxCreateString(node->getAttribute("type").c_str());
	}

	list<XMLNode*> nodes = node->getNodes();
	for (list<XMLNode*>::iterator it = nodes.begin(); it != nodes.end(); it++) {
		XMLNode* subnode = (*it);

		// option
		if (subnode->getName() == "Option") {
			mOptions[subnode->getAttribute("key")] = stringToMxArray(subnode->getAttribute("value"));
		}

		// regular content
		else {
			mList[subnode->getName()] = stringToMxArray(subnode->getContent());
		}
		delete subnode;
	}

	if (mOptions.size() > 0) mList["options"] = buildStruct(mOptions);
	return buildStruct(mList);
}

//-----------------------------------------------------------------------------------------
mxArray* stringToMxArray(std::string input) 
{
	// matrix
	if (input.find(';') != std::string::npos) {

		// split rows
		std::vector<std::string> row_strings;
		std::vector<std::string> col_strings;
		boost::split(row_strings, input, boost::is_any_of(";"));
		boost::split(col_strings, row_strings[0], boost::is_any_of(","));

		// get dimensions
		int rows = row_strings.size();
		int cols = col_strings.size();

		// init matrix
		mxArray* pMatrix = mxCreateDoubleMatrix(rows, cols, mxREAL);
		double* out = mxGetPr(pMatrix);

		// loop elements
		for (unsigned int row = 0; row < rows; row++) {
			boost::split(col_strings, row_strings[row], boost::is_any_of(","));
			// check size
			for (unsigned int col = 0; col < col_strings.size(); col++) {
				out[col*rows + row] = boost::lexical_cast<float32>(col_strings[col]);
			}
		}
		return pMatrix;
	}
	
	// vector
	if (input.find(',') != std::string::npos) {

		// split
		std::vector<std::string> items;
		boost::split(items, input, boost::is_any_of(","));

		// init matrix
		mxArray* pVector = mxCreateDoubleMatrix(1, items.size(), mxREAL);
		double* out = mxGetPr(pVector);

		// loop elements
		for (unsigned int i = 0; i < items.size(); i++) {
			out[i] = boost::lexical_cast<float32>(items[i]);
		}
		return pVector;
	}
	
	// number
	char* end;
	double content = ::strtod(input.c_str(), &end);
	bool isnumber = !*end;
	if (isnumber) {
		return mxCreateDoubleScalar(content);
	}
	
	// string
	return mxCreateString(input.c_str());
}
//-----------------------------------------------------------------------------------------
// turn a c++ map into a matlab struct
mxArray* buildStruct(std::map<std::string, mxArray*> mInput) 
{
	mwSize dims[2] = {1, 1};
	mxArray* res = mxCreateStructArray(2,dims,0,0);
	
	for (std::map<std::string, mxArray*>::iterator it = mInput.begin(); it != mInput.end(); it++) {
		mxAddField(res, (*it).first.c_str());
		mxSetField(res, 0, (*it).first.c_str(), (*it).second);
	}
	return res;
}










