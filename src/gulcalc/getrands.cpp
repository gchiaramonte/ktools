/*
* Copyright (c)2015 - 2016 Oasis LMF Limited 
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*
*   * Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in
*     the documentation and/or other materials provided with the
*     distribution.
*
*   * Neither the original author of this software nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
* THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
*/

#include "getrands.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include "../include/oasis.hpp"

std::random_device rd;
using namespace std;

getRands::getRands(bool fromFile, int rand_vec_size,int rand_seed) : gen_(rd()), dis_(0, 1), rand_vec_size_(rand_vec_size), rand_seed_(rand_seed)
{
	if (rand_seed_ > 0) gen_.seed(rand_seed_);

	FILE *fin;
	fromFile_ = fromFile;
	base_offset_ = 0;
	if (fromFile_) {
		fin = fopen(RANDOM_FILE, "rb");
		if (fin == NULL){
			fprintf(stderr, "%s: Error opening file %s\n", __func__, RANDOM_FILE);
			exit(-1);
		}
		flseek(fin, 0L, SEEK_END);
		long long p = fltell(fin);
		buffersize_ = p / 4;
//		_buffersize = _buffersize - 1;		// first 4 bytes is the limit
		fseek(fin, 0L, SEEK_SET);

		buf_ = new float[buffersize_];
		if (fread(buf_, sizeof(float), buffersize_, fin) != buffersize_) {
			fprintf(stderr,"%s: Error reading random number file\n", __func__);
			exit(-1);
		}
		fclose(fin);
	}else {
		rnd_.resize(rand_vec_size,-1);
	}
}
