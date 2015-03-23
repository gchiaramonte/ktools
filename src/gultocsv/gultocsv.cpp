/*
* Copyright (c)2015 Oasis LMF Limited 
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

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#ifdef _MSC_VER
#include <fcntl.h>
#include <io.h>
#endif


using namespace std;
#include "../include/oasis.h"

void doit()
{

#ifdef _MSC_VER
        _setmode(_fileno(stdout), O_BINARY);
        _setmode(_fileno(stdin), O_BINARY);
#endif

#ifdef __unix
        freopen(NULL, "rb", stdin);
        freopen(NULL, "wb", stdout);
#endif

	int stream_type = 0;
	int i = fread(&stream_type, sizeof(stream_type), 1, stdin);
	if (stream_type != 2) {
		std::cerr << "invalid stream time";
		exit(-1);
	}


    gulGulSampeslevel p;
    i = fread(&p, sizeof(gulGulSampeslevel), 1, stdin);
    while (i != 0) {
        printf("%d, %d, %d, %f\n", p.event_id,p.item_id,p.sidx, p.gul);
        i = fread(&p, sizeof(gulGulSampeslevel), 1, stdin);
    }

}
int main()
{
    doit();
    return 0;
}
