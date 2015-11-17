// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <climits>
#include <limits>
#include <cfloat>
#include <cmath>
#include <vector>

#include <cstdlib>

#define DEFAULT_PRECISION 15

using namespace std;

int main(int argc, char** argv)
{
	if(argc < 4)
	{
		std::cout << "Usage: "<<argv[0]<< " <inputfilename_without_xyz> <outputfilename_without_xyz> <num_desired_parts>\n\n";

		return 1;
	}

        int num_desired_parts = std::atoi(argv[3]);

        double point_num_threshold = 1.0/num_desired_parts;
 
        std::string output_full_path = std::string(argv[2]);
 
	unsigned int file_name_start_index  = output_full_path.find_last_of("\\/");
	std::string results_directory = output_full_path.substr(0, file_name_start_index) + "/";
	std::string results_filename = output_full_path.substr(file_name_start_index + 1, output_full_path.length());

	std::cout << results_directory  <<"\n";
	std::cout << results_filename  <<"\n";

	ofstream files_to_keep(std::string(results_directory+"leaf_file_indices.lf").c_str());

	//create kind of a working queue with an index of a file (also filename) and a number of points that are contained in this file 
	std::vector<std::pair<unsigned long long int, unsigned long long int> > working_queue;

	unsigned long long int num_points_whole_file = 0;	
		
	unsigned long long int current_index = 0;
	working_queue.push_back(std::pair<unsigned long long int, unsigned long long int>(current_index , UINT_MAX));

	while(!working_queue.empty())
	//for(int x = 0; x < 3; ++x)
	{
		std::cout<< "NumPointsWhole = "<< num_points_whole_file<<"\n\n";

		unsigned long long int num_points = 0;

		std::string current_filename = "";

		
		std::pair<unsigned long long int, unsigned long long int> curr_file_pair = working_queue.back();

		working_queue.pop_back();

		unsigned long long int file_index = curr_file_pair.first;
		if(file_index != 0)
		{


			std::stringstream number_converter;
		
			number_converter << file_index;

			std::string file_index_as_string;

			number_converter >> file_index_as_string;
			current_filename = (std::string(argv[2]) + "_" + file_index_as_string + ".xyz" );

			std::cout << "Trying to open: "<<current_filename<<"\n\n";
		}
		else
		{

	
			current_filename = std::string(argv[1])+".xyz";
			std::cout << "Trying to open: "<<current_filename<<"\n\n";
		}
		
		std::ifstream gigFile(current_filename.c_str());

		std::string oneline;





		double xMin = std::numeric_limits<double>::max();
	 	double yMin = std::numeric_limits<double>::max();
		double zMin = std::numeric_limits<double>::max();
		double xMax = std::numeric_limits<double>::lowest(); 
		double yMax = std::numeric_limits<double>::lowest();
		double zMax = std::numeric_limits<double>::lowest(); 

		{
			unsigned long long int buckets[100];
	

			for(int i = 0; i < 100; ++i)
			{
				buckets[i] = 0;
			}
			////////////////////////////////
			//first pass: determine bounding box


			std::cout << "Starting to determine bounding box\n\n";

			while(getline(gigFile, oneline))
			{
		
				istringstream lineparser(oneline);
				double pX, pY, pZ;

				lineparser >> std::setprecision(DEFAULT_PRECISION) >> pX;
				lineparser >> std::setprecision(DEFAULT_PRECISION) >> pY;
				lineparser >> std::setprecision(DEFAULT_PRECISION) >> pZ;

				if(pX < xMin)
					xMin = pX;
				if(pY < yMin)
					yMin = pY;
				if(pZ < zMin)
					zMin = pZ;
				if(pX > xMax)
					xMax = pX;
				if(pY > yMax)
					yMax = pY;
				if(pZ > zMax)
					zMax = pZ;




				++num_points;

				if(num_points % 100000000 == 0)
				{
					std::cout<<num_points<<"\n";
				}
		

		
			
			}
			gigFile.close();

			std::cout<<"Counted num points: "<< num_points<<"\n\n";


			std::cout<<"Bounding box is: "<<"["<<xMin<<", "<<xMax<<"], "<<"["<<yMin<<", "<<yMax<<"], "<<"["<<zMin<<", "<<zMax<<"] \n\n";

			//write for the first file the total number of surfels
			if(num_points > num_points_whole_file)
				num_points_whole_file = num_points;


			//open the file again, assume that there was no error to determine a good splitting index

			gigFile.open(current_filename.c_str());
	
			double deltaX = xMax - xMin;
			double deltaY = yMax - yMin;
			double deltaZ = zMax - zMin;

			unsigned long long int longest_axis = (deltaX > deltaY ? (deltaX > deltaZ ? 0 : 2) : (deltaY > deltaZ ? 1 : 2) ); 

			std::cout<<"Counting surfels into buckets\n\n";

			while(getline(gigFile, oneline))
			{

				istringstream lineparser(oneline);
				double pX, pY, pZ;

				lineparser >> std::setprecision(DEFAULT_PRECISION) >> pX;
				lineparser >> std::setprecision(DEFAULT_PRECISION) >> pY;
				lineparser >> std::setprecision(DEFAULT_PRECISION) >> pZ;

				if( longest_axis == 0)
				{
			

					unsigned long long int index = ( (( pX - xMin )*(99.0f) ) / (xMax - xMin)) + 0.5f ;

					if(index < 0)
						index = 0;
					if(index > 99)
						index = 99;

					++buckets[index];

					
				}
				else if( longest_axis == 1)
				{
			

					unsigned long long int index = ( (( pY - yMin )*(99.0f) ) / (yMax - yMin)) + 0.5f ;

					if(index < 0)
						index = 0;
					if(index > 99)
						index = 99;

					++buckets[index];

					
				}
				else if( longest_axis == 2)
				{
			

					unsigned long long int index = ( (( pZ - zMin )*(99.0f) ) / (zMax - zMin)) + 0.5f ;

					if(index < 0)
						index = 0;
					if(index > 99)
						index = 99;

					++buckets[index];

					
				}
			}

			unsigned long long int sum = 0;

			for(int i = 0; i < 100; ++i)
			{
			//	std::cout<<"Bucket "<<i<<": "<<buckets[i]<<"\n";
				
				sum += buckets[i];
				

			}
			std::cout << "Sum: "<<sum<<"\n\n";

			unsigned long long int num_points_in_cut_before_half = 0;
			unsigned long long int num_points_in_cut_after_half = 0;

			unsigned long long int num_temp_points = 0;

			double splitPos = 0.0;


			unsigned long long  int index_after_half_points = 99;


		//	std::cout<<"Before going into strange loop. NumSurfels: "<<num_points<<"\n";

			for(int i = 0; i < 100; ++i)
			{
				num_temp_points = buckets[i];
			
				if(num_points_in_cut_before_half < num_points / 2)
				{
					num_points_in_cut_before_half += num_temp_points;
//				  std::cout<<"I am currently at: "<<num_points_in_cut_before_half<<" points\n";
				  //std::cout<<"num points: "<< num_points/2 <<"\n\n";	
				} 
				else
				{

//					std::cout<<"I am in an important loop \n\n";
					num_points_in_cut_after_half += num_temp_points + num_points_in_cut_before_half;
					index_after_half_points = i;
					break;
				}

			}
		
			unsigned long long int actual_split_index = 0;
		
			std::cout<<"index after half_points: "<<index_after_half_points<<"\n\n";
	
			if( ( std::abs( (num_points/2.0) - num_points_in_cut_before_half) ) <  ( std::abs( (num_points/2.0) - num_points_in_cut_after_half) ) )
			{
				actual_split_index = index_after_half_points - 1;
			}
			else
			{
				actual_split_index = index_after_half_points;
			}


			if(longest_axis == 0)
			{
				splitPos = xMin + (((xMax-xMin)/100.0) * (actual_split_index+0.5) ); 
			}
			else if(longest_axis == 1)
			{
				splitPos = yMin + (((yMax-yMin)/100.0) * (actual_split_index+0.5) ); 
			}
			else if(longest_axis == 2)
			{
				splitPos = zMin + (((zMax-zMin)/100.0) * (actual_split_index+0.5) ); 
			}


			std::cout<<"actual split index: "<<actual_split_index<<"  splitPos: "<<splitPos<<"\n\n";
	 
			gigFile.close();


			//open the file again, assume that there was no error
		
			unsigned long long int smaller_file_index = ++current_index;
			unsigned long long int bigger_file_index = ++current_index;

			stringstream number_converter;


			std::cout << "smaller_file_index: "<<smaller_file_index<<"\n";
			std::cout << "bigger_file_index: " <<bigger_file_index<<"\n\n";

			//convert smaller index to string and open file for splats that have pos < splitpos
			number_converter << smaller_file_index;
		
			std::string file_index_as_string_1;
			number_converter >> file_index_as_string_1;

			std::cout<<"fi_1_as_string " << file_index_as_string_1<<"\n";

			ofstream smaller_file( (std::string(argv[2]) + "_" + file_index_as_string_1 + ".xyz").c_str());

			unsigned long long int num_surfel_in_smaller_file = 0;

			//convert smaller index to string and open file for splats that have pos >= splitpos
			stringstream number_converter2;
			number_converter2 << bigger_file_index;
		
			std::string file_index_as_string_2;
			number_converter2 >> file_index_as_string_2;
		std::cout<<"fi_2_as_string " << file_index_as_string_2<<"\n";


			ofstream bigger_file((std::string(argv[2]) + "_" + file_index_as_string_2 + ".xyz").c_str());
		
			unsigned long long int num_surfel_in_bigger_file = 0;

			gigFile.open(current_filename.c_str());

			std::cout << "Sorting surfels into files\n\n";
			while(getline(gigFile, oneline))
			{
				istringstream lineparser(oneline);
				double pX, pY, pZ;
				unsigned R, G, B;

				lineparser >> std::setprecision(DEFAULT_PRECISION) >> pX;
				lineparser >> std::setprecision(DEFAULT_PRECISION) >> pY;
				lineparser >> std::setprecision(DEFAULT_PRECISION) >> pZ;
				lineparser >> R;
				lineparser >> G;
				lineparser >> B;
	

			//	std::cout<<"longest axis: "<<longest_axis<<"\n";
//				std::cout<<"Splitpos: "<<splitPos<<"\n";
				if( longest_axis == 0)
				{
					if(pX < splitPos)
					{

//						std::cout<<"Writing something\n\n";
						smaller_file << std::setprecision(DEFAULT_PRECISION) << pX << " ";
                                                smaller_file << std::setprecision(DEFAULT_PRECISION) << pY << " ";
                                                smaller_file << std::setprecision(DEFAULT_PRECISION) << pZ << " ";
                                                smaller_file << R << " " << G << " " << B << "\n";

						++num_surfel_in_smaller_file;
					} 
					else
					{
						bigger_file << std::setprecision(DEFAULT_PRECISION) << pX << " ";
                                                bigger_file << std::setprecision(DEFAULT_PRECISION) << pY << " ";
                                                bigger_file << std::setprecision(DEFAULT_PRECISION) << pZ << " ";
                                                bigger_file << R << " " << G << " " << B << "\n";

						++num_surfel_in_bigger_file;
					}	
				}
				else if( longest_axis == 1)
				{
//						std::cout<<" 1 Writing something\n\n";

					if(pY < splitPos)
					{
                                          	smaller_file << std::setprecision(DEFAULT_PRECISION) << pX << " ";
                                                smaller_file << std::setprecision(DEFAULT_PRECISION) << pY << " ";
                                                smaller_file << std::setprecision(DEFAULT_PRECISION) << pZ << " ";
                                                smaller_file << R << " " << G << " " << B << "\n";

						++num_surfel_in_smaller_file;
					} 
					else
					{
                                                bigger_file << std::setprecision(DEFAULT_PRECISION) << pX << " ";
                                                bigger_file << std::setprecision(DEFAULT_PRECISION) << pY << " ";
                                                bigger_file << std::setprecision(DEFAULT_PRECISION) << pZ << " ";
                                                bigger_file << R << " " << G << " " << B << "\n";

						++num_surfel_in_bigger_file;
					}	
				}
				else if( longest_axis == 2)
				{
//				std::cout<<"2 Writing something\n\n";
//				std::cout<<pX<<" " << pY <<" " <<pZ<<"\n";
					if(pZ < splitPos)
					{
                                                smaller_file << std::setprecision(DEFAULT_PRECISION) << pX << " ";
                                                smaller_file << std::setprecision(DEFAULT_PRECISION) << pY << " ";
                                                smaller_file << std::setprecision(DEFAULT_PRECISION) << pZ << " ";
                                                smaller_file << R << " " << G << " " << B << "\n";

						++num_surfel_in_smaller_file;
					} 
					else
					{
                                                bigger_file << std::setprecision(DEFAULT_PRECISION) << pX << " ";
                                                bigger_file << std::setprecision(DEFAULT_PRECISION) << pY << " ";
                                                bigger_file << std::setprecision(DEFAULT_PRECISION) << pZ << " ";
                                                bigger_file << R << " " << G << " " << B << "\n";

						++num_surfel_in_bigger_file;
					}	
				}
			}

			if(num_surfel_in_smaller_file > point_num_threshold * num_points_whole_file)
			{
				working_queue.push_back(std::pair<unsigned long long int, unsigned long long int>(smaller_file_index, num_surfel_in_smaller_file) );
				std::cout << "Pushing back File number: " << smaller_file_index<<"\n\n";
			}
			else
			{
				files_to_keep << smaller_file_index<<"\n";
				std::cout << "This file is small enough: " << smaller_file_index<<"\n\n";
			}

			if(num_surfel_in_bigger_file > point_num_threshold * num_points_whole_file)
			{
				working_queue.push_back(std::pair<unsigned long long int, unsigned long long int>(bigger_file_index, num_surfel_in_bigger_file) );
				std::cout << "Pushing back File number: " << bigger_file_index<<"\n\n";
			}
			else
			{
				files_to_keep << bigger_file_index<<"\n";
				std::cout << "This file is small enough: " << bigger_file_index<<"\n\n";
			}
		
		
			gigFile.close();
			smaller_file.close();
			bigger_file.close();
			
		

		}//end of one file




	}
	files_to_keep.close();

	//reopen the leaf node file to delete all of the other files.
	
	ifstream leaf_node_file_list(std::string(results_directory+"leaf_file_indices.lf").c_str());
	
	std::string line_buffer;

	unsigned file_deletion_starting_index = 1;
	while(std::getline(leaf_node_file_list, line_buffer) )
	{
	 	unsigned int read_number = std::stoi(line_buffer);
		std::cout << "ReadNumber: " << read_number <<"\n";

		for(file_deletion_starting_index; file_deletion_starting_index < read_number; ++file_deletion_starting_index)
		{
			std::cout << "Deleting intermediate File: "<<output_full_path + "_" + std::to_string(file_deletion_starting_index) + ".xyz" << "\n";
			std::remove(std::string(output_full_path + "_" + std::to_string(file_deletion_starting_index) + ".xyz").c_str() );

		}
		
		++file_deletion_starting_index;
		//std::cout << ": " << line_buffer <<"\n";
	}
	
	std::cout << "\n\nDONE SPLITTING.\n";
	
	leaf_node_file_list.close();

	std::cout << "\nDeleting intermediate File: "<<results_directory + "leaf_file_indices.lf" << "\n";
	std::remove(std::string(results_directory + "leaf_file_indices.lf").c_str() );

	return 0;

}
