#ifndef LOGGERS_H
#define LOGGERS_H

#include <vector>
#include <string>
#include <memory>
#include "models/model.H"
#include "owned-ptr.H"
#include "mcmc/logger.H"
#include "mcmc/mcmc.H" // for MCMC::Logger

std::vector<MCMC::Logger> construct_loggers(owned_ptr<Model>& M, int subsample, const std::vector<std::string>& Rao_Blackwellize, int proc_id, const std::string& dir_name);

#endif
