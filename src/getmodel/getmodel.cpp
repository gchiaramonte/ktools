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
/*
    Author: Mark Pinkerton  email: mark.pinkerton@oasislmf.org

*/  
#include "string.h"
#include <iostream>
#include <list>
#include <set>
#include <map>
#include "getmodel.hpp"
#include "../include/oasis.hpp"

struct Exposure 
{
    int location_id;
    int areaperil_id;
    int vulnerability_id;
    int group_id;
    float tiv;
};


struct Result {

    Result(): prob(0.0), damage(0.0) {
        //
    }

    Result(float damage, float prob);

    float prob;
    float damage;
};

Result::Result(float prob, float damage):
     prob(prob), damage(damage)
{
    //
}


// Is this the number of damage bins?
const int MAX_RESULTS = 100;

const size_t SIZE_OF_INT = sizeof(int);
const size_t SIZE_OF_FLOAT = sizeof(float);
const size_t SIZE_OF_RESULT = sizeof(Result);

const unsigned int OUTPUT_STREAM_TYPE = 1;

getmodel::getmodel()
{
    //
}

getmodel::~getmodel()
{
    if (_temp_results != nullptr) delete _temp_results;
}

void getmodel::getVulnerabilities()
{
    // Read the vulnerabilities
    Vulnerability vulnerability;

    FILE *fin = fopen(VULNERABILITY_FILE, "rb");
    if (fin == nullptr) {
		fprintf(stderr, "%s: cannot open %s\n", __func__, VULNERABILITY_FILE);
        exit(EXIT_FAILURE);
    }
    int current_vulnerability_id = -1;
	fread(&_num_damage_bins, sizeof(_num_damage_bins), 1, fin);

    while (fread(&vulnerability, sizeof(vulnerability), 1, fin) != 0) {
		if (_num_intensity_bins >= vulnerability.intensity_bin_id) {
			if (vulnerability.vulnerability_id != current_vulnerability_id)
			{
				_vulnerabilities[vulnerability.vulnerability_id] = std::vector<float>(_num_intensity_bins * _num_damage_bins, 0.0);
				current_vulnerability_id = vulnerability.vulnerability_id;
			}
			int vulnerabilityIndex = getVulnerabilityIndex(vulnerability.intensity_bin_id, vulnerability.damage_bin_id);
			_vulnerabilities[vulnerability.vulnerability_id][vulnerabilityIndex] = vulnerability.probability;
		}
    }
    fclose(fin);
}


void getmodel::getIntensityInfo()
{
	FILE* fin = fopen(FOOTPRINT_FILE, "rb");
	if (fin == nullptr) {
		fprintf(stderr, "%s: cannot open %s\n", __func__, FOOTPRINT_FILE);
		exit(EXIT_FAILURE);
	}
	fread(&_num_intensity_bins, sizeof(_num_intensity_bins), 1, fin);	
	fread(&_has_intensity_uncertainty, sizeof(_has_intensity_uncertainty), 1, fin);
	fclose(fin);
}

void getmodel::getItems()
{
    // Read the exposures and generate a set of vulnerabilities by area peril
    item item_rec;

    FILE* fin = fopen(ITEMS_FILE, "rb");
    if (fin == nullptr) {
		fprintf(stderr, "%s: cannot open %s\n", __func__, ITEMS_FILE);
        exit(EXIT_FAILURE);
    }

    while (fread(&item_rec, sizeof(item_rec), 1, fin) != 0) {
        if (_vulnerability_ids_by_area_peril.count(item_rec.areaperil_id) == 0)
            _vulnerability_ids_by_area_peril[item_rec.areaperil_id] = std::set<int>();
        _vulnerability_ids_by_area_peril[item_rec.areaperil_id].insert(item_rec.vulnerability_id);
        _area_perils.insert(item_rec.areaperil_id);
    }
    fclose(fin);
}

