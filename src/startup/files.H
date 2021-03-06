#ifndef FILES_H
#define FILES_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

#include <boost/filesystem/operations.hpp>
#include <boost/program_options.hpp>

void close_files(std::vector<std::shared_ptr<std::ofstream>>& files);

std::string init_dir(const boost::program_options::variables_map& args);

std::vector<std::shared_ptr<std::ostream>> 
init_files(int proc_id, const std::string& dirname,int argc,char* argv[]);

boost::filesystem::path find_exe_path(const boost::filesystem::path& argv0);

#endif
