/*
Copyright (c) 2012-2013, Lawrence Livermore National Security, LLC.
Produced at the Lawrence Livermore National Laboratory.
Written by Niklas Nielsen and Gregory L. Lee <lee218@llnl.gov>.
LLNL-CODE-645136.
All rights reserved. 

This file is part of Dysect API. For details, see 
https://github.com/lee218llnl. 

Please also read this link the file "LICENSE" included in thie package for
Our Notice and GNU Lesser General Public License. 

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License (as published by the Free Software 
Foundation) version 2.1 dated February 1999. 

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU 
General Public License for more details. 

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to 

the Free Software Foundation, Inc.
59 Temple Place, Suite 330
Boston, MA 02111-1307 USA 
*/

#include "DysectAPI/Aggregates/Aggregate.h"
#include "DysectAPI.h"

using namespace std;
using namespace DysectAPI;
using namespace Dyninst;
using namespace ProcControlAPI;

bool DescribeVariable::collect(void* process, void *thread) {
  Process::const_ptr process_ptr = *(Process::const_ptr*)process;
  Thread::const_ptr thread_ptr = *(Thread::const_ptr*)thread;

  if(!process_ptr) {
    return Err::verbose(false, "Process object not available");
  }

  Walker* proc = (Walker*)process_ptr->getData();

  if(!proc) {
    return Err::verbose(false, "Could not get walker from process");
  }
  
  DataLocation* varLocation;

  if(!DataLocation::findVariable(process_ptr, proc, varName, varLocation)) {
    return Err::warn(false, "Could not locate variable '%s'", varName.c_str());
  }

  string varSpec = "";

  if(varLocation) {
    if(!varLocation->isStructure()) {
      
      // XXX: Add more sophisticated procedure to recognize
      // already prepared aggregates
      if(varSpecs.empty()) {
        const int bufSize = 512;
        char buf[bufSize];

        Min* minagg = new Min("%d", varName.c_str());
        Max* maxagg = new Max("%d", varName.c_str());

        if(!minagg) {
          return Err::verbose(false, "Minimum aggregate could not be created");
        }

        minagg->collect(process, thread);
        maxagg->collect(process, thread);
      
        aggregates.insert(pair<int, AggregateFunction*>(minagg->getId(), minagg));
        aggregates.insert(pair<int, AggregateFunction*>(maxagg->getId(), maxagg));

        string format = "%d";
        Type* symType = varLocation->getType();
        if(symType && symType->getPointerType()) {
          format="%p";
        } else {
          string typeName = symType->getName();
        }

        snprintf((char*)&buf, bufSize, "%s[%s:%d:%d]", varName.c_str(), format.c_str(), minagg->getId(), maxagg->getId());
        varSpec.append(buf);
  
        varSpecs.push_back(varSpec);

        Err::verbose(true, "Var spec: %s", varSpec.c_str());
      } else {
        // Collect existing
        if(!aggregates.empty()) {
          map<int, AggregateFunction*>::iterator aggIter = aggregates.begin();
          for(; aggIter != aggregates.end(); aggIter++) {
            AggregateFunction* aggFunc = aggIter->second;
            if(aggFunc) {
              aggFunc->collect(process, thread);
            }
          }
        }
      }
    } else {
      Err::verbose(true, "DescribeVariable(%s) collected: isStructure(%s)", varName.c_str(), varLocation->isStructure() ? "yes" : "no");
      Type* symType = varLocation->getType();
      typeStruct *stType = symType->getStructType();
      if(stType) {
        vector<Field*>* fields = stType->getComponents();
        if(fields) {
          vector<Field*>::iterator fieldIter = fields->begin();

          for(;fieldIter != fields->end(); fieldIter++) {
            Field* field = *fieldIter;

            if(field) {
              Err::verbose(true, "Member: '%s'", field->getName().c_str());
            }
          }
        }
      }
    }
  }

  

  return true;
}

bool DescribeVariable::fetchAggregates(Probe* probe) {
  return true;
}
