#include <ctime>
#include <iostream>
#include <vector>
#include "filter-training.h"
#include "image-analyzer.h"
#include "util.h"

using namespace std;

vector<Filter> FilterTraining::Training(const string& original_wave_path,
	const string& degraded_wave_path) {
	vector<Filter> filters;
	_GenerateFilter();
	_PrepareSamples(original_wave_path, degraded_wave_path);
	_PreComputeEnergy();
	_CalculateThreshold();
	_Training(&filters);
	return filters;
}

// 5 kinds of filters, totally 26301 filters.
void FilterTraining::_GenerateFilter() {
	std::vector<int> time_range = { 1, 2, 3, 5, 7, 11, 17, 25, 38, 57, 82 }; // time from 1 frame to 82 frame in exponential steps of 1.5

	// Generate filters of type 1. Totally 5808 filters.
	for (int time_end = 0; time_end < time_range.size(); time_end++) {
		for (int band_start = 0; band_start < 32; band_start++) {
			for (int band_end = band_start + 2; band_end <= 33; band_end++) {
				Filter filter;
				filter.type = 1;
				filter.time_end = time_range[time_end];
				filter.band_start = band_start;
				filter.band_end = band_end;
				filters.push_back(filter);
			}
		}
	}

	// Generate filters of type 2. Totally 5610 filters.
	for (int time_end = 1; time_end < time_range.size(); time_end++) {
		for (int band_start = 0; band_start < 33; band_start++) {
			for (int band_end = band_start + 1; band_end <= 33; band_end++) {
				Filter filter;
				filter.type = 2;
				filter.time_end = time_range[time_end];
				filter.band_start = band_start;
				filter.band_end = band_end;
				filters.push_back(filter);
			}
		}
	}

	// Generate filters of type 3. Totally 5280 filters.
	for (int time_end = 1; time_end < time_range.size(); time_end++) {
		for (int band_start = 0; band_start < 32; band_start++) {
			for (int band_end = band_start + 2; band_end <= 33; band_end++) {
				Filter filter;
				filter.type = 3;
				filter.time_end = time_range[time_end];
				filter.band_start = band_start;
				filter.band_end = band_end;
				filters.push_back(filter);
			}
		}
	}

	// Generate filters of type 4. Totally 5115 filters.
	for (int time_end = 0; time_end < time_range.size(); time_end++) {
		for (int band_start = 0; band_start < 30; band_start++) {
			for (int band_end = band_start + 4; band_end <= 33; band_end++) {
				Filter filter;
				filter.type = 4;
				filter.time_end = time_range[time_end];
				filter.band_start = band_start;
				filter.band_end = band_end;
				filters.push_back(filter);
			}
		}
	}

	// Generate filters of type 5. Totally 4488 filters.
	for (int time_end = 3; time_end < time_range.size(); time_end++) {
		for (int band_start = 0; band_start < 33; band_start++) {
			for (int band_end = band_start + 1; band_end <= 33; band_end++) {
				Filter filter;
				filter.type = 5;
				filter.time_end = time_range[time_end];
				filter.band_start = band_start;
				filter.band_end = band_end;
				filters.push_back(filter);
			}
		}
	}
}

void FilterTraining::_PrepareSamples(const string& original_wave_path,
	const string& degraded_wave_path) {
	vector<string> original_files;
	original_files = Util::load_dir(original_wave_path, "wav");
	
	for (const auto file : original_files) {
		cout << file << endl;
		vector<Sample> original_samples, degraded_samples;

		ImageAnalyzer image_analyzer(file);
		image_analyzer.CreateImage();
		image_analyzer.GetSamples(&original_samples);
		string filename = file.substr(file.find_last_of("\\") + 1, file.size());
		image_analyzer.SetWavepath(degraded_wave_path + "\\" + filename);
		image_analyzer.CreateImage();
		image_analyzer.GetSamples(&degraded_samples);

		// Add positive samples.
		for (size_t i = 0; i < original_samples.size(); i++) {
			SamplePair pair;
			pair.sample1 = original_samples[i];
			pair.sample2 = degraded_samples[i];
			pair.label = true;
			sample_pairs.push_back(pair);
		}
	}

	// Add negative samples.
	srand((unsigned)time(NULL));
	size_t pos_num = sample_pairs.size();
	for (size_t neg_num = 0; neg_num < pos_num;) {
		int neg1 = rand() % pos_num;
		int neg2 = rand() % pos_num;
		// Successfully generate a negative sample with different song ids.
		if (sample_pairs[neg1].sample1.song_id != sample_pairs[neg2].sample2.song_id) {
			SamplePair pair;
			pair.sample1 = sample_pairs[neg1].sample1;
			pair.sample2 = sample_pairs[neg2].sample2;
			pair.label = false;
			sample_pairs.push_back(pair);
			neg_num++;		
		}
	}
	cerr << "Totally generate " << sample_pairs.size() << " samples." << endl;
	return;
}