void getmodel::getDamageBinDictionary()
{
    FILE *fin = fopen(DAMAGE_BIN_DICT_FILE, "rb");
    if (fin == nullptr) {       
		fprintf(stderr, "%s: cannot open %s\n", __func__, DAMAGE_BIN_DICT_FILE);
        exit(-1);
    }
    flseek(fin, 0L, SEEK_END);
    long long sz = fltell(fin);
    flseek(fin, 0L, SEEK_SET);
    int nrec = static_cast<int>(sz / sizeof(damagebindictionary));
    
    damagebindictionary *damage_bins = new damagebindictionary[nrec];
    
    if (fread(damage_bins, sizeof(damagebindictionary), nrec, fin) != nrec) {
        std::ostringstream poss;
        poss << "Error reading file " << DAMAGE_BIN_DICT_FILE;
        perror(poss.str().c_str());
        exit(-1);
    }

    fclose(fin);

    _mean_damage_bins = std::vector<float>(nrec);
    for (int i = 0; i < nrec; i++) {
        _mean_damage_bins[i] = damage_bins[i].interpolation;
    }
    
    delete[] damage_bins;
}

void getmodel::initOutputStream()
{
    fwrite(&OUTPUT_STREAM_TYPE, sizeof(OUTPUT_STREAM_TYPE), 1, stdout);
}

void getmodel::doResults(
    int& event_id, 
    int& areaperil_id, 
    std::map<int, std::set<int>>& vulnerabilities_by_area_peril, 
    std::map<int, std::vector<float>>& vulnerabilities, 
    std::vector<float> intensity) const
{
    for (int vulnerability_id : vulnerabilities_by_area_peril[areaperil_id])
    {
        auto results = new Result[_num_damage_bins];
        int result_index = 0;
        auto vulnerability = vulnerabilities[vulnerability_id];
        int vulnerability_index = 0;
        double cumulative_prob = 0;		
        for (int damage_bin_index = 0; damage_bin_index < _num_damage_bins; damage_bin_index++)
        {
            double prob = 0.0f;
            for (int intensity_bin_index = 0;
                    intensity_bin_index < _num_intensity_bins;
                    intensity_bin_index++)
            {
                prob +=
                    vulnerability[vulnerability_index++] * intensity[intensity_bin_index];
            }

            //if (prob > 0 || damage_bin_index == 0)
            //{
            cumulative_prob += prob;
            results[result_index++] =
                Result(static_cast<float>(cumulative_prob), _mean_damage_bins[damage_bin_index]);
            //}
			if (cumulative_prob > 0.999999940) break;		// single precision value approx 1
        }

        int num_results = result_index;
        fwrite(&event_id, SIZE_OF_INT, 1, stdout);
        fwrite(&areaperil_id, SIZE_OF_INT, 1, stdout);
        fwrite(&vulnerability_id, SIZE_OF_INT, 1, stdout);
        fwrite(&num_results, SIZE_OF_INT, 1, stdout);
        fwrite(results, SIZE_OF_RESULT, num_results, stdout);

        delete results;
    }
}


void getmodel::doResultsNoIntensityUncertainty(
    int &event_id,
    int &areaperil_id,
    std::map<int, std::set<int>> &vulnerabilities_by_area_peril,
    std::map<int, std::vector<float>> &vulnerabilities,
    int intensity_bin_index) const
{
    for (int vulnerability_id : vulnerabilities_by_area_peril[areaperil_id])
    {
        int result_index = 0;
        float cumulative_prob = 0;
        for (int damage_bin_index = 1; damage_bin_index <= _num_damage_bins; damage_bin_index++)
        {
            float prob = vulnerabilities[vulnerability_id][getVulnerabilityIndex(intensity_bin_index, damage_bin_index)];

            //if (prob > 0 || damage_bin_index == 0)
            //{
                cumulative_prob += prob;
                _temp_results[result_index++].prob = cumulative_prob;
            //}
         }

        int num_results = result_index;
        fwrite(&event_id, SIZE_OF_INT, 1, stdout);
        fwrite(&areaperil_id, SIZE_OF_INT, 1, stdout);
        fwrite(&vulnerability_id, SIZE_OF_INT, 1, stdout);
        fwrite(&num_results, SIZE_OF_INT, 1, stdout);
        fwrite(_temp_results, SIZE_OF_RESULT, num_results, stdout);
    }
}

int getmodel::getVulnerabilityIndex(int intensity_bin_index, int damage_bin_index) const
{
    return (intensity_bin_index - 1) + ((damage_bin_index - 1) * _num_intensity_bins);
}

