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

#ifndef __DYSECTAPI_DESC_VAR_H
#define __DYSECTAPI_DESC_VAR_H

namespace DysectAPI {
  class Probe;

  class DescribeVariable : public AggregateFunction {
    std::string varName;
    std::vector<std::string> varSpecs;
    std::map<int, AggregateFunction*> aggregates;
    StaticStr* strAgg;

    std::string outStr;
    std::vector<std::string> output;

  public:
    DescribeVariable(std::string name);

    int getSize();
    int writeSubpacket(char *p);
    bool collect(void* process, void* thread);
    bool clear();

    bool getStr(std::string& str);
    bool print();
    bool aggregate(AggregateFunction* agg);

    bool isSynthetic();
    bool getRealAggregates(std::vector<AggregateFunction*>& aggregates);
    bool fetchAggregates(Probe* probe);
  };
}

#endif