void FilterTraining::_PreComputeEnergy() {
	cout << filters.size() << endl;
	time_t start, end;
	start = clock();
	energy.resize(filters.size());
	for (size_t i = 0; i < filters.size(); i++)
		energy[i].resize(sample_pairs.size());
	
	for (size_t i = 0; i < filters.size(); i++) {
		for (size_t j = 0; j < sample_pairs.size(); j++) {
			energy[i][j].energy1 = filters[i].GetEnergy(sample_pairs[j].sample1.image);
			energy[i][j].energy2 = filters[i].GetEnergy(sample_pairs[j].sample2.image);
		}
	}
	end = clock();
	cerr << "Pre compute takes " << (end - start) / CLOCKS_PER_SEC << "s." << endl;
}

void FilterTraining::_CalculateThreshold() {
	for (size_t i = 0; i < filters.size(); i++) {
		long long threshold = 0;
		int number = 0;
		for (size_t j = 0; j < sample_pairs.size(); j++) {
			if (sample_pairs[j].label == false)
				break;
			threshold += (int)energy[i][j].energy1;
			threshold += (int)energy[i][j].energy2;
			number += 2;
		}
		filters[i].threshold = (int)(threshold / number);
	}
	cerr << "Generate thresholds done!" << endl;
	return;
}

void FilterTraining::_Training(vector<Filter>* selected_filters) {
	time_t start, end;
	vector<int> selected_filters_idx;

	// Initialize the weight of each filter as 1/n.
	for (size_t i = 0; i < sample_pairs.size(); i++)
		sample_pairs[i].weight = 1.0 / sample_pairs.size();

	// 32 times interation, 32 weak classifiers.
	for (int number = 0; number < 32; number++) {
		start = clock();
		// 1. Find the hypothesis that minimizes weighted error.
		size_t selected_filter = -1;
		int min_err = INT_MAX;
		for (size_t i = 0; i < filters.size(); i++) {
			int local_min_err = 0;
			for (size_t j = 0; j < sample_pairs.size(); j++) {
				bool sign1 = (energy[i][j].energy1 - filters[i].threshold) > 0 ? true : false;
				bool sign2 = (energy[i][j].energy2 - filters[i].threshold) > 0 ? true : false;
				bool re = sign1 & sign2;
				if (re != sample_pairs[j].label)
					local_min_err++;
			}
			if (local_min_err < min_err &&
				find(selected_filters_idx.begin(), selected_filters_idx.end(), i) == selected_filters_idx.end()) {
				min_err = local_min_err;
				selected_filter = i;
			}
		}
		selected_filters->push_back(filters[selected_filter]);
		selected_filters_idx.push_back(selected_filter);
		//cerr << "Select filter: " << selected_filter << endl;
		// 2. Calculate weight error.
		double weight_error = 0;
		Filter& filter = filters[selected_filter];
		vector<size_t> error_samples;
		for (size_t i = 0; i < sample_pairs.size(); i++) {
			bool sign1 = (energy[selected_filter][i].energy1 - filter.threshold) > 0 ? true : false;
			bool sign2 = (energy[selected_filter][i].energy2 - filter.threshold) > 0 ? true : false;
			bool re = sign1 & sign2;
			if (re != sample_pairs[i].label) {
				weight_error += sample_pairs[i].weight;
				error_samples.push_back(i);
			}
		}

		// 3. Assign confidence to hm.
		double confidence = log((1 - weight_error) / weight_error);
		filter.confidence = confidence;

		// 4. Update weights for matching pairs.
		for (const size_t i : error_samples) {
			if (sample_pairs[i].label == true)
				sample_pairs[i].weight *= ((1 - weight_error) / weight_error);
		}

		// 5. Normalize weights.
		double matching_weight = 0, non_matching_weight = 0;
		for (const SamplePair& pair : sample_pairs) {
			if (pair.label == true)
				matching_weight += pair.weight;
			else
				non_matching_weight += pair.weight;
		}
		for (SamplePair& pair : sample_pairs) {
			if (pair.label == true)
				pair.weight /= matching_weight;
			else
				pair.weight /= non_matching_weight;
			pair.weight /= 2;
		}
		end = clock();
		//cerr << "Round " << number << " done. Using: " << (end - start) / CLOCKS_PER_SEC << "s." << endl;
	}
	return;
}