void getmodel::init()
{
	getIntensityInfo();

    getItems();    
    getVulnerabilities();
    getDamageBinDictionary();

    _temp_results = new Result[_num_damage_bins];
    for (int damage_bin_index = 1; damage_bin_index <= _num_damage_bins; damage_bin_index++)
    {
        _temp_results[damage_bin_index - 1] = Result(0.0, _mean_damage_bins[damage_bin_index - 1]);
    }
}

void getmodel::doCdf(std::list<int> event_ids)
{
    FILE* fin = fopen(FOOTPRINT_IDX_FILE, "rb");
    if (fin == nullptr) {
		fprintf(stderr, "%s: cannot open %s\n", __func__, FOOTPRINT_IDX_FILE);
        exit(EXIT_FAILURE);
    }

    EventIndex event_index;
    std::map<int, EventIndex> event_index_by_event_id;
    while (fread(&event_index, sizeof(event_index), 1, fin) != 0) {
        event_index_by_event_id[event_index.event_id] = event_index;
    }
    fclose(fin);

    initOutputStream();

    if (_has_intensity_uncertainty)
    {
        doCdfInner(event_ids, event_index_by_event_id);
    }
    else
    {
        doCdfInnerNoIntensityUncertainty(event_ids, event_index_by_event_id);
    }
}

void getmodel::doCdfInner(
    std::list<int> &event_ids, 
    std::map<int, EventIndex> &event_index_by_event_id)
{
    auto sizeof_EventKey = sizeof(EventRow);
    auto fin = fopen(FOOTPRINT_FILE, "rb");
    auto intensity = std::vector<float>(_num_intensity_bins , 0.0f);

    for (int event_id : event_ids)
    {
        int current_areaperil_id = -1;
        bool do_cdf_for_area_peril = false;
        intensity = std::vector<float>(_num_intensity_bins, 0.0f);

        if (event_index_by_event_id.count(event_id) == 0) continue;
        flseek(fin, event_index_by_event_id[event_id].offset, 0);
        EventRow event_key;
        int number_of_event_records = static_cast<int>(event_index_by_event_id[event_id].size / sizeof_EventKey);
        for (int i = 0; i < number_of_event_records; i++)
        {
            fread(&event_key, sizeof(event_key), 1, fin);
            if (event_key.areaperil_id != current_areaperil_id)
            {
                if (do_cdf_for_area_peril)
                {
                    // Generate and write the results
                    doResults(
                        event_id,
                        current_areaperil_id,
                        _vulnerability_ids_by_area_peril,
                        _vulnerabilities,
                        intensity);
                    intensity = std::vector<float>(_num_intensity_bins , 0.0f);
                }
                current_areaperil_id = event_key.areaperil_id;
                do_cdf_for_area_peril = (_area_perils.count(current_areaperil_id) == 1);
            }
            if (do_cdf_for_area_peril)
            {
                intensity[event_key.intensity_bin_id - 1] = event_key.probability;
            }
        }
        // Write out results for last event record
        if (do_cdf_for_area_peril)
        {
            // Generate and write the results
            doResults(
                event_id,
                current_areaperil_id,
                _vulnerability_ids_by_area_peril,
                _vulnerabilities,
                intensity);
        }
    }

    fclose(fin);
}

void getmodel::doCdfInnerNoIntensityUncertainty(
    std::list<int> &event_ids, 
    std::map<int, EventIndex> &event_index_by_event_id)
{
    auto sizeof_EventKey = sizeof(EventRow);
    auto fin = fopen(FOOTPRINT_FILE, "rb");

    for (int event_id : event_ids)
    {
        if (event_index_by_event_id.count(event_id) == 0) continue;
        flseek(fin, event_index_by_event_id[event_id].offset, 0);

        int number_of_event_records = static_cast<int>(event_index_by_event_id[event_id].size / sizeof_EventKey);
        for (int i = 0; i < number_of_event_records; i++)
        {
            EventRow event_key;
            fread(&event_key, sizeof(event_key), 1, fin);

            bool do_cdf_for_area_peril = (_area_perils.count(event_key.areaperil_id) == 1);
            if (do_cdf_for_area_peril)
            {
                // Generate and write the results
                doResultsNoIntensityUncertainty(
                    event_id,
                    event_key.areaperil_id,
                    _vulnerability_ids_by_area_peril,
                    _vulnerabilities,
                    event_key.intensity_bin_id);
            }
        }
    }

    fclose(fin);
}